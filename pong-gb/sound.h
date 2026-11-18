/*
 * sound.h - Efectos de sonido del Pong (portables).
 *
 * El juego solo dice QUE paso (sound_play(SND_HIT_P1), ...). sound.c se encarga
 * de reproducir la melodia, nota por nota, y le pide al hardware (hal_tone) la
 * frecuencia de cada nota. Asi el mismo codigo suena en el PC y en la consola.
 */
#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

enum {
    SND_MENU,        /* mover el cursor del menu            */
    SND_START,       /* empezar partida / revancha          */
    SND_SERVE,       /* sacar la bola                       */
    SND_WALL,        /* la bola rebota en una pared         */
    SND_HIT_P1,      /* golpe de la raqueta del jugador     */
    SND_HIT_P2,      /* golpe de la raqueta de la CPU       */
    SND_SPEEDUP,     /* la bola acelera                     */
    SND_PAUSE,       /* pausa                               */
    SND_RESUME,      /* continuar                           */
    SND_POINT_WIN,   /* el jugador anota                    */
    SND_POINT_LOSE,  /* la CPU anota                        */
    SND_WIN,         /* fin de partida: ganaste             */
    SND_LOSE,        /* fin de partida: perdiste            */
    SND_COUNT
};

void sound_init(void);
void sound_play(uint8_t id);   /* empieza un efecto (si no hay uno mas importante sonando) */
void sound_update(void);       /* llamar UNA vez por cuadro (60 veces por segundo)         */

#endif
