/*
 * hal_fpgbc.c - Implementacion de hal.h para la consola FPGBC (SoC LiteX + FemtoRV).
 *
 * Que funciona hoy:
 *  - Botones: lee el periferico game_buttons (GPIOIn de LiteX) que ya existe en
 *    FPGBC_digital_UN. Los botones son activos en bajo y vienen en otro orden
 *    que en hal.h, asi que aqui se invierten y se reordenan.
 *  - Ritmo de 60 cuadros por segundo con busy_wait_us() mientras no haya video.
 *  - Modo de depuracion por UART (HAL_UART_DEBUG): dibuja la pantalla con
 *    caracteres en la terminal (litex_term) para probar el juego en la FPGA
 *    ANTES de que exista el periferico de video.
 *
 * Que falta:
 *  - El periferico de video. Si el SoC define VIDEO_BASE (una region "video"
 *    en generated/mem.h), este archivo escribe en los registros propuestos en
 *    VIDEO.md. Si no existe, guarda todo en una copia en RAM.
 *
 * NOTA: este archivo no se ha podido compilar contra el SoC real (hace falta
 * el build de LiteX con los headers generated/). Revisar al primer intento.
 */
#include <stdio.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <generated/soc.h>
#include <system.h>
#include <irq.h>
#include <libbase/uart.h>

#include "hal.h"

/* ------------------------------------------------------------------ */
/* Copia en RAM de la "memoria de video"                               */
/* (siempre existe: la usa la depuracion por UART)                     */
/* ------------------------------------------------------------------ */
static uint8_t bgmap[MAP_H][MAP_W];
static struct { int16_t x, y; uint8_t tile, visible; } oam[MAX_SPRITES];

/* ------------------------------------------------------------------ */
/* Periferico de video (propuesto en VIDEO.md)                         */
/* ------------------------------------------------------------------ */
#ifdef VIDEO_BASE
#define VID_TILES    ((volatile uint8_t  *)(VIDEO_BASE + 0x0000))  /* 64 x 16 bytes   */
#define VID_BGMAP    ((volatile uint8_t  *)(VIDEO_BASE + 0x0400))  /* 32 x 32 bytes   */
#define VID_OAM      ((volatile uint32_t *)(VIDEO_BASE + 0x0800))  /* 16 x 1 palabra  */
#define VID_CTRL     (*(volatile uint32_t *)(VIDEO_BASE + 0x0C00))
#define VID_STATUS   (*(volatile uint32_t *)(VIDEO_BASE + 0x0C04))
#define VID_SCX      (*(volatile uint32_t *)(VIDEO_BASE + 0x0C08))
#define VID_SCY      (*(volatile uint32_t *)(VIDEO_BASE + 0x0C0C))

#define CTRL_BG_ON       0x1
#define CTRL_SPR_ON      0x2
#define STATUS_VBLANK_F  0x2   /* se pone en 1 al empezar el VBlank; se borra escribiendo 1 */
#define OAM_VISIBLE      0x80

static void vid_write_sprite(uint8_t id)
{
    uint32_t w = 0;
    if (oam[id].visible) {
        /* X e Y se guardan con desplazamiento (+8 / +16, como en la Game Boy)
         * para poder poner sprites parcialmente fuera por la izquierda/arriba. */
        w  = (uint32_t)(uint8_t)(oam[id].y + 16);
        w |= (uint32_t)(uint8_t)(oam[id].x + 8) << 8;
        w |= (uint32_t)oam[id].tile << 16;
        w |= (uint32_t)OAM_VISIBLE << 24;
    }
    VID_OAM[id] = w;
}
#endif

/* ------------------------------------------------------------------ */
/* Depuracion por UART: la pantalla en caracteres                      */
/* ------------------------------------------------------------------ */
#ifdef HAL_UART_DEBUG
#include "tiles.h"

#ifndef HAL_UART_DEBUG_EVERY
#define HAL_UART_DEBUG_EVERY 15     /* un dibujo cada 15 cuadros = 4 por segundo */
#endif

static char tile_char(uint8_t t)
{
    if (t >= TILE_LETTER_A && t < TILE_LETTER_A + 26) return (char)('A' + (t - TILE_LETTER_A));
    if (t >= TILE_DIGIT0 && t < TILE_DIGIT0 + 10)     return (char)('0' + (t - TILE_DIGIT0));
    switch (t) {
    case TILE_SOLID:  return '#';
    case TILE_WALL:   return '=';
    case TILE_BALL:   return 'o';
    case TILE_NET:    return '|';
    case TILE_CURSOR: return '>';
    default:          return ' ';
    }
}

static void uart_draw(void)
{
    static char pantalla[MAP_H][MAP_W + 1];
    uint8_t x, y, i;

    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++)
            pantalla[y][x] = tile_char(bgmap[y][x]);
        pantalla[y][MAP_W] = '\0';
    }
    /* Sprites: se ubican en la celda de 8x8 donde cae su centro */
    for (i = 0; i < MAX_SPRITES; i++) {
        int16_t cx = (int16_t)((oam[i].x + 4) >> 3);
        int16_t cy = (int16_t)((oam[i].y + 4) >> 3);
        if (!oam[i].visible || cx < 0 || cy < 0 || cx >= MAP_W || cy >= MAP_H) continue;
        pantalla[cy][cx] = tile_char(oam[i].tile);
    }

    fputs("\033[H", stdout);            /* cursor arriba a la izquierda */
    for (y = 0; y < MAP_H; y++) {
        fputs(pantalla[y], stdout);
        fputs("\n", stdout);
    }
}
#endif

/* ------------------------------------------------------------------ */
/* Funciones de hal.h                                                  */
/* ------------------------------------------------------------------ */

void hal_init(void)
{
    /* Igual que el demo de LiteX: interrupciones y UART listas para printf */
#ifdef CONFIG_CPU_HAS_INTERRUPT
    irq_setmask(0);
    irq_setie(1);
#endif
    uart_init();

#ifdef VIDEO_BASE
    VID_SCX    = 0;
    VID_SCY    = 0;
    VID_STATUS = STATUS_VBLANK_F;       /* borra una bandera vieja */
    VID_CTRL   = CTRL_BG_ON | CTRL_SPR_ON;
#endif
#ifdef HAL_UART_DEBUG
    fputs("\033[2J", stdout);           /* limpia la terminal */
#endif
}

void hal_shutdown(void)
{
    /* En la consola el juego nunca termina */
}

void hal_load_tiles(uint8_t first, uint8_t count, const uint8_t *data)
{
#ifdef VIDEO_BASE
    unsigned i;
    if ((unsigned)first + count > MAX_TILES) return;
    for (i = 0; i < (unsigned)count * 16; i++)
        VID_TILES[(unsigned)first * 16 + i] = data[i];
#else
    (void)first; (void)count; (void)data;
#endif
}

void hal_set_bg(uint8_t tx, uint8_t ty, uint8_t tile)
{
    if (tx >= MAP_W || ty >= MAP_H || tile >= MAX_TILES) return;
    bgmap[ty][tx] = tile;
#ifdef VIDEO_BASE
    VID_BGMAP[((unsigned)ty << 5) + tx] = tile;     /* mapa de 32 columnas */
#endif
}

void hal_set_sprite(uint8_t id, int16_t x, int16_t y, uint8_t tile)
{
    if (id >= MAX_SPRITES || tile >= MAX_TILES) return;
    oam[id].x = x;
    oam[id].y = y;
    oam[id].tile = tile;
    oam[id].visible = 1;
#ifdef VIDEO_BASE
    vid_write_sprite(id);
#endif
}

void hal_hide_sprite(uint8_t id)
{
    if (id >= MAX_SPRITES) return;
    oam[id].visible = 0;
#ifdef VIDEO_BASE
    vid_write_sprite(id);
#endif
}

uint8_t hal_buttons(void)
{
#ifdef CSR_GAME_BUTTONS_BASE
    /* Orden en el SoC (FPGBC_unal_platform.py, game_buttons_n):
     *   bit 0 A, 1 B, 2 START, 3 SELECT, 4 RIGHT, 5 UP, 6 DOWN, 7 LEFT
     * Activos en bajo (pull-up): 0 = presionado. */
    uint32_t in = ~game_buttons_in_read();
    uint8_t  b  = 0;

    if (in & (1u << 0)) b |= BTN_A;
    if (in & (1u << 1)) b |= BTN_B;
    if (in & (1u << 2)) b |= BTN_START;
    if (in & (1u << 3)) b |= BTN_SELECT;
    if (in & (1u << 4)) b |= BTN_RIGHT;
    if (in & (1u << 5)) b |= BTN_UP;
    if (in & (1u << 6)) b |= BTN_DOWN;
    if (in & (1u << 7)) b |= BTN_LEFT;
    return b;
#else
    /* El SoC se construyo sin --with-game-buttons */
    return 0;
#endif
}

int hal_frame(void)
{
#ifdef HAL_UART_DEBUG
    static uint8_t cuenta;
    if (++cuenta >= HAL_UART_DEBUG_EVERY) {
        cuenta = 0;
        uart_draw();
    }
#endif

#ifdef VIDEO_BASE
    /* Espera el inicio del siguiente VBlank y borra la bandera */
    while (!(VID_STATUS & STATUS_VBLANK_F))
        ;
    VID_STATUS = STATUS_VBLANK_F;
#else
    /* Sin video: aproximadamente 60 cuadros por segundo */
    busy_wait_us(16667);
#endif
    return 1;   /* en la consola nunca se sale */
}
