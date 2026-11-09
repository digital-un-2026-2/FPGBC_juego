#include "hal.h"
#include "game.h"

int main(void)
{
    hal_init();
    game_init();

    do {
        game_update();
    } while (hal_frame());   /* hal_frame() dibuja y espera el VBlank */

    hal_shutdown();
    return 0;
}
