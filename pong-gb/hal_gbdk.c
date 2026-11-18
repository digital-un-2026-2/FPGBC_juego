/*
 * hal_gbdk.c - HAL para Game Boy usando GBDK-2020.
 *
 * Implementa las mismas funciones de hal.h que hal_pc.c, pero con el
 * hardware real de la Game Boy. game.c, tiles.c y main.c NO cambian.
 *
 * Compilar con -DPLATFORM_GBDK (ver Makefile.gbdk): asi hal.h fija 160x144.
 *
 * Por que funciona directo:
 *  - El formato de tile de tiles.c (2bpp, 16 bytes, bit 7 = pixel izquierdo)
 *    es exactamente el de la Game Boy.
 *  - El color 0 de un sprite ya es transparente en el hardware.
 */
#include <gb/gb.h>
#include <stdint.h>
#include "hal.h"

static uint8_t display_started;

void hal_init(void)
{
    DISPLAY_OFF;                 /* LCD apagado mientras se cargan los graficos */
    SPRITES_8x8;                 /* sprites de 8x8 */
    BGP_REG  = 0xE4;             /* paleta del fondo:   0 claro ... 3 oscuro */
    OBP0_REG = 0xE4;             /* paleta de sprites:  igual */

    NR52_REG = 0x80;             /* enciende el sonido */
    NR51_REG = 0xFF;             /* todos los canales a los dos altavoces */
    NR50_REG = 0x77;             /* volumen general al maximo */
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
