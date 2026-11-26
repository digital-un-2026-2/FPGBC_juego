#include <gb/gb.h>
#include <gb/cgb.h>
#include "game.h"
#include "sound.h"
#include "tiles.h"

int main(void)
{
    DISPLAY_OFF;
    LCDC_REG = LCDCF_BG8800 | LCDCF_BG9800 | LCDCF_OBJ8 | LCDCF_WINOFF;
    SCX_REG  = 0U;
    SCY_REG  = 0U;
    VBK_REG  = 0U;
    BGP_REG  = 0xE4U;
    OBP0_REG = 0xE4U;
    OBP1_REG = 0xD2U;

    gfx_init();
    sound_init();
    game_init();

    while (1) {
        vsync();
        game_update();
        sound_update();
    }
}
