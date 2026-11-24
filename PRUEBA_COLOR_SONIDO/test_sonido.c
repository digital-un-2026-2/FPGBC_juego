/*
 * test_sonido.c
 * 
 * Generador de tono constante con menu de dos opciones:
 *   - REPRODUCIR
 *   - PARAR
 * 
 * Cada opcion se selecciona con START (o A).
 * El cursor se mueve con la cruceta (ARRIBA / ABAJO).
 */

#include <gb/gb.h>
#include <stdint.h>

#define C3(b) (b), (b)

#define TILE_EMPTY   0
#define TILE_ARROW_R 1 /* '>' */
#define TILE_COLON   2 /* ':' */
#define TILE_LET_A   3 /* 3..28: 'A'..'Z' */
#define TOTAL_TILES  29

static const uint8_t font_tiles[TOTAL_TILES * 16] = {
    /* 0: Vacio */
    C3(0x00), C3(0x00), C3(0x00), C3(0x00), C3(0x00), C3(0x00), C3(0x00), C3(0x00),

    /* 1: '>' */
    C3(0x30), C3(0x18), C3(0x0C), C3(0x06), C3(0x0C), C3(0x18), C3(0x30), C3(0x00),

    /* 2: ':' */
    C3(0x00), C3(0x18), C3(0x18), C3(0x00), C3(0x18), C3(0x18), C3(0x00), C3(0x00),

    /* 3..28: Letras A..Z */
    C3(0x3C), C3(0x66), C3(0x66), C3(0x7E), C3(0x66), C3(0x66), C3(0x66), C3(0x00), /* A */
    C3(0x7C), C3(0x66), C3(0x66), C3(0x7C), C3(0x66), C3(0x66), C3(0x7C), C3(0x00), /* B */
    C3(0x3C), C3(0x66), C3(0x60), C3(0x60), C3(0x60), C3(0x66), C3(0x3C), C3(0x00), /* C */
    C3(0x78), C3(0x6C), C3(0x66), C3(0x66), C3(0x66), C3(0x6C), C3(0x78), C3(0x00), /* D */
    C3(0x7E), C3(0x60), C3(0x60), C3(0x7C), C3(0x60), C3(0x60), C3(0x7E), C3(0x00), /* E */
    C3(0x7E), C3(0x60), C3(0x60), C3(0x7C), C3(0x60), C3(0x60), C3(0x60), C3(0x00), /* F */
    C3(0x3C), C3(0x66), C3(0x60), C3(0x6E), C3(0x66), C3(0x66), C3(0x3E), C3(0x00), /* G */
    C3(0x66), C3(0x66), C3(0x66), C3(0x7E), C3(0x66), C3(0x66), C3(0x66), C3(0x00), /* H */
    C3(0x7E), C3(0x18), C3(0x18), C3(0x18), C3(0x18), C3(0x18), C3(0x7E), C3(0x00), /* I */
    C3(0x1E), C3(0x06), C3(0x06), C3(0x06), C3(0x66), C3(0x66), C3(0x3C), C3(0x00), /* J */
    C3(0x66), C3(0x6C), C3(0x78), C3(0x70), C3(0x78), C3(0x6C), C3(0x66), C3(0x00), /* K */
    C3(0x60), C3(0x60), C3(0x60), C3(0x60), C3(0x60), C3(0x60), C3(0x7E), C3(0x00), /* L */
    C3(0xC6), C3(0xEE), C3(0xFE), C3(0xD6), C3(0xC6), C3(0xC6), C3(0xC6), C3(0x00), /* M */
    C3(0x66), C3(0x76), C3(0x7E), C3(0x7E), C3(0x6E), C3(0x66), C3(0x66), C3(0x00), /* N */
    C3(0x3C), C3(0x66), C3(0x66), C3(0x66), C3(0x66), C3(0x66), C3(0x3C), C3(0x00), /* O */
    C3(0x7C), C3(0x66), C3(0x66), C3(0x7C), C3(0x60), C3(0x60), C3(0x60), C3(0x00), /* P */
    C3(0x3C), C3(0x66), C3(0x66), C3(0x66), C3(0x6E), C3(0x6C), C3(0x36), C3(0x00), /* Q */
    C3(0x7C), C3(0x66), C3(0x66), C3(0x7C), C3(0x78), C3(0x6C), C3(0x66), C3(0x00), /* R */
    C3(0x3C), C3(0x66), C3(0x60), C3(0x3C), C3(0x06), C3(0x66), C3(0x3C), C3(0x00), /* S */
    C3(0x7E), C3(0x18), C3(0x18), C3(0x18), C3(0x18), C3(0x18), C3(0x18), C3(0x00), /* T */
    C3(0x66), C3(0x66), C3(0x66), C3(0x66), C3(0x66), C3(0x66), C3(0x3C), C3(0x00), /* U */
    C3(0x66), C3(0x66), C3(0x66), C3(0x66), C3(0x66), C3(0x3C), C3(0x18), C3(0x00), /* V */
    C3(0xC6), C3(0xC6), C3(0xC6), C3(0xD6), C3(0xFE), C3(0xEE), C3(0xC6), C3(0x00), /* W */
    C3(0x66), C3(0x66), C3(0x3C), C3(0x18), C3(0x3C), C3(0x66), C3(0x66), C3(0x00), /* X */
    C3(0x66), C3(0x66), C3(0x66), C3(0x3C), C3(0x18), C3(0x18), C3(0x18), C3(0x00), /* Y */
    C3(0x7E), C3(0x06), C3(0x0C), C3(0x18), C3(0x30), C3(0x60), C3(0x7E), C3(0x00)  /* Z */
};

static uint8_t char_to_tile(char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)(TILE_LET_A + (c - 'A'));
    if (c >= 'a' && c <= 'z') return (uint8_t)(TILE_LET_A + (c - 'a'));
    if (c == '>') return TILE_ARROW_R;
    if (c == ':') return TILE_COLON;
    return TILE_EMPTY;
}

static void draw_text(uint8_t x, uint8_t y, const char *str) {
    while (*str) {
        set_bkg_tile_xy(x, y, char_to_tile(*str));
        x++;
        str++;
    }
}

static void iniciar_tono_constante(void) {
    NR21_REG = 0x80; /* Duty 50% */
    NR22_REG = 0xF0; /* Vol 15, sin decaimiento (continuo indefinido) */
    NR23_REG = 0xD6; /* Tono constante (~440 Hz) */
    NR24_REG = 0x86; /* Trigger continuo sin limite de tiempo */
}

static void silenciar_tono(void) {
    NR22_REG = 0x00; /* Silenciar canal 2 */
    NR24_REG = 0x80;
}

static void actualizar_cursor(uint8_t opcion) {
    /* Opcion 0 = Fila 6 (REPRODUCIR), Opcion 1 = Fila 8 (PARAR) */
    set_bkg_tile_xy(3, 6, (opcion == 0) ? TILE_ARROW_R : TILE_EMPTY);
    set_bkg_tile_xy(3, 8, (opcion == 1) ? TILE_ARROW_R : TILE_EMPTY);
}

void main(void) {
    uint8_t opcion = 0; /* 0: REPRODUCIR, 1: PARAR */
    uint8_t cur_joy, prev_joy = 0;
    uint8_t x, y;

    DISPLAY_OFF;
    move_bkg(0, 0);

    BGP_REG = 0xE4;

    /* Cargar tiles de fuente */
    set_bkg_data(0, TOTAL_TILES, font_tiles);

    /* Limpiar pantalla */
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, TILE_EMPTY);
        }
    }

    /* Dibujar texto en pantalla */
    draw_text(4, 3, "TEST SONIDO");
    draw_text(5, 6, "REPRODUCIR");
    draw_text(5, 8, "PARAR");
    draw_text(1, 14, "START: SELECCIONAR");

    /* Inicializar hardware APU de audio */
    NR52_REG = 0x80; /* APU ON */
    NR50_REG = 0x77; /* Volumen maximo */
    NR51_REG = 0xFF; /* Paneo estereo completo */

    actualizar_cursor(opcion);

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        cur_joy = joypad();

        /* Cambiar entre REPRODUCIR y PARAR con ARRIBA o ABAJO */
        if (((cur_joy & J_UP) && !(prev_joy & J_UP)) ||
            ((cur_joy & J_DOWN) && !(prev_joy & J_DOWN))) {
            opcion = 1 - opcion;
            actualizar_cursor(opcion);
        }

        /* Seleccionar con START (o A) */
        if (((cur_joy & J_START) && !(prev_joy & J_START)) ||
            ((cur_joy & J_A) && !(prev_joy & J_A))) {
            if (opcion == 0) {
                iniciar_tono_constante();
            } else {
                silenciar_tono();
            }
        }

        prev_joy = cur_joy;
        vsync();
    }
}
