/*
 * test_color.c
 * 
 * Visualizador a pantalla completa de colores puros RGB (Game Boy Color / FPGBC).
 * Colores al maximo (31):
 *   1. Rojo
 *   2. Verde
 *   3. Azul
 *   4. Negro
 *   5. Blanco
 *   6. Rojo-Verde (Amarillo)
 *   7. Rojo-Azul (Magenta)
 *   8. Azul-Verde (Cian)
 * 
 * Control: START (o A) para cambiar al siguiente color.
 */

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

#define NUM_COLORES 8

static const uint16_t colores[NUM_COLORES] = {
    RGB(31, 0, 0),   /* Rojo */
    RGB(0, 31, 0),   /* Verde */
    RGB(0, 0, 31),   /* Azul */
    RGB(0, 0, 0),    /* Negro */
    RGB(31, 31, 31), /* Blanco */
    RGB(31, 31, 0),  /* Rojo-Verde */
    RGB(31, 0, 31),  /* Rojo-Azul */
    RGB(0, 31, 31)   /* Azul-Verde */
};

static void aplicar_color(uint8_t idx) {
    uint16_t pal[4];
    uint8_t i;
    for (i = 0; i < 4; i++) {
        pal[i] = colores[idx];
    }
    for (i = 0; i < 8; i++) {
        set_bkg_palette(i, 1, pal);
    }
}

void main(void) {
    uint8_t color_idx = 0;
    uint8_t cur_joy, prev_joy = 0;

    DISPLAY_OFF;
    HIDE_SPRITES;
    HIDE_WIN;
    SHOW_BKG;

    aplicar_color(color_idx);

    DISPLAY_ON;

    while (1) {
        cur_joy = joypad();

        if (((cur_joy & J_START) && !(prev_joy & J_START)) ||
            ((cur_joy & J_A) && !(prev_joy & J_A))) {
            color_idx = (uint8_t)((color_idx + 1) % NUM_COLORES);
            aplicar_color(color_idx);
        }

        prev_joy = cur_joy;
        vsync();
    }
}
