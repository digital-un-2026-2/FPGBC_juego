# Makefile para Windows (MinGW-w64 / w64devkit) con raylib
#
# RAYLIB = carpeta que contiene "include" y "lib" de raylib.
#   Opcion A (la mas simple): copiar las carpetas include y lib junto a este Makefile -> RAYLIB = .
#   Opcion B (instalador de raylib): RAYLIB = C:/raylib/raylib
#
# Objetivos:
#   make         -> pong.exe (simulador con ventana, necesita raylib)
#   make test    -> pruebas automaticas sin ventana (NO necesita raylib):
#                   un jugador (hal_test.c) y 1 VS 1 con dos consolas (link_test.c)
#   make clean
#
# 1 VS 1 en el PC: abrir pong.exe DOS veces y elegir "1 VS 1" en las dos
# ventanas (se conectan por UDP en este mismo PC, ver link_pc.c).
#
# Para la consola FPGBC se usa el otro Makefile:  make -f Makefile.fpgbc
RAYLIB = .

CC           = gcc
CFLAGS_EXTRA =
CFLAGS       = -std=c99 -Wall -Wextra -O2 -I$(RAYLIB)/include $(CFLAGS_EXTRA)
LDFLAGS      = -L$(RAYLIB)/lib
LIBS         = -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32
GAME_SRC     = game.c tiles.c link.c
SRC          = main.c $(GAME_SRC) hal_pc.c link_pc.c
TEST_CFLAGS  = -std=c99 -Wall -Wextra -O2

pong.exe: $(SRC) hal.h game.h tiles.h link.h link_pc.h
	$(CC) $(CFLAGS) -o pong.exe $(SRC) $(LDFLAGS) $(LIBS)

# Pruebas: compila el juego con un HAL en memoria y revisa menu, pausa, fin de partida...
# Deja capturas .ppm en pruebas/capturas (se abren con GIMP, IrfanView, etc.)
pruebas/capturas:
	mkdir -p pruebas/capturas

test: pruebas/capturas
	$(CC) $(TEST_CFLAGS) -o pruebas/pong_test pruebas/hal_test.c $(GAME_SRC)
	./pruebas/pong_test
	$(CC) $(TEST_CFLAGS) -DCONSOLA=A -include pruebas/renombrar.h -c game.c -o pruebas/game_A.o
	$(CC) $(TEST_CFLAGS) -DCONSOLA=A -include pruebas/renombrar.h -c link.c -o pruebas/link_A.o
	$(CC) $(TEST_CFLAGS) -DCONSOLA=B -include pruebas/renombrar.h -c game.c -o pruebas/game_B.o
	$(CC) $(TEST_CFLAGS) -DCONSOLA=B -include pruebas/renombrar.h -c link.c -o pruebas/link_B.o
	$(CC) $(TEST_CFLAGS) -o pruebas/link_test pruebas/link_test.c tiles.c \
		pruebas/game_A.o pruebas/link_A.o pruebas/game_B.o pruebas/link_B.o
	./pruebas/link_test

clean:
	rm -f pong.exe pruebas/pong_test pruebas/pong_test.exe pruebas/link_test pruebas/link_test.exe pruebas/*.o

.PHONY: test clean
