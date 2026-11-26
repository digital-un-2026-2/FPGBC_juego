#include <gb/gb.h>
#include "sound.h"

struct Note {
    uint16_t freq;
    uint8_t  dur;
};

/* Registros APU precalculados: 2048 - 131072 / Hz */
#define N_REST 0x0000U
#define N_E3   0x04E6U
#define N_F3   0x0513U
#define N_FS3  0x053CU
#define N_G3   0x0563U
#define N_A3   0x05ACU
#define N_B3   0x05EDU
#define N_C4   0x060CU
#define N_D4   0x0642U
#define N_E4   0x0673U
#define N_F4   0x0689U
#define N_G4   0x06B2U
#define N_A4   0x06D6U
#define N_B4   0x06F7U
#define N_C5   0x0706U
#define N_D5   0x0721U
#define N_E5   0x0739U
#define N_G5   0x0759U
#define N_A5   0x076BU
#define N_C6   0x0783U

/* Pistas musicales (canal 2, en bucle) */
static const struct Note mus_menu_notes[] = {
    { N_C4, 10U }, { N_E4, 10U }, { N_G4, 10U }, { N_B4, 10U },
    { N_C5, 10U }, { N_B4, 10U }, { N_G4, 10U }, { N_E4, 10U },
    { N_A3, 10U }, { N_C4, 10U }, { N_E4, 10U }, { N_A4, 10U },
    { N_G4, 10U }, { N_E4, 10U }, { N_D4, 10U }, { N_REST, 10U }
};
#define MUS_MENU_LEN 16U

/* Pulso clasico Space Invaders de 4 notas graves */
static const uint16_t mus_game_bass[4] = {
    N_A3, N_G3, N_FS3, N_F3
};

/* Pista intensa para Jefe Final */
static const uint16_t mus_boss_notes[8] = {
    N_E4, N_G4, N_A4, N_G4, N_E4, N_B3, N_D4, N_E4
};

/* Fanfarrias de prioridad alta (canal 2) */
static const struct Note fanfare_clear[] = {
    { N_C5, 6U }, { N_E5, 6U }, { N_G5, 6U }, { N_C6, 18U }, { 0U, 0U }
};

static const struct Note fanfare_gameover[] = {
    { N_G4, 12U }, { N_F4, 12U }, { N_E4, 12U }, { N_C4, 28U }, { 0U, 0U }
};

static const struct Note fanfare_victory[] = {
    { N_C5, 6U }, { N_E5, 6U }, { N_G5, 6U }, { N_A5, 6U },
    { N_G5, 8U }, { N_C6, 26U }, { 0U, 0U }
};

static uint8_t ch1_prio;
static uint8_t ch1_timer;
static uint8_t ch4_prio;
static uint8_t ch4_timer;

static uint8_t mus_track;
static uint8_t mus_step;
static uint8_t mus_timer;
static uint8_t mus_tempo;
static uint8_t mus_paused;

static const struct Note *ch2_fanfare;
static uint8_t ch2_fan_timer;

static void trigger_ch1(uint8_t sweep, uint8_t duty, uint8_t env, uint16_t freq, uint8_t prio, uint8_t frames)
{
    if (ch1_timer > 0U && prio < ch1_prio) return;
    ch1_prio = prio;
    ch1_timer = frames;
    NR10_REG = sweep;
    NR11_REG = duty;
    NR12_REG = env;
    NR13_REG = (uint8_t)(freq & 0xFFU);
    NR14_REG = (uint8_t)(0x80U | ((freq >> 8) & 0x07U));
}

static void trigger_ch4(uint8_t len, uint8_t env, uint8_t poly, uint8_t prio, uint8_t frames)
{
    if (ch4_timer > 0U && prio < ch4_prio) return;
    ch4_prio = prio;
    ch4_timer = frames;
    NR41_REG = len;
    NR42_REG = env;
    NR43_REG = poly;
    NR44_REG = 0x80U;
}

static void play_ch2_note(uint16_t freq, uint8_t duty, uint8_t env)
{
    if (freq == N_REST) {
        NR22_REG = 0x00U;
        NR24_REG = 0x80U;
        return;
    }
    NR21_REG = duty;
    NR22_REG = env;
    NR23_REG = (uint8_t)(freq & 0xFFU);
    NR24_REG = (uint8_t)(0x80U | ((freq >> 8) & 0x07U));
}

static void start_fanfare(const struct Note *seq)
{
    ch2_fanfare = seq;
    ch2_fan_timer = 0U;
}

void sound_init(void)
{
    NR52_REG = 0x80U;
    NR51_REG = 0xFFU;
    NR50_REG = 0x77U;
    NR12_REG = 0x00U;
    NR22_REG = 0x00U;
    NR42_REG = 0x00U;

    ch1_prio = 0U;
    ch1_timer = 0U;
    ch4_prio = 0U;
    ch4_timer = 0U;

    mus_track = MUS_NONE;
    mus_step = 0U;
    mus_timer = 0U;
    mus_tempo = 20U;
    mus_paused = 0U;

    ch2_fanfare = 0;
    ch2_fan_timer = 0U;
}

void sound_set_music(uint8_t track)
{
    mus_track = track;
    mus_step = 0U;
    mus_timer = 0U;
    mus_paused = 0U;
    ch2_fanfare = 0;
    if (track == MUS_NONE) {
        NR22_REG = 0x00U;
        NR24_REG = 0x80U;
    }
}

void sound_set_tempo(uint8_t frames_per_step)
{
    if (frames_per_step < 6U) frames_per_step = 6U;
    if (frames_per_step > 32U) frames_per_step = 32U;
    mus_tempo = frames_per_step;
}

void sound_set_paused(uint8_t paused)
{
    mus_paused = paused;
    if (paused) {
        NR22_REG = 0x00U;
        NR24_REG = 0x80U;
    }
}

void sound_play_sfx(uint8_t id)
{
    switch (id) {
    case SFX_MENU:
        trigger_ch1(0x00U, 0x80U, 0x91U, N_G5, 1U, 4U);
        break;
    case SFX_ENEMY_SHOT:
        trigger_ch1(0x1CU, 0x40U, 0x81U, N_A4, 1U, 5U);
        break;
    case SFX_PLAYER_SHOT:
        trigger_ch1(0x1EU, 0x80U, 0xB2U, N_E5, 2U, 7U);
        break;
    case SFX_ENEMY_HIT:
        trigger_ch1(0x00U, 0x40U, 0xB2U, N_C5, 3U, 6U);
        break;
    case SFX_UFO:
        trigger_ch1(0x25U, 0x80U, 0xA3U, N_A5, 3U, 10U);
        break;
    case SFX_SPECIAL_SHOT:
        trigger_ch1(0x16U, 0xC0U, 0xD2U, N_C5, 4U, 12U);
        trigger_ch4(0x28U, 0x81U, 0x22U, 2U, 6U);
        break;
    case SFX_PAUSE:
        trigger_ch1(0x00U, 0x80U, 0xA2U, N_C5, 4U, 6U);
        break;
    case SFX_BARRIER_HIT:
        trigger_ch4(0x36U, 0x91U, 0x32U, 1U, 5U);
        break;
    case SFX_ENEMY_EXPLODE:
        trigger_ch1(0x2EU, 0x80U, 0xB2U, N_D4, 3U, 8U);
        trigger_ch4(0x24U, 0xC2U, 0x44U, 3U, 10U);
        break;
    case SFX_PLAYER_EXPLODE:
        trigger_ch1(0x3FU, 0x80U, 0xE4U, N_G4, 5U, 28U);
        trigger_ch4(0x00U, 0xE3U, 0x56U, 5U, 28U);
        break;
    case SFX_LEVEL_CLEAR:
        start_fanfare(fanfare_clear);
        break;
    case SFX_GAME_OVER:
        start_fanfare(fanfare_gameover);
        break;
    case SFX_VICTORY:
        start_fanfare(fanfare_victory);
        break;
    default:
        break;
    }
}

void sound_update(void)
{
    if (ch1_timer > 0U) {
        ch1_timer--;
        if (ch1_timer == 0U) ch1_prio = 0U;
    }
    if (ch4_timer > 0U) {
        ch4_timer--;
        if (ch4_timer == 0U) ch4_prio = 0U;
    }

    /* Canal 2: fanfarria tiene prioridad maxima sobre musica de fondo */
    if (ch2_fanfare) {
        if (ch2_fan_timer == 0U) {
            if (ch2_fanfare->dur == 0U) {
                ch2_fanfare = 0;
                NR22_REG = 0x00U;
                NR24_REG = 0x80U;
                return;
            }
            play_ch2_note(ch2_fanfare->freq, 0x80U, 0xB2U);
            ch2_fan_timer = ch2_fanfare->dur;
            ch2_fanfare++;
        }
        ch2_fan_timer--;
        return;
    }

    if (mus_paused || mus_track == MUS_NONE) return;

    if (mus_timer > 0U) {
        mus_timer--;
        return;
    }

    if (mus_track == MUS_MENU) {
        play_ch2_note(mus_menu_notes[mus_step].freq, 0x40U, 0x72U);
        mus_timer = mus_menu_notes[mus_step].dur;
        mus_step++;
        if (mus_step >= MUS_MENU_LEN) mus_step = 0U;
    } else if (mus_track == MUS_GAME) {
        play_ch2_note(mus_game_bass[mus_step], 0x80U, 0x81U);
        mus_timer = mus_tempo;
        mus_step = (uint8_t)((mus_step + 1U) & 3U);
    } else if (mus_track == MUS_BOSS) {
        play_ch2_note(mus_boss_notes[mus_step], 0x80U, 0x82U);
        mus_timer = 10U;
        mus_step = (uint8_t)((mus_step + 1U) & 7U);
    }
}
