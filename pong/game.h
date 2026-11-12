#ifndef GAME_H
#define GAME_H

void game_init(void);    /* carga tiles, dibuja el fondo, deja todo listo */
void game_update(void);  /* un cuadro de logica (se llama 60 veces por segundo) */

#endif
