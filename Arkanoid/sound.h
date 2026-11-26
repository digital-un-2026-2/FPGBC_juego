#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

enum {
    SFX_NONE = 0,
    SFX_MENU,
    SFX_LAUNCH,
    SFX_PADDLE,
    SFX_WALL,
    SFX_BRICK_HIT,
    SFX_BRICK_BREAK,
    SFX_LOSE_LIFE,
    SFX_PAUSE,
    SFX_LEVEL_CLEAR,
    SFX_GAME_OVER,
    SFX_VICTORY
};

void sound_init(void);
void sound_play(uint8_t id);
void sound_update(void);

#endif
