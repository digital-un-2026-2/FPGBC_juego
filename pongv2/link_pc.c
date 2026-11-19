/*
 * link_pc.c - Cable de enlace simulado en el PC con UDP.
 *
 * Esta en un archivo aparte de hal_pc.c porque en Windows <winsock2.h>
 * choca con raylib.h (los dos definen Rectangle, CloseWindow, etc.).
 *
 * Como jugar 1 VS 1 en el PC:
 *  - En el MISMO computador: abrir pong.exe dos veces. La primera ventana
 *    toma el puerto 47001 y la segunda el 47002; cada una le envia a la otra.
 *  - En DOS computadores de la misma red: en cada uno, antes de abrir el
 *    juego, poner la IP del otro computador en la variable PONG_IP:
 *        PowerShell:  $env:PONG_IP="192.168.1.20"; .\pong.exe
 *    Los dos usan el puerto 47001 (puede que el firewall pida permiso).
 *
 * Los bytes de cada cuadro se juntan y se mandan en un solo datagrama en
 * link_pc_flush(), que llama hal_frame().
 */
#include <stdlib.h>
#include <string.h>
#include "hal.h"
#include "link_pc.h"

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
   typedef SOCKET sock_t;
#  define SOCK_MALO INVALID_SOCKET
#  define cerrar_sock closesocket
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <fcntl.h>
#  include <unistd.h>
   typedef int sock_t;
#  define SOCK_MALO (-1)
#  define cerrar_sock close
#endif

#define PUERTO_1  47001
#define PUERTO_2  47002

static sock_t             sock = SOCK_MALO;
static int                iniciado;          /* 0 = sin intentar, 1 = listo, -1 = fallo */
static struct sockaddr_in destino;

static uint8_t  tx_buf[512];
static unsigned tx_n;
static uint8_t  rx_buf[2048];
static unsigned rx_n, rx_pos;

static int enlazar(unsigned short puerto)
{
    struct sockaddr_in yo;
    memset(&yo, 0, sizeof yo);
    yo.sin_family      = AF_INET;
    yo.sin_addr.s_addr = htonl(INADDR_ANY);
    yo.sin_port        = htons(puerto);
    return bind(sock, (struct sockaddr *)&yo, sizeof yo) == 0;
}

static int iniciar(void)
{
    const char *ip = getenv("PONG_IP");

    if (iniciado) return iniciado > 0;
    iniciado = -1;

#ifdef _WIN32
    {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 0;
    }
#endif
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == SOCK_MALO) return 0;

    memset(&destino, 0, sizeof destino);
    destino.sin_family = AF_INET;

    if (ip && *ip) {
        /* Dos computadores: los dos en 47001, cada uno le envia al otro */
        if (!enlazar(PUERTO_1)) return 0;
        destino.sin_addr.s_addr = inet_addr(ip);
        destino.sin_port        = htons(PUERTO_1);
    } else {
        /* Mismo computador: la primera ventana toma 47001, la segunda 47002 */
        destino.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (enlazar(PUERTO_1))      destino.sin_port = htons(PUERTO_2);
        else if (enlazar(PUERTO_2)) destino.sin_port = htons(PUERTO_1);
        else return 0;
    }

    /* Que recvfrom no se quede esperando */
#ifdef _WIN32
    {
        u_long si = 1;
        ioctlsocket(sock, FIONBIO, &si);
    }
#else
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
#endif

    iniciado = 1;
    return 1;
}

void hal_link_send(uint8_t byte)
{
    if (tx_n < sizeof tx_buf) tx_buf[tx_n++] = byte;
}

int hal_link_recv(void)
{
    int n;
    if (rx_pos < rx_n) return rx_buf[rx_pos++];
    if (!iniciar()) return -1;
    n = (int)recvfrom(sock, (char *)rx_buf, sizeof rx_buf, 0, NULL, NULL);
    if (n <= 0) return -1;           /* no llego nada (o el otro no esta abierto) */
    rx_n   = (unsigned)n;
    rx_pos = 0;
    return rx_buf[rx_pos++];
}

void link_pc_flush(void)
{
    if (tx_n == 0) return;
    if (iniciar())
        sendto(sock, (const char *)tx_buf, (int)tx_n, 0, (struct sockaddr *)&destino, sizeof destino);
    tx_n = 0;
}

void link_pc_close(void)
{
    if (iniciado > 0) cerrar_sock(sock);
#ifdef _WIN32
    if (iniciado > 0) WSACleanup();
#endif
    iniciado = 0;
}
