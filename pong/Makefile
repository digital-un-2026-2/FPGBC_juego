# Makefile para Windows (MinGW-w64 / w64devkit) con raylib
#
# RAYLIB = carpeta que contiene "include" y "lib" de raylib.
#   Opcion A (la mas simple): copiar las carpetas include y lib junto a este Makefile -> RAYLIB = .
#   Opcion B (instalador de raylib): RAYLIB = C:/raylib/raylib
#
# Objetivos:
#   make         -> pong.exe (simulador con ventana, necesita raylib)
#   make test    -> pruebas automaticas sin ventana (NO necesita raylib)
#   make clean
#
# Para la consola FPGBC se usa el otro Makefile:  make -f Makefile.fpgbc
RAYLIB = .

CC           = gcc
CFLAGS_EXTRA =
CFLAGS       = -std=c99 -Wall -Wextra -O2 -I$(RAYLIB)/include $(CFLAGS_EXTRA)
LDFLAGS      = -L$(RAYLIB)/lib
LIBS         = -lraylib -lopengl32 -lgdi32 -lwinmm
GAME_SRC     = game.c tiles.c
SRC          = main.c $(GAME_SRC) hal_pc.c

pong.exe: $(SRC) hal.h game.h tiles.h
	$(CC) $(CFLAGS) -o pong.exe $(SRC) $(LDFLAGS) $(LIBS)

# Pruebas: compila el juego con un HAL en memoria y revisa menu, pausa, fin de partida...
# Deja capturas .ppm en pruebas/capturas (se abren con GIMP, IrfanView, etc.)
pruebas/capturas:
	mkdir -p pruebas/capturas

test: pruebas/capturas
	$(CC) -std=c99 -Wall -Wextra -O2 -o pruebas/pong_test pruebas/hal_test.c $(GAME_SRC)
	./pruebas/pong_test

clean:
	rm -f pong.exe pruebas/pong_test pruebas/pong_test.exe

.PHONY: test clean
