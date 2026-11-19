/*
 * hal.h - Capa de abstraccion de hardware.
 *
 * El juego (game.c) SOLO usa estas funciones. Nunca llama a raylib.
 *  - En el PC las implementa hal_pc.c (con raylib).
 *  - En la FPGBC las implementaras tu (hal_fpgbc.c) escribiendo en los
 *    registros de tus perifericos (VRAM de tiles, OAM, joypad, vblank).
 *
 * SUPUESTOS que debes ajustar a tu hardware:
 *  - Pantalla de 160x144 (como Game Boy).
 *  - Tiles de 8x8 pixeles, 2 bits por pixel (4 colores), 16 bytes por tile.
 *  - Un puerto serie (UART) en el conector de enlace para jugar 1 VS 1.
 */
#ifndef HAL_H
#define HAL_H

#include <stdint.h>

#define SCREEN_W    160
#define SCREEN_H    144
#define TILE_SIZE   8
#define MAP_W       (SCREEN_W / TILE_SIZE)   /* 20 tiles de ancho */
#define MAP_H       (SCREEN_H / TILE_SIZE)   /* 18 tiles de alto  */
#define MAX_TILES   64
#define MAX_SPRITES 16

/* Mascara de botones (un bit por boton, 1 = presionado) */
#define BTN_UP      0x01
#define BTN_DOWN    0x02
#define BTN_LEFT    0x04
#define BTN_RIGHT   0x08
#define BTN_A       0x10
#define BTN_B       0x20
#define BTN_START   0x40
#define BTN_SELECT  0x80

void    hal_init(void);
void    hal_shutdown(void);

/* Copia 'count' tiles (16 bytes c/u) a la memoria de tiles desde el indice 'first' */
void    hal_load_tiles(uint8_t first, uint8_t count, const uint8_t *data);

/* Fondo: pone el tile 'tile' en la celda (tx,ty) del mapa de 20x18 */
void    hal_set_bg(uint8_t tx, uint8_t ty, uint8_t tile);

/* Sprites (tipo OAM): posicion en pixeles de pantalla; el color 0 es transparente */
void    hal_set_sprite(uint8_t id, int16_t x, int16_t y, uint8_t tile);
void    hal_hide_sprite(uint8_t id);

uint8_t hal_buttons(void);

/* Puerto de enlace (cable entre dos consolas) para el modo 1 VS 1.
 * Se mueven bytes sueltos; el protocolo (paquetes) esta en link.c.
 *  - hal_link_send: envia un byte (no espera respuesta).
 *  - hal_link_recv: devuelve el siguiente byte recibido (0..255) o -1 si no
 *    hay ninguno. NUNCA se queda esperando.
 * En la FPGBC es el UART del SoC, que ya sale por el puerto GB Link (pines
 * 27 = Serial In y 28 = Serial Out de la FPGA). En el PC es UDP (link_pc.c). */
void    hal_link_send(uint8_t byte);
int     hal_link_recv(void);

/* Fin de cuadro: presenta la imagen y espera al siguiente VBlank (60 Hz).
 * Devuelve 0 si hay que salir (solo pasa en el PC al cerrar la ventana). */
int     hal_frame(void);

#endif
