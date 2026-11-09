# Makefile para Windows (MinGW-w64 / w64devkit) con raylib
#
# RAYLIB = carpeta que contiene "include" y "lib" de raylib.
#   Opcion A (la mas simple): copiar las carpetas include y lib junto a este Makefile -> RAYLIB = .
#   Opcion B (instalador de raylib): RAYLIB = C:/raylib/raylib
RAYLIB = .
 
CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2 -I$(RAYLIB)/include
LDFLAGS = -L$(RAYLIB)/lib
LIBS    = -lraylib -lopengl32 -lgdi32 -lwinmm
SRC     = main.c game.c tiles.c hal_pc.c
 
pong.exe: $(SRC) hal.h game.h tiles.h
	$(CC) $(CFLAGS) -o pong.exe $(SRC) $(LDFLAGS) $(LIBS)
 
clean:
	rm -f pong.exe
 