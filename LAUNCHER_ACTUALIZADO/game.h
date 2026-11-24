#ifndef GAME_H
#define GAME_H

#include <stdint.h>

void game_init(void);
void game_update(void);
uint8_t game_is_running(void);
void run_pong(void);

#endif
