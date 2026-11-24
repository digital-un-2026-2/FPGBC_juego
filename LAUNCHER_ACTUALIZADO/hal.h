#ifndef HAL_H
#define HAL_H

#include <stdint.h>

#define SCREEN_W    160
#define SCREEN_H    144
#define TILE_SIZE   8
#define MAP_W       (SCREEN_W / TILE_SIZE)
#define MAP_H       (SCREEN_H / TILE_SIZE)
#define MAX_TILES   64
#define MAX_SPRITES 16

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
void    hal_load_tiles(uint8_t first, uint8_t count, const uint8_t *data);
void    hal_set_bg(uint8_t tx, uint8_t ty, uint8_t tile);
void    hal_set_sprite(uint8_t id, int16_t x, int16_t y, uint8_t tile);
void    hal_hide_sprite(uint8_t id);
uint8_t hal_buttons(void);
int     hal_frame(void);

#endif