#include <gb/gb.h>
#include <stdint.h>
#include "mario.h"
#include "game.h"
#include "hal.h"
#include "tiles.h"

#define MENU_OPT_MARIO 0
#define MENU_OPT_PONG  1

static uint8_t char_to_tile(char c)
{
    if (c >= 'A' && c <= 'Z') return (uint8_t)(TILE_LETTER_A + (c - 'A'));
    if (c >= '0' && c <= '9') return (uint8_t)(TILE_DIGIT0 + (c - '0'));
    if (c == ':') return TILE_COLON;
    if (c == '>') return TILE_CURSOR;
    return TILE_EMPTY;
}

static void menu_print(uint8_t x, uint8_t y, const char *str)
{
    while (*str && x < 20) {
        set_bkg_tile_xy(x, y, char_to_tile(*str));
        x++;
        str++;
    }
}

static void menu_clear_screen(void)
{
    uint8_t x, y;
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, TILE_EMPTY);
        }
    }
}

static void menu_draw(uint8_t selected)
{
    menu_clear_screen();

    /* Titulo centrado */
    menu_print(5, 3, "FPGBC GAME");

    /* Opciones */
    menu_print(7, 7, "MARIO");
    menu_print(7, 9, "PONG");

    /* Cursor '>' */
    set_bkg_tile_xy(5, 7, (selected == MENU_OPT_MARIO) ? TILE_CURSOR : TILE_EMPTY);
    set_bkg_tile_xy(5, 9, (selected == MENU_OPT_PONG)  ? TILE_CURSOR : TILE_EMPTY);

    /* Mensaje de accion */
    menu_print(3, 13, "A: SELECCIONAR");
}

static void menu_init(uint8_t selected)
{
    uint8_t i;
    DISPLAY_OFF;
    move_bkg(0, 0);
    SPRITES_8x8;
    BGP_REG  = 0xE4;
    OBP0_REG = 0xE4;
    OBP1_REG = 0xE4;

    /* Ocultar los 40 sprites */
    for (i = 0; i < 40; i++) {
        hide_sprite(i);
    }

    /* Cargar tiles del menu desde game_tiles (fuente, cursor, colon) */
    set_bkg_data(0, TILE_COUNT, game_tiles);

    menu_draw(selected);

    SHOW_BKG;
    HIDE_SPRITES;
    DISPLAY_ON;
}

int main(void)
{
    uint8_t selected = MENU_OPT_MARIO;
    uint8_t prev_joy = 0;

    menu_init(selected);

    while (1) {
        uint8_t cur_joy = joypad();
        uint8_t pressed = (uint8_t)(cur_joy & (uint8_t)~prev_joy);
        prev_joy = cur_joy;

        /* Navegacion con la cruceta (Arriba / Abajo) */
        if (pressed & (J_UP | J_DOWN)) {
            selected = (selected == MENU_OPT_MARIO) ? MENU_OPT_PONG : MENU_OPT_MARIO;
            set_bkg_tile_xy(5, 7, (selected == MENU_OPT_MARIO) ? TILE_CURSOR : TILE_EMPTY);
            set_bkg_tile_xy(5, 9, (selected == MENU_OPT_PONG)  ? TILE_CURSOR : TILE_EMPTY);
        }

        /* Seleccion con boton A o START */
        if (pressed & (J_A | J_START)) {
            waitpadup();
            if (selected == MENU_OPT_MARIO) {
                run_mario();
            } else {
                run_pong();
            }

            /* Al regresar de Mario o Pong, restaurar el menu principal */
            menu_init(selected);
            prev_joy = joypad();
        }

        vsync();
    }
}
