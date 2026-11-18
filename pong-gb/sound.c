/*
 * sound.c - Secuenciador de efectos de sonido.
 *
 * Solo enteros, sin malloc: sirve en el RV32I del FemtoRV.
 * Cada efecto es una lista de notas {frecuencia en Hz, duracion en cuadros}
 * que termina con {0, 0}. Una nota con hz = 0 es un silencio.
 * A 60 cuadros por segundo, 6 cuadros = 0.1 s.
 *
 * El HAL solo necesita un canal de onda cuadrada: hal_tone(hz), con 0 = silencio.
 */
#include "sound.h"
#include "hal.h"

struct Note   { uint16_t hz; uint8_t frames; };
struct Effect { const struct Note *notes; uint8_t prio; };

/* Notas (Hz) */
#define N_C4  262
#define N_E4  330
#define N_F4  349
#define N_G4  392
#define N_A4  440
#define N_C5  523
#define N_E5  659
#define N_G5  784
#define N_B5  988
#define N_C6 1047
#define N_E6 1319

static const struct Note s_menu[]       = { {880, 3}, {0, 0} };
static const struct Note s_start[]      = { {N_C5, 4}, {N_E5, 4}, {N_G5, 7}, {0, 0} };
static const struct Note s_serve[]      = { {N_E5, 5}, {0, 0} };
static const struct Note s_wall[]       = { {N_E4, 3}, {0, 0} };
static const struct Note s_hit_p1[]     = { {N_G5, 4}, {0, 0} };
static const struct Note s_hit_p2[]     = { {N_E5, 4}, {0, 0} };
static const struct Note s_speedup[]    = { {N_B5, 3}, {N_E6, 6}, {0, 0} };
static const struct Note s_pause[]      = { {N_A4, 4}, {N_E4, 7}, {0, 0} };
static const struct Note s_resume[]     = { {N_E4, 4}, {N_A4, 7}, {0, 0} };
static const struct Note s_point_win[]  = { {N_C5, 5}, {N_E5, 5}, {N_G5, 9}, {0, 0} };
static const struct Note s_point_lose[] = { {N_G4, 6}, {N_C4, 12}, {0, 0} };
static const struct Note s_win[]        = { {N_C5, 7}, {N_E5, 7}, {N_G5, 7}, {N_C6, 20}, {0, 0} };
static const struct Note s_lose[]       = { {N_G4, 9}, {N_F4, 9}, {N_E4, 9}, {N_C4, 24}, {0, 0} };

/* Prioridad: un efecto nuevo solo corta al que suena si su prioridad es igual o mayor */
static const struct Effect effects[SND_COUNT] = {
    /* SND_MENU       */ { s_menu,       1 },
    /* SND_START      */ { s_start,      3 },
    /* SND_SERVE      */ { s_serve,      2 },
    /* SND_WALL       */ { s_wall,       1 },
    /* SND_HIT_P1     */ { s_hit_p1,     2 },
    /* SND_HIT_P2     */ { s_hit_p2,     2 },
    /* SND_SPEEDUP    */ { s_speedup,    3 },
    /* SND_PAUSE      */ { s_pause,      3 },
    /* SND_RESUME     */ { s_resume,     3 },
    /* SND_POINT_WIN  */ { s_point_win,  4 },
    /* SND_POINT_LOSE */ { s_point_lose, 4 },
    /* SND_WIN        */ { s_win,        5 },
    /* SND_LOSE       */ { s_lose,       5 },
};

static const struct Note *cur;   /* proxima nota a tocar; 0 = nada sonando */
static uint8_t cur_prio;
static uint8_t left;             /* cuadros que le quedan a la nota actual */

void sound_init(void)
{
    cur      = 0;
    cur_prio = 0;
    left     = 0;
    hal_tone(0);
}

void sound_play(uint8_t id)
{
    if (id >= SND_COUNT) return;
    if (cur && effects[id].prio < cur_prio) return;   /* suena algo mas importante */
    cur      = effects[id].notes;
    cur_prio = effects[id].prio;
    left     = 0;                                      /* la primera nota arranca en el proximo update */
}

void sound_update(void)
{
    if (!cur) return;

    if (left == 0) {                                   /* toca pasar a la siguiente nota */
        if (cur->frames == 0) {                        /* fin del efecto */
            cur      = 0;
            cur_prio = 0;
            hal_tone(0);
            return;
        }
        hal_tone(cur->hz);                             /* el HAL solo se toca al cambiar de nota */
        left = cur->frames;
        cur++;
    }
    left--;
}
