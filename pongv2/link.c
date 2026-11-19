/*
 * link.c - Armado y lectura de paquetes del cable de enlace (ver link.h).
 *
 * No sabe nada del juego ni del hardware: usa hal_link_send()/hal_link_recv()
 * de hal.h para mover bytes sueltos.
 */
#include "link.h"
#include "hal.h"

enum { RX_SYNC, RX_TIPO, RX_DATOS, RX_SUMA };

static uint8_t rx_estado, rx_tipo, rx_largo, rx_pos, rx_suma;
static uint8_t rx_buf[LINK_MAX_DATOS];

static uint8_t largo_de(uint8_t tipo)
{
    switch (tipo) {
    case LINK_HOLA:    return LINK_LEN_HOLA;
    case LINK_BOTONES: return LINK_LEN_BOTONES;
    case LINK_ESTADO:  return LINK_LEN_ESTADO;
    default:           return 0xFF;            /* tipo desconocido */
    }
}

void link_reset(void)
{
    rx_estado = RX_SYNC;
}

void link_send(uint8_t tipo, const uint8_t *datos)
{
    uint8_t i, n = largo_de(tipo), suma = tipo;
    if (n == 0xFF) return;
    hal_link_send(LINK_SYNC);
    hal_link_send(tipo);
    for (i = 0; i < n; i++) {
        hal_link_send(datos[i]);
        suma = (uint8_t)(suma + datos[i]);
    }
    hal_link_send(suma);
}

uint8_t link_poll(uint8_t *datos)
{
    int c;
    uint8_t i, b;

    while ((c = hal_link_recv()) >= 0) {
        b = (uint8_t)c;
        switch (rx_estado) {
        case RX_SYNC:
            if (b == LINK_SYNC) rx_estado = RX_TIPO;
            break;
        case RX_TIPO:
            rx_largo = largo_de(b);
            if (rx_largo == 0xFF) {             /* basura: volver a buscar 0x5A */
                rx_estado = (b == LINK_SYNC) ? RX_TIPO : RX_SYNC;
                break;
            }
            rx_tipo = b;
            rx_suma = b;
            rx_pos  = 0;
            rx_estado = rx_largo ? RX_DATOS : RX_SUMA;
            break;
        case RX_DATOS:
            rx_buf[rx_pos++] = b;
            rx_suma = (uint8_t)(rx_suma + b);
            if (rx_pos >= rx_largo) rx_estado = RX_SUMA;
            break;
        case RX_SUMA:
            rx_estado = RX_SYNC;
            if (b == rx_suma) {
                for (i = 0; i < rx_largo; i++) datos[i] = rx_buf[i];
                return rx_tipo;                 /* paquete completo y valido */
            }
            break;                              /* suma mala: se descarta */
        }
    }
    return 0;
}
