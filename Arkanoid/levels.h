#ifndef LEVELS_H
#define LEVELS_H

#include <stdint.h>

#define PLAY_LEFT     8U
#define PLAY_RIGHT    152U
#define PLAY_TOP      16U
#define PLAY_BOTTOM   144U

#define BRICK_COLS    9U
#define BRICK_ROWS    7U
#define BRICK_W       16U
#define BRICK_H       8U
#define BRICK_TOP_TY  3U
#define BRICK_TOP_Y   24U
#define NUM_LEVELS    3U

extern uint8_t brick_hp[BRICK_ROWS][BRICK_COLS];
extern uint8_t brick_type[BRICK_ROWS][BRICK_COLS];
extern uint8_t g_bricks_left;

void level_load(uint8_t level_idx);
const char *level_get_name(uint8_t level_idx);
void level_draw_playfield(void);
void level_draw_brick(uint8_t row, uint8_t col);
uint16_t level_hit_brick(uint8_t row, uint8_t col, uint8_t *destroyed);

#endif
