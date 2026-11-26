#ifndef LEVELS_H
#define LEVELS_H

#include <stdint.h>

#define PLAY_LEFT      8
#define PLAY_RIGHT     152
#define PLAY_TOP       8
#define SHIELD_TOP_Y   104
#define SHIELD_BOT_Y   120
#define PLAYER_Y       124
#define INVADER_LOSE_Y 116

#define NUM_LEVELS     5U
#define MAX_ENEMIES    24U
#define SHIELD_COUNT   4U
#define SHIELD_CELLS   4U

enum EnemyType {
    EN_GRUNT = 0,
    EN_SHOOTER,
    EN_ARMORED,
    EN_DIVER
};

struct Enemy {
    int16_t x;
    int16_t y;
    uint8_t type;
    uint8_t hp;
    uint8_t active;
    uint8_t mode; /* 0 = formacion, 1 = picada individual */
    int8_t  vx;
    int8_t  vy;
};

struct Boss {
    int16_t x;
    int16_t y;
    int8_t  vx;
    uint8_t hp;
    uint8_t max_hp;
    uint8_t active;
    uint8_t flash_timer;
    uint8_t fire_cd;
};

struct LevelSpec {
    const char *name;
    uint8_t base_step_frames;
    uint8_t fire_interval;
    uint8_t aimed_shots;
    uint8_t allow_divers;
    uint8_t has_boss;
};

extern struct Enemy g_enemies[MAX_ENEMIES];
extern struct Boss  g_boss;
extern uint8_t      g_enemies_alive;
extern uint8_t      g_barrier_hp[SHIELD_COUNT][SHIELD_CELLS];

void level_load(uint8_t level_idx);
const struct LevelSpec *level_get_spec(uint8_t level_idx);
uint16_t level_enemy_points(uint8_t type);

void barriers_init(void);
void barriers_draw_all(void);
void barrier_draw_cell(uint8_t shield, uint8_t cell);
uint8_t barrier_check_hit(int16_t px, int16_t py, uint8_t w, uint8_t h);
void barriers_crush_area(int16_t ex, int16_t ey, uint8_t w, uint8_t h);

#endif
