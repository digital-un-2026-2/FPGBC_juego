GBDK_HOME ?= C:/gbdk/

LCC   = $(GBDK_HOME)bin/lcc
SRC   = main.c mario.c game.c tiles.c hal_gbdk.c
FLAGS = -DPLATFORM_GBDK -Wm-yn"FPGBC_GAME" -Wa-l -Wl-m -Wl-j

all: FPGBC_LAUNCHER_ACTUALIZADO.gb

FPGBC_LAUNCHER_ACTUALIZADO.gb: $(SRC) mario.h game.h tiles.h hal.h
	$(LCC) $(FLAGS) -o FPGBC_LAUNCHER_ACTUALIZADO.gb $(SRC)

clean:
	rm -f FPGBC_LAUNCHER_ACTUALIZADO.gb *.o *.lst *.map *.sym *.asm *.ihx *.noi
