#include <gb/gb.h>
#include "sound.h"

struct SfxNote {
    uint16_t freq_reg;
    uint8_t frames;
};

/* Registros de frecuencia APU precalculados: 2048 - 131072 / Hz */
#define F_REST 0x0000U
#define F_C4   0x060CU
#define F_E4   0x0673U
#define F_F4   0x0689U
#define F_G4   0x06B2U
#define F_A4   0x06D6U
#define F_B4   0x06F7U
#define F_C5   0x0706U
#define F_D5   0x0721U
#define F_E5   0x0739U
#define F_G5   0x0759U
#define F_C6   0x0783U

static const struct SfxNote seq_pause[] = {
    { F_E5, 4U }, { F_C5, 5U }, { 0U, 0U }
};

static const struct SfxNote seq_level_clear[] = {
    { F_C5, 6U }, { F_E5, 6U }, { F_G5, 6U }, { F_C6, 16U }, { 0U, 0U }
};

static const struct SfxNote seq_game_over[] = {
    { F_G4, 10U }, { F_F4, 10U }, { F_E4, 10U }, { F_C4, 24U }, { 0U, 0U }
};

static const struct SfxNote seq_victory[] = {
    { F_C5, 6U }, { F_E5, 6U }, { F_G5, 6U }, { F_C5, 6U },
    { F_G5, 8U }, { F_C6, 22U }, { 0U, 0U }
};

static const struct SfxNote *seq_cur;
static uint8_t seq_frames;

static void start_seq(const struct SfxNote *notes)
{
    seq_cur = notes;
    seq_frames = 0U;
}

static void play_ch1(uint8_t sweep, uint8_t duty_len, uint8_t env, uint16_t freq)
{
    NR10_REG = sweep;
    NR11_REG = duty_len;
    NR12_REG = env;
    NR13_REG = (uint8_t)(freq & 0xFFU);
    NR14_REG = (uint8_t)(0x80U | ((freq >> 8) & 0x07U));
}

void sound_init(void)
{
    NR52_REG = 0x80U;
    NR51_REG = 0xFFU;
    NR50_REG = 0x77U;
    NR12_REG = 0x00U;
    NR22_REG = 0x00U;
    NR42_REG = 0x00U;
    seq_cur = 0;
    seq_frames = 0U;
}

void sound_play(uint8_t id)
{
    switch (id) {
    case SFX_MENU:
        play_ch1(0x00U, 0x80U, 0x91U, F_G5);
        break;
    case SFX_LAUNCH:
        play_ch1(0x16U, 0x80U, 0xC2U, F_C5);
        break;
    case SFX_PADDLE:
        play_ch1(0x00U, 0x80U, 0xC2U, F_C5);
        break;
    case SFX_WALL:
        play_ch1(0x00U, 0x40U, 0x81U, F_E5);
        break;
    case SFX_BRICK_HIT:
        play_ch1(0x1CU, 0x40U, 0xB2U, F_A4);
        break;
    case SFX_BRICK_BREAK:
        play_ch1(0x14U, 0x80U, 0xB2U, F_E5);
        NR41_REG = 0x34U;
        NR42_REG = 0x91U;
        NR43_REG = 0x22U;
        NR44_REG = 0xC0U;
        break;
    case SFX_LOSE_LIFE:
        play_ch1(0x2FU, 0x80U, 0xD3U, F_G4);
        NR41_REG = 0x20U;
        NR42_REG = 0xA2U;
        NR43_REG = 0x44U;
        NR44_REG = 0xC0U;
        break;
    case SFX_PAUSE:
        start_seq(seq_pause);
        break;
    case SFX_LEVEL_CLEAR:
        start_seq(seq_level_clear);
        break;
    case SFX_GAME_OVER:
        start_seq(seq_game_over);
        break;
    case SFX_VICTORY:
        start_seq(seq_victory);
        break;
    default:
        break;
    }
}

void sound_update(void)
{
    if (!seq_cur) return;

    if (seq_frames == 0U) {
        if (seq_cur->frames == 0U) {
            seq_cur = 0;
            NR22_REG = 0x00U;
            NR24_REG = 0x80U;
            return;
        }
        if (seq_cur->freq_reg == F_REST) {
            NR22_REG = 0x00U;
            NR24_REG = 0x80U;
        } else {
            NR21_REG = 0x80U;
            NR22_REG = 0xB2U;
            NR23_REG = (uint8_t)(seq_cur->freq_reg & 0xFFU);
            NR24_REG = (uint8_t)(0x80U | ((seq_cur->freq_reg >> 8) & 0x07U));
        }
        seq_frames = seq_cur->frames;
        seq_cur++;
    }
    seq_frames--;
}
