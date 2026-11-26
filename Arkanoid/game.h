#ifndef GAME_H
#define GAME_H

#include <stdint.h>

enum GameState {
    STATE_TITLE = 0,
    STATE_LEVEL_INTRO,
    STATE_SERVE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_LIFE_LOST,
    STATE_LEVEL_CLEAR,
    STATE_GAME_OVER,
    STATE_VICTORY
};

#define PADDLE_W      32U
#define PADDLE_H      8U
#define PADDLE_Y      128U
#define PADDLE_MIN_X  8
#define PADDLE_MAX_X  120
#define PADDLE_SPEED  3

#define BALL_SIZE     6U
#define INIT_LIVES    3U

extern uint8_t  g_state;
extern uint8_t  g_level;
extern uint8_t  g_lives;
extern uint8_t  g_speed_tier;
extern uint16_t g_score;
extern int16_t  g_paddle_x;
extern int16_t  g_ball_x_fp;
extern int16_t  g_ball_y_fp;
extern int8_t   g_ball_vx_fp;
extern int8_t   g_ball_vy_fp;
extern uint8_t  g_ball_px;
extern uint8_t  g_ball_py;

void game_init(void);
void game_update(void);

#endif
