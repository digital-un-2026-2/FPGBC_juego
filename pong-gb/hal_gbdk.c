/*
 * hal_gbdk.c - HAL para Game Boy usando GBDK-2020.
 *
 * Implementa las mismas funciones de hal.h que hal_pc.c, pero con el
 * hardware real de la Game Boy. game.c, link.c, tiles.c y main.c NO cambian.
 *
 * Compilar con -DPLATFORM_GBDK (ver Makefile.gbdk): asi hal.h fija 160x144.
 *
 * Por que funciona directo:
 *  - El formato de tile de tiles.c (2bpp, 16 bytes, bit 7 = pixel izquierdo)
 *    es exactamente el de la Game Boy.
 *  - El color 0 de un sprite ya es transparente en el hardware.
 *
 * ------------------------------------------------------------------------
 * Cable de enlace (1 VS 1)
 * ------------------------------------------------------------------------
 * El puerto serie de la Game Boy NO es un UART: es SINCRONO. Una consola
 * (la "maestra del reloj") genera el reloj y en cada transferencia las dos
 * se intercambian un byte a la vez (8192 bits/s -> ~1 byte por ms).
 * Esto es independiente de MAESTRO/ESCLAVO del juego (game.c): ese rol sale
 * del numero al azar del saludo. Aqui solo se decide quien da el reloj.
 *
 *  - Normalmente cada consola espera con reloj externo (SC = 0x80).
 *  - Cuando el juego tiene algo que enviar, y nadie le esta dando reloj, la
 *    consola espera unos cuadros al azar (para no chocar con la otra) y se
 *    vuelve "maestra del reloj": encadena transferencias una tras otra
 *    (SC = 0x81) mientras siga habiendo actividad.
 *  - Como cada transferencia mueve un byte en cada sentido, un solo lado
 *    dando el reloj basta para los dos sentidos.
 *  - Si como maestra del reloj no llega ningun encabezado de paquete (0x5A + tipo) en ~2 s, no
 *    hay nadie al otro lado (o chocaron): suelta el reloj y lo reintenta
 *    despues de una espera al azar.
 *  - Entre transferencias no se mandan "huecos" a mitad de un paquete: los
 *    bytes que el juego escribe en un cuadro se publican todos juntos en
 *    hal_frame(). Cuando no hay nada que enviar se manda 0x00 (relleno), que
 *    link.c ignora mientras busca el 0x5A.
 *
 * Capacidad: ~16 bytes por cuadro en cada sentido. El maestro del juego manda
 * 13 bytes por cuadro (paquete de estado) y el esclavo 5: cabe.
 */
#include <gb/gb.h>
#include <stdint.h>
#include "hal.h"
#include "link.h"

static uint8_t display_started;

/* ------------------------------------------------------------------ */
/* Cable de enlace                                                     */
/* ------------------------------------------------------------------ */
#define LINK_IDLE    0x00     /* byte de relleno cuando no hay nada que mandar */
#define LINK_KEEP    120      /* cuadros de actividad tras el ultimo envio (2 s) */
#define LINK_NOSYNC  120      /* cuadros de maestra del reloj sin oir 0x5A       */
#define LINK_GAP     16       /* pausa entre bytes (~80 us) para que la otra consola
                               * alcance a recargar SB antes del siguiente reloj */

#define TX_SIZE 128           /* potencias de 2: los indices se enmascaran */
#define RX_SIZE 128
#define TX_MASK (TX_SIZE - 1)
#define RX_MASK (RX_SIZE - 1)

static uint8_t          tx_buf[TX_SIZE];
static uint8_t          rx_buf[RX_SIZE];
static volatile uint8_t tx_pend;      /* donde escribe el juego (aun no publicado) */
static volatile uint8_t tx_in;        /* hasta donde puede leer la interrupcion    */
static volatile uint8_t tx_out;       /* lo que ya consumio la interrupcion        */
static volatile uint8_t rx_in;        /* la interrupcion escribe aqui              */
static volatile uint8_t rx_out;       /* el juego lee de aqui                      */

static volatile uint8_t link_master;  /* 1 = esta consola da el reloj              */
static volatile uint8_t link_keep;    /* cuadros de actividad que quedan           */
static volatile uint8_t ext_seen;     /* la interrupcion vio una transferencia externa */
static volatile uint8_t sync_seen;    /* la interrupcion recibio un encabezado de paquete */
static uint8_t          rx_prev;      /* byte anterior (para reconocer 0x5A + tipo)     */
static uint8_t          ext_idle;     /* cuadros desde la ultima transferencia externa */
static uint8_t          link_holdoff; /* espera al azar antes de pedir el reloj    */
static uint8_t          link_nosync;  /* cuadros dando el reloj sin oir 0x5A       */
static uint8_t          entropy = 0x5C; /* "azar": DIV (que corre libre) + botones   */

/* Numero de 0 a 255 distinto en cada consola, aunque se enciendan a la vez:
 * mezcla el divisor del reloj con el momento en que se tocaron los botones */
static uint8_t rnd8(void)
{
    entropy ^= DIV_REG;
    entropy  = (uint8_t)((entropy << 3) | (entropy >> 5));
    entropy  = (uint8_t)(entropy + 0x6D);
    return entropy;
}

/* Interrupcion del puerto serie: termino una transferencia de 8 bits */
static void link_isr(void)
{
    uint8_t b = SB_REG;                  /* lo que llego */
    uint8_t n = (uint8_t)((rx_in + 1) & RX_MASK);
    volatile uint8_t gap;

    if (n != rx_out) {                   /* si el buffer esta lleno, se pierde */
        rx_buf[rx_in] = b;
        rx_in = n;
    }
    /* Encabezado de paquete valido (0x5A + tipo conocido): hay alguien al otro lado */
    if (rx_prev == LINK_SYNC &&
        (b == LINK_HOLA || b == LINK_BOTONES || b == LINK_ESTADO))
        sync_seen = 1;
    rx_prev = b;
    entropy ^= b;                        /* si chocaron, la basura recibida desempata */

    /* Lo siguiente que se va a mandar queda listo en SB */
    if (tx_out != tx_in) {
        SB_REG = tx_buf[tx_out];
        tx_out = (uint8_t)((tx_out + 1) & TX_MASK);
    } else {
        SB_REG = LINK_IDLE;
    }

    if (link_master) {
        if (link_keep) {
            for (gap = LINK_GAP; gap; gap--)
                ;
            SC_REG = 0x81;               /* siguiente transferencia, reloj interno */
        } else {
            link_master = 0;             /* sin actividad: soltar el reloj */
            SC_REG = 0x80;
        }
    } else {
        ext_seen = 1;                    /* la otra consola nos dio el reloj */
        SC_REG = 0x80;                   /* volver a esperar reloj externo */
    }
}

void hal_link_send(uint8_t byte)
{
    uint8_t n = (uint8_t)((tx_pend + 1) & TX_MASK);
    link_keep = LINK_KEEP;
    if (n == tx_out) return;             /* lleno: se pierde este byte */
    tx_buf[tx_pend] = byte;
    tx_pend = n;
}

int hal_link_recv(void)
{
    uint8_t b;
    if (rx_out == rx_in) return -1;
    b = rx_buf[rx_out];
    rx_out = (uint8_t)((rx_out + 1) & RX_MASK);
    return b;
}

/* Una vez por cuadro (desde hal_frame) */
static void link_tick(void)
{
    /* Publica de una vez lo que el juego escribio en este cuadro */
    disable_interrupts();
    tx_in = tx_pend;
    enable_interrupts();

    if (ext_seen) { ext_seen = 0; ext_idle = 0; }
    else if (ext_idle < 255) ext_idle++;

    if (link_keep) link_keep--;

    if (link_master) {
        /* Dando el reloj: si nadie contesta con un paquete, soltarlo */
        if (sync_seen) {
            sync_seen   = 0;
            link_nosync = 0;
        } else if (++link_nosync > LINK_NOSYNC) {
            disable_interrupts();
            link_master = 0;
            SC_REG      = 0x80;          /* cancela la transferencia en curso */
            enable_interrupts();
            link_holdoff = (uint8_t)(8 + (rnd8() & 31));
        }
    } else {
        /* Esperando: si hay algo que mandar y la otra consola no nos esta
         * dando reloj, pedir el reloj despues de una espera al azar */
        if (!link_keep || ext_idle < 30)
            link_holdoff = (uint8_t)(2 + (rnd8() & 7));
        else if (link_holdoff)
            link_holdoff--;
        else {
            link_nosync = 0;
            sync_seen   = 0;
            disable_interrupts();
            link_master = 1;
            SC_REG      = 0x81;          /* arranca: reloj interno, 8192 Hz */
            enable_interrupts();
        }
    }
}

/* ------------------------------------------------------------------ */
/* Funciones de hal.h                                                  */
/* ------------------------------------------------------------------ */

void hal_init(void)
{
    uint8_t i;

    DISPLAY_OFF;                 /* LCD apagado mientras se cargan los graficos */

    /* No se confia en el estado inicial del hardware: en la Game Boy real el
     * BIOS deja LCDC en 0x91, pero un clon o un nucleo en FPGA puede arrancar
     * con otros valores (por ejemplo, el fondo leyendo los tiles de 0x8800 en
     * vez de 0x8000: los sprites se veran bien y el fondo, vacio). Aqui se fija
     * todo: fondo con tiles en 0x8000, mapa del fondo en 0x9800, sprites de 8x8,
     * ventana apagada. DISPLAY_ON y SHOW_* se activan despues, en hal_frame(). */
    LCDC_REG = LCDCF_BG8000 | LCDCF_BG9800 | LCDCF_OBJ8 | LCDCF_WINOFF;
    SCX_REG  = 0;
    SCY_REG  = 0;
    fill_bkg_rect(0, 0, 32, 32, 0);              /* mapa del fondo limpio (tile 0 = vacio) */
    for (i = 0; i < 40; i++) move_sprite(i, 0, 0);   /* los 40 sprites fuera de pantalla */
    BGP_REG  = 0xE4;             /* paleta del fondo:   0 claro ... 3 oscuro */
    OBP0_REG = 0xE4;             /* paleta de sprites:  igual */

    NR52_REG = 0x80;             /* enciende el sonido */
    NR51_REG = 0xFF;             /* todos los canales a los dos altavoces */
    NR50_REG = 0x77;             /* volumen general al maximo */

    /* Puerto serie: esperando reloj externo, con el interruptor de VBlank y
     * el del puerto serie activos */
    SB_REG = LINK_IDLE;
    SC_REG = 0x80;
    add_SIO(link_isr);
    set_interrupts(VBL_IFLAG | SIO_IFLAG);
}

void hal_shutdown(void)
{
    /* En la consola main() nunca termina */
}

void hal_load_tiles(uint8_t first, uint8_t count, const uint8_t *data)
{
    /* Fondo y sprites tienen memorias de tiles separadas en el rango 0..127:
     * se cargan los mismos graficos en las dos. */
    set_bkg_data(first, count, data);
    set_sprite_data(first, count, data);
}

void hal_set_bg(uint8_t tx, uint8_t ty, uint8_t tile)
{
    set_bkg_tile_xy(tx, ty, tile);
}

void hal_set_sprite(uint8_t id, int16_t x, int16_t y, uint8_t tile)
{
    set_sprite_tile(id, tile);

    /* La OAM suma 8 a X y 16 a Y. Si el sprite queda totalmente fuera de la
     * pantalla se oculta (los valores no caben en un byte). */
    if (x > -8 && x < SCREEN_W && y > -8 && y < SCREEN_H)
        move_sprite(id, (uint8_t)(x + 8), (uint8_t)(y + 16));
    else
        hide_sprite(id);
}

void hal_hide_sprite(uint8_t id)
{
    hide_sprite(id);
}

uint8_t hal_buttons(void)
{
    uint8_t j = joypad();
    uint8_t b = 0;

    if (j & J_UP)     b |= BTN_UP;
    if (j & J_DOWN)   b |= BTN_DOWN;
    if (j & J_LEFT)   b |= BTN_LEFT;
    if (j & J_RIGHT)  b |= BTN_RIGHT;
    if (j & J_A)      b |= BTN_A;
    if (j & J_B)      b |= BTN_B;
    if (j & J_START)  b |= BTN_START;
    if (j & J_SELECT) b |= BTN_SELECT;
    entropy = (uint8_t)(entropy + j);        /* los botones aportan azar */
    return b;
}

/*
 * Sonido: canal 2 de la Game Boy (onda cuadrada, sin barrido).
 * Registro de frecuencia: x = 2048 - 131072 / hz   (hz entre 64 y 131072)
 * hz = 0 -> silencio (se apaga el DAC del canal escribiendo 0 en NR22).
 */
void hal_tone(uint16_t hz)
{
    uint16_t x;

    if (hz == 0) {
        NR22_REG = 0x00;
        return;
    }
    x = (uint16_t)(2048u - 131072UL / hz);
    NR21_REG = 0x80;                                   /* ciclo de trabajo del 50 % */
    NR22_REG = 0xA0;                                   /* volumen 10 de 15, sin envolvente */
    NR23_REG = (uint8_t)(x & 0xFF);
    NR24_REG = (uint8_t)(0x80 | ((x >> 8) & 0x07));    /* bit 7 = reiniciar el canal */
}

int hal_frame(void)
{
    link_tick();                 /* publica lo que el juego envio en este cuadro */

    /* La primera vez: ya esta todo cargado, se enciende el LCD */
    if (!display_started) {
        SHOW_BKG;
        SHOW_SPRITES;
        DISPLAY_ON;
        display_started = 1;
    }
    vsync();                     /* espera el VBlank: 60 cuadros por segundo */
    return 1;
}
