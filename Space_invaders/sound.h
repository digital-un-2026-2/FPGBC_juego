#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

enum {
    SFX_NONE = 0,
    SFX_MENU,
    SFX_PLAYER_SHOT,
    SFX_SPECIAL_SHOT,
    SFX_ENEMY_SHOT,
    SFX_ENEMY_HIT,
    SFX_ENEMY_EXPLODE,
    SFX_BARRIER_HIT,
    SFX_PLAYER_EXPLODE,
    SFX_UFO,
    SFX_PAUSE,
    SFX_LEVEL_CLEAR,
    SFX_GAME_OVER,
    SFX_VICTORY
};

enum {
    MUS_NONE = 0,
    MUS_MENU,
    MUS_GAME,
    MUS_BOSS
};

void sound_init(void);
void sound_play_sfx(uint8_t id);
void sound_set_music(uint8_t track);
void sound_set_tempo(uint8_t frames_per_step);
void sound_set_paused(uint8_t paused);
void sound_update(void);

#endif
