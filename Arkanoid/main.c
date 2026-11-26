#include <gb/gb.h>
#include <gb/cgb.h>
#include "game.h"
#include "sound.h"
#include "tiles.h"

int main(void)
{
    DISPLAY_OFF;
    move_bkg(0U, 0U);
    SPRITES_8x8;
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
