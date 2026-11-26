#ifndef GAME_H
#define GAME_H

#include <stdint.h>

enum GameState {
    STATE_TITLE = 0,
    STATE_HIGH_SCORE,
    STATE_CONTROLS,
    STATE_LEVEL_INTRO,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_PLAYER_HIT,
    STATE_LEVEL_CLEAR,
    STATE_GAME_OVER,
    STATE_VICTORY
};

#define PLAYER_MIN_X  8
#define PLAYER_MAX_X  136
#define PLAYER_W      16U
#define PLAYER_H      8U
#define INIT_LIVES    3U

#define MAX_P_SHOTS   3U
#define MAX_E_SHOTS   3U

struct Projectile {
    int16_t x;
    int16_t y;
    int8_t  vx;
    int8_t  vy;
    uint8_t active;
    uint8_t dmg;
    uint8_t is_special;
};

extern uint8_t  g_state;
extern uint8_t  g_level;
extern uint8_t  g_lives;
extern uint16_t g_score;
extern uint16_t g_high_score;
extern uint8_t  g_max_wave_reached;
extern int16_t  g_player_x;
extern uint8_t  g_player_speed;
extern uint8_t  g_player_invuln;
extern uint8_t  g_special_charge;
extern uint8_t  g_ufo_active;
extern int16_t  g_ufo_x;
extern struct Projectile g_p_shots[MAX_P_SHOTS];
extern struct Projectile g_e_shots[MAX_E_SHOTS];

void game_init(void);
void game_update(void);

#endif
