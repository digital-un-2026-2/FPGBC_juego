/*
 * juego_completo.c
 * 
 * Videojuego de plataformas completo para Game Boy (GB / GBC).
 * 
 * Caracteristicas implementadas:
 *   1. Menu de seleccion de nivel interactivo en espanol ("ELIGE TU NIVEL").
 *   2. Tres niveles independientes de 96x16 tiles (768 px de longitud horizontal).
 *   3. Fisicas reales de plataformas: gravedad progresiva, salto y colisiones.
 *   4. Gravedad real para enemigos: caen y aterrizan en plataformas sin quedar suspendidos.
 *   5. Patrullaje dinamico de enemigos (2 pasos a la izq y 2 pasos a la der).
 *   6. Disparo de proyectil con boton A para destruir los enemigos.
 *   7. Monedas coleccionables (gemas diamante) con contador HUD en la parte superior.
 *   8. Sonido nativo de Game Boy para salto, disparo, impacto, moneda, muerte y victoria.
 *   9. Pantallas de fin de partida en espanol: FIN DEL JUEGO y VICTORIA.
 *  10. Opciones de reintento con START o retorno al menu principal con boton B.
 */

#include <gb/gb.h>
#include "Tile1.c"
#include "Tile2.c"
#include "Mapa.c"
#include "niveles/NivelesData.c"

/* --- CONSTANTES DEL ESCENARIO --- */
#define TOTAL_NIVELES     3
#define MAP_W_TILES       96
#define MAP_H_TILES       16
#define MAP_W_PX          (MAP_W_TILES * 8)   /* 768 px */
#define MAP_H_PX          (MAP_H_TILES * 8)   /* 128 px */
#define SCREEN_W_PX       160
#define SCREEN_H_PX       144
#define MAX_CAM_X         (MAP_W_PX - SCREEN_W_PX) /* 608 px */

/* Posicion de inicio (Spawn) */
#define SPAWN_X           16
#define SPAWN_Y           80

/* Coordenada de meta para la victoria */
#define GOAL_X            716 /* Columna 89.5 - Entrada al arco de meta */

/* Limite de muerte por caida al abismo */
#define PIT_DEATH_Y       120

/* Constantes de fisica del jugador */
#define WALK_SPEED_PX     1
#define GRAVITY_SUBPX     36
#define JUMP_IMPULSE      -730    /* ~27.5 px de altura de salto */
#define MAX_FALL_SPEED    768

/* Estados de juego */
#define ESTADO_MENU       0
#define ESTADO_JUGANDO    1
#define ESTADO_GAME_OVER  2
#define ESTADO_VICTORIA   3

/* Indices de sprites de hardware (OAM 0..39) */
#define SPR_PLAYER_0      1
#define SPR_PLAYER_1      2
#define SPR_PLAYER_2      3
#define SPR_PLAYER_3      4
#define SPR_PROYECTIL     5
#define SPR_OBS_BASE      6 /* Sprites 6, 7, 8, 9 para obstaculos (4 obstaculos) */
#define SPR_HUD_COIN      10
#define SPR_HUD_X         11
#define SPR_HUD_D1        12
#define SPR_HUD_D0        13

/* Tiles de sprites cargados en VRAM */
#define TILE_SPR_COIN     14
#define TILE_SPR_DIG0     15 /* 15..24 = digitos '0'..'9' */
#define TILE_SPR_X        25 /* letra 'X' */

/* Maximo de obstaculos simultaneos en el nivel */
#define NUM_OBSTACULOS    4

/* --- ESTRUCTURAS --- */

struct ObjetoJugador {
    UBYTE spiritids[4];
};

struct Proyectil {
    INT16 x;
    INT16 y;
    UBYTE dir;         /* 1 = derecha, 0 = izquierda */
    UBYTE activo;
    UBYTE distancia;
};

struct Obstaculo {
    INT16 origin_x;
    INT16 x;
    INT16 y;
    INT8 vel_y;        /* Velocidad vertical: gravedad real de los enemigos */
    INT8 dir;          /* -1 = izquierda, +1 = derecha */
    INT8 offset;       /* Desplazamiento respecto a origin_x (-16 .. +16: 2 pasos a cada lado) */
    UBYTE timer;       /* Control de velocidad de patrullaje */
    UBYTE activo;
    UBYTE sprite_id;
};

struct ConfigEnemigo {
    INT16 origin_x;
    INT16 y;
};

/* Configuracion inicial de enemigos para cada uno de los 3 niveles */
const struct ConfigEnemigo enemigos_por_nivel[3][NUM_OBSTACULOS] = {
    /* Nivel 1: Pradera e Islas */
    { {152, 88}, {240, 64}, {440, 56}, {624, 88} },
    /* Nivel 2: Fortaleza y Torres (plataformas y suelos escalonados) */
    { {160, 88}, {208, 72}, {336, 64}, {512, 64} },
    /* Nivel 3: Desafio de Abismos (plataformas intermedias) */
    { {128, 72}, {208, 80}, {352, 72}, {464, 64} }
};

/* --- VARIABLES GLOBALES --- */

struct ObjetoJugador jugador;
struct Proyectil proyectil;
struct Obstaculo obstaculos[NUM_OBSTACULOS];

#define MAP_TOTAL_TILES   (MAP_W_TILES * MAP_H_TILES) /* 1536 */
UBYTE coins_collected[MAP_TOTAL_TILES / 8];           /* 192 bytes de bitmask en WRAM */
UINT8 monedas_recogidas;

INT16 plx;
INT16 ply_fp;
INT16 vel_y;
UBYTE on_ground;
UBYTE pldir;

INT16 camera_x;
INT16 stream_right_col;
INT16 stream_left_col;

UBYTE estado_juego;
UBYTE prev_joy;

UINT8 nivel_seleccionado;
UINT8 nivel_actual;
const unsigned char* nivel_actual_mapa;

/* --- DECLARACIONES PREVIAS --- */
void actualizar_cursor_menu(void);
void iniciar_nivel(UINT8 lvl);
void mostrar_menu_seleccion(void);

/* --- SISTEMA DE SONIDO CHIPTUNE NATIVO --- */

void init_sound(void) {
    NR52_REG = 0x80; /* Activar circuito de sonido general de la Game Boy */
    NR50_REG = 0x77; /* Volumen maximo estereo (7/7) */
    NR51_REG = 0xFF; /* Habilitar todos los canales de audio */
}

/* Sonido 1: Salto (onda cuadrada con barrido ascendente rapido) */
void play_sound_jump(void) {
    NR10_REG = 0x16; /* Barrido hacia arriba */
    NR11_REG = 0x80; /* Ciclo de trabajo 50% */
    NR12_REG = 0xF3; /* Volumen 15, decay medio */
    NR13_REG = 0x60; /* Frecuencia base */
    NR14_REG = 0x86; /* Trigger */
}

/* Sonido 2: Disparo de proyectil (laser afilado descendente) */
void play_sound_shoot(void) {
    NR10_REG = 0x1C; /* Barrido rapido hacia abajo */
    NR11_REG = 0x40; /* Ciclo de trabajo 25% */
    NR12_REG = 0xE2; /* Volumen 14, fade rapido */
    NR13_REG = 0xA0; /* Frecuencia aguda */
    NR14_REG = 0x87; /* Trigger */
}

/* Sonido 3: Impacto y destruccion de obstaculo (explosion de ruido blanco) */
void play_sound_hit(void) {
    NR41_REG = 0x20; /* Duracion corta */
    NR42_REG = 0xF1; /* Volumen 15, caida rapida */
    NR43_REG = 0x33; /* Frecuencia de ruido crujiente */
    NR44_REG = 0x80; /* Trigger Canal 4 */
}

/* Sonido 4: Muerte y derrota (tono descendente con golpe sordo) */
void play_sound_death(void) {
    NR10_REG = 0x3E; /* Barrido descendente */
    NR11_REG = 0x80;
    NR12_REG = 0xF4; /* Decay prolongado */
    NR13_REG = 0x80;
    NR14_REG = 0x84; /* Trigger Canal 1 */
    NR41_REG = 0x3F;
    NR42_REG = 0xC2;
    NR43_REG = 0x65;
    NR44_REG = 0x80; /* Trigger Canal 4 */
}

/* Sonido 5: Victoria (fanfarria triunfal brillante) */
void play_sound_win(void) {
    NR10_REG = 0x00; /* Sin barrido */
    NR11_REG = 0x80;
    NR12_REG = 0xF4; /* Volumen 15, decay sonoro */
    NR13_REG = 0xD7; /* Nota brillante aguda (Sol / C6) */
    NR14_REG = 0x86; /* Trigger */
}

/* Sonido 6: Recogida de moneda / gema (ding brillante agudo en Canal 2) */
void play_sound_coin(void) {
    NR21_REG = 0x80; /* Ciclo de trabajo 50% */
    NR22_REG = 0xF1; /* Volumen maximo 15, caida rapida */
    NR23_REG = 0x5C; /* Frecuencia aguda (Nota B5 / Si) */
    NR24_REG = 0x87; /* Trigger */
}

/* --- STREAMING DE COLUMNAS A VRAM (Nivel extendido de 96 tiles) --- */

/* Carga una columna de 16 tiles desde el mapa activo hacia la VRAM circular */
void load_vram_column(UINT8 map_col, UINT8 vram_col) {
    UINT8 r;
    UINT16 idx;
    UBYTE tile;
    for (r = 0; r < MAP_H_TILES; r++) {
        idx = ((UINT16)r * (UINT16)MAP_W_TILES) + (UINT16)map_col;
        tile = nivel_actual_mapa[idx];
        /* Si la moneda ya fue recogida, no volver a dibujarla en VRAM */
        if (tile == 1 && (coins_collected[idx >> 3] & (1 << (idx & 7)))) {
            tile = 0;
        }
        set_bkg_tile_xy(vram_col, r, tile);
    }
}

/* Inicializa el buffer circular de 32 columnas de VRAM */
void init_vram_buffer(void) {
    UINT8 c;
    for (c = 0; c < 32; c++) {
        load_vram_column(c, c);
    }
    stream_right_col = 31;
    stream_left_col = 0;
}

/* Actualiza el streaming segun la posicion de la camara */
void update_vram_streaming(INT16 cam_x) {
    INT16 needed_right;
    INT16 needed_left;

    needed_right = (cam_x + SCREEN_W_PX + 8) >> 3;
    if (needed_right >= MAP_W_TILES) needed_right = MAP_W_TILES - 1;

    while (stream_right_col < needed_right) {
        stream_right_col++;
        load_vram_column((UINT8)stream_right_col, (UINT8)(stream_right_col & 31));
    }

    needed_left = (cam_x > 8) ? ((cam_x - 8) >> 3) : 0;
    while (stream_left_col > needed_left) {
        stream_left_col--;
        load_vram_column((UINT8)stream_left_col, (UINT8)(stream_left_col & 31));
    }
}

/* --- GESTION DE SPRITES --- */

void moverJugador(INT16 x, INT16 y) {
    move_sprite(jugador.spiritids[0], (UINT8)x,     (UINT8)y);
    move_sprite(jugador.spiritids[1], (UINT8)(x+8), (UINT8)y);
    move_sprite(jugador.spiritids[2], (UINT8)x,     (UINT8)(y+8));
    move_sprite(jugador.spiritids[3], (UINT8)(x+8), (UINT8)(y+8));
}

void ocultarJugador(void) {
    move_sprite(jugador.spiritids[0], 0, 0);
    move_sprite(jugador.spiritids[1], 0, 0);
    move_sprite(jugador.spiritids[2], 0, 0);
    move_sprite(jugador.spiritids[3], 0, 0);
}

void update_hud_monedas(void) {
    UINT8 d1 = monedas_recogidas / 10;
    UINT8 d0 = monedas_recogidas % 10;
    set_sprite_tile(SPR_HUD_D1, (UBYTE)(TILE_SPR_DIG0 + d1));
    set_sprite_tile(SPR_HUD_D0, (UBYTE)(TILE_SPR_DIG0 + d0));
}

void setupSprites(void) {
    UINT8 i;

    /* Jugador (16x16: tiles 1, 2, 3, 4 de TileLabel2) */
    set_sprite_tile(SPR_PLAYER_0, 1);
    jugador.spiritids[0] = SPR_PLAYER_0;
    set_sprite_tile(SPR_PLAYER_1, 2);
    jugador.spiritids[1] = SPR_PLAYER_1;
    set_sprite_tile(SPR_PLAYER_2, 3);
    jugador.spiritids[2] = SPR_PLAYER_2;
    set_sprite_tile(SPR_PLAYER_3, 4);
    jugador.spiritids[3] = SPR_PLAYER_3;

    /* Proyectil (8x8: tile 12 de TileLabel2 - orbe de energia) */
    set_sprite_tile(SPR_PROYECTIL, 12);
    move_sprite(SPR_PROYECTIL, 0, 0);

    /* Obstaculos (8x8: tile 9 de TileLabel2 - criatura peligrosa) */
    for (i = 0; i < NUM_OBSTACULOS; i++) {
        obstaculos[i].sprite_id = (UBYTE)(SPR_OBS_BASE + i);
        set_sprite_tile(obstaculos[i].sprite_id, 9);
        move_sprite(obstaculos[i].sprite_id, 0, 0);
    }

    /* HUD de monedas superior: [Moneda] x [D1][D0] */
    set_sprite_data(TILE_SPR_COIN, 1,  &MapaLabel[1 * 16]);  /* Tile 1 de MapaLabel = gema diamante */
    set_sprite_data(TILE_SPR_DIG0, 10, &MapaLabel[7 * 16]);  /* Tiles 7..16 = digitos '0'..'9' */
    set_sprite_data(TILE_SPR_X,    1,  &MapaLabel[40 * 16]); /* Tile 40 = letra 'X' */

    set_sprite_tile(SPR_HUD_COIN, TILE_SPR_COIN);
    set_sprite_tile(SPR_HUD_X,    TILE_SPR_X);
    set_sprite_tile(SPR_HUD_D1,   TILE_SPR_DIG0);
    set_sprite_tile(SPR_HUD_D0,   TILE_SPR_DIG0);
}

/* --- DETECCION DE COLISIONES CON EL MAPA --- */

UBYTE is_solid_tile(UBYTE tile) {
    if (tile == 2) return 1;              /* Ladrillo */
    if (tile >= 3 && tile <= 6) return 1; /* Bloques de suelo */
    return 0;                             /* Aire (0), gemas (1) */
}

UBYTE get_world_tile(INT16 wx, INT16 wy) {
    UINT16 idx;
    UBYTE tile;
    if (wx < 0 || wx >= MAP_W_PX) return 2; /* Paredes en los extremos */
    if (wy < 0 || wy >= MAP_H_PX) return 0; /* Cielo y abismo */

    idx = (((UINT16)(wy >> 3)) * (UINT16)MAP_W_TILES) + (UINT16)(wx >> 3);
    tile = nivel_actual_mapa[idx];
    if (tile == 1 && (coins_collected[idx >> 3] & (1 << (idx & 7)))) {
        return 0;
    }
    return tile;
}

UBYTE point_is_solid(INT16 wx, INT16 wy) {
    return is_solid_tile(get_world_tile(wx, wy));
}

/* --- TEXTO Y MENSAJES EN PANTALLA --- */

UBYTE ascii_to_tile(char c) {
    if (c >= 'A' && c <= 'Z') return (UBYTE)(17 + (c - 'A'));
    if (c >= 'a' && c <= 'z') return (UBYTE)(17 + (c - 'a'));
    if (c >= '1' && c <= '9') return (UBYTE)(8 + (c - '1'));
    if (c == '0') return 7;
    return 0;
}

void draw_text(UINT8 screen_tile_x, UINT8 screen_tile_y, const char* str) {
    UINT8 map_x, map_y, tile_val;
    map_y = screen_tile_y;
    while (*str) {
        tile_val = ascii_to_tile(*str);
        map_x = (UINT8)(((camera_x >> 3) + screen_tile_x) & 31);
        set_bkg_tiles(map_x, map_y, 1, 1, &tile_val);
        screen_tile_x++;
        str++;
    }
}

void draw_text_menu(UINT8 x, UINT8 y, const char* str) {
    UINT8 tile_val;
    while (*str) {
        tile_val = ascii_to_tile(*str);
        set_bkg_tiles(x, y, 1, 1, &tile_val);
        x++;
        str++;
    }
}

void limpiar_pantalla(void) {
    UINT8 x, y;
    UINT8 empty_tile = 0;
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            set_bkg_tiles(x, y, 1, 1, &empty_tile);
        }
    }
}

void mostrar_game_over(void) {
    ocultarJugador();
    draw_text(4, 5, "FIN DEL JUEGO");
    draw_text(4, 7, "START REINTENTO");
    draw_text(4, 9, "B VOLVER AL MENU");
}

void mostrar_victoria(void) {
    draw_text(6, 5, "VICTORIA");
    draw_text(4, 7, "START REINTENTO");
    draw_text(4, 9, "B VOLVER AL MENU");
}

/* --- LOGICA DE OBSTACULOS: GRAVEDAD REAL Y PATRULLAJE --- */

void init_obstaculos_nivel(UINT8 lvl) {
    UINT8 i;
    for (i = 0; i < NUM_OBSTACULOS; i++) {
        obstaculos[i].origin_x = enemigos_por_nivel[lvl][i].origin_x;
        obstaculos[i].y        = enemigos_por_nivel[lvl][i].y;
        obstaculos[i].vel_y    = 0;
        obstaculos[i].dir      = -1;
        obstaculos[i].offset   = 0;
        obstaculos[i].timer    = 0;
        obstaculos[i].activo   = 1;
        obstaculos[i].x        = obstaculos[i].origin_x;
        move_sprite(obstaculos[i].sprite_id, 0, 0);
    }
}

/* Actualiza gravedad y patrullaje de los enemigos: caen si estan en el aire */
void update_obstaculos(void) {
    UINT8 i;
    INT16 check_y;
    for (i = 0; i < NUM_OBSTACULOS; i++) {
        if (obstaculos[i].activo) {
            /* 1. Gravedad real del enemigo: si no tiene suelo solido debajo, cae */
            check_y = obstaculos[i].y + 8;
            if (!point_is_solid(obstaculos[i].x + 1, check_y) &&
                !point_is_solid(obstaculos[i].x + 6, check_y)) {
                if (obstaculos[i].vel_y < 3) {
                    obstaculos[i].vel_y++;
                }
                obstaculos[i].y += obstaculos[i].vel_y;

                /* Aterrizaje al chocar con suelo solido */
                if (point_is_solid(obstaculos[i].x + 1, obstaculos[i].y + 7) ||
                    point_is_solid(obstaculos[i].x + 6, obstaculos[i].y + 7)) {
                    obstaculos[i].y = (INT16)(((obstaculos[i].y + 7) >> 3) << 3) - 8;
                    obstaculos[i].vel_y = 0;
                }
            } else {
                obstaculos[i].vel_y = 0;
            }

            /* Si cae al abismo inferior, se desactiva */
            if (obstaculos[i].y >= PIT_DEATH_Y) {
                obstaculos[i].activo = 0;
                move_sprite(obstaculos[i].sprite_id, 0, 0);
                continue;
            }

            /* 2. Patrullaje horizontal con colision de bloques (no atraviesan muros) */
            obstaculos[i].timer++;
            if (obstaculos[i].timer >= 3) {
                obstaculos[i].timer = 0;
                if (obstaculos[i].dir == -1) {
                    /* Comprobar colision frontal izquierda con bloques solidos */
                    if (point_is_solid(obstaculos[i].x - 1, obstaculos[i].y + 1) ||
                        point_is_solid(obstaculos[i].x - 1, obstaculos[i].y + 6)) {
                        /* Choca con muro a la izquierda: rebota hacia la derecha */
                        obstaculos[i].dir = 1;
                    } else if (obstaculos[i].offset <= -16) {
                        /* Fin de rango de patrullaje a la izquierda */
                        obstaculos[i].dir = 1;
                    } else {
                        /* Avanzar un pixel a la izquierda */
                        obstaculos[i].x--;
                        obstaculos[i].offset--;
                    }
                } else {
                    /* Comprobar colision frontal derecha con bloques solidos */
                    if (point_is_solid(obstaculos[i].x + 8, obstaculos[i].y + 1) ||
                        point_is_solid(obstaculos[i].x + 8, obstaculos[i].y + 6)) {
                        /* Choca con muro a la derecha: rebota hacia la izquierda */
                        obstaculos[i].dir = -1;
                    } else if (obstaculos[i].offset >= 16) {
                        /* Fin de rango de patrullaje a la derecha */
                        obstaculos[i].dir = -1;
                    } else {
                        /* Avanzar un pixel a la derecha */
                        obstaculos[i].x++;
                        obstaculos[i].offset++;
                    }
                }
            }
        }
    }
}

/* Comprueba la recogida de monedas cuando el jugador las toca */
void check_coin_collection(INT16 px, INT16 py) {
    INT16 left_col   = (px + 2) >> 3;
    INT16 right_col  = (px + 13) >> 3;
    INT16 top_row    = (py + 2) >> 3;
    INT16 bot_row    = (py + 14) >> 3;
    INT16 c, r;
    UINT16 idx;
    UBYTE mask;

    if (left_col < 0) left_col = 0;
    if (right_col >= MAP_W_TILES) right_col = MAP_W_TILES - 1;
    if (top_row < 0) top_row = 0;
    if (bot_row >= MAP_H_TILES) bot_row = MAP_H_TILES - 1;

    for (r = top_row; r <= bot_row; r++) {
        for (c = left_col; c <= right_col; c++) {
            idx = ((UINT16)r * (UINT16)MAP_W_TILES) + (UINT16)c;
            if (nivel_actual_mapa[idx] == 1) {
                mask = (UBYTE)(1 << (idx & 7));
                if (!(coins_collected[idx >> 3] & mask)) {
                    coins_collected[idx >> 3] |= mask;
                    set_bkg_tile_xy((UINT8)(c & 31), (UINT8)r, 0);
                    monedas_recogidas++;
                    play_sound_coin();
                    update_hud_monedas();
                }
            }
        }
    }
}

/* --- GESTION DE MENU Y NIVELES --- */

void actualizar_cursor_menu(void) {
    /* Mueve la gema diamante como cursor al lado del nivel seleccionado */
    move_sprite(SPR_HUD_COIN, 36, (UINT8)(56 + nivel_seleccionado * 16));
}

void mostrar_menu_seleccion(void) {
    UINT8 i;
    estado_juego = ESTADO_MENU;

    /* Ocultar elementos de juego */
    ocultarJugador();
    move_sprite(SPR_PROYECTIL, 0, 0);
    for (i = 0; i < NUM_OBSTACULOS; i++) {
        move_sprite(obstaculos[i].sprite_id, 0, 0);
    }
    move_sprite(SPR_HUD_X, 0, 0);
    move_sprite(SPR_HUD_D1, 0, 0);
    move_sprite(SPR_HUD_D0, 0, 0);

    /* Reiniciar posicion de camara y limpiar pantalla */
    camera_x = 0;
    move_bkg(0, 0);
    limpiar_pantalla();

    /* Textos en espanol */
    draw_text_menu(3, 2,  "ELIGE TU NIVEL");
    draw_text_menu(6, 5,  "NIVEL 1");
    draw_text_menu(6, 7,  "NIVEL 2");
    draw_text_menu(6, 9,  "NIVEL 3");
    draw_text_menu(2, 13, "PULSA A O START");

    actualizar_cursor_menu();
}

void iniciar_nivel(UINT8 lvl) {
    UINT16 ci;
    UINT8 i;

    nivel_actual = lvl;
    nivel_actual_mapa = niveles_mapas[lvl];

    plx = SPAWN_X;
    ply_fp = ((INT16)SPAWN_Y) << 8;
    vel_y = 0;
    on_ground = 1;
    pldir = 1;
    camera_x = 0;
    estado_juego = ESTADO_JUGANDO;

    /* Restablecer monedas recogidas */
    monedas_recogidas = 0;
    for (ci = 0; ci < sizeof(coins_collected); ci++) {
        coins_collected[ci] = 0;
    }
    update_hud_monedas();

    /* Desactivar proyectil */
    proyectil.activo = 0;
    move_sprite(SPR_PROYECTIL, 0, 0);

    /* Inicializar obstaculos para este nivel */
    init_obstaculos_nivel(lvl);

    /* Iniciar streaming de VRAM y posicionar camara */
    init_vram_buffer();
    move_bkg(0, 0);

    /* Mostrar jugador */
    moverJugador(plx + 8, SPAWN_Y + 16);

    /* Ocultar obstaculos lejanos */
    for (i = 0; i < NUM_OBSTACULOS; i++) {
        move_sprite(obstaculos[i].sprite_id, 0, 0);
    }

    /* Mostrar HUD de monedas en pantalla */
    move_sprite(SPR_HUD_COIN, 16, 22);
    move_sprite(SPR_HUD_X,    24, 22);
    move_sprite(SPR_HUD_D1,   32, 22);
    move_sprite(SPR_HUD_D0,   40, 22);
}

/* --- BUCLE PRINCIPAL --- */

void main(void) {
    UINT8 cur_joy;
    INT16 ply_px;
    INT16 next_x;
    INT16 check_x;
    INT16 check_y;
    INT16 spr_screen_x;
    INT16 spr_screen_y;
    UINT8 i;

    /* Inicializar sonido */
    init_sound();

    /* Cargar graficos a VRAM */
    set_sprite_data(0, 14, TileLabel2);
    set_bkg_data(0, 43, MapaLabel);

    SHOW_SPRITES;
    SHOW_BKG;

    setupSprites();

    /* Arrancar directamente en el menu de seleccion de nivel */
    nivel_seleccionado = 0;
    mostrar_menu_seleccion();
    prev_joy = 0;

    while (1) {
        cur_joy = joypad();

        if (estado_juego == ESTADO_MENU) {
            /* 1. Navegacion del menu con Arriba / Abajo */
            if ((cur_joy & J_DOWN) && !(prev_joy & J_DOWN)) {
                if (nivel_seleccionado < (TOTAL_NIVELES - 1)) {
                    nivel_seleccionado++;
                    actualizar_cursor_menu();
                    play_sound_jump();
                }
            } else if ((cur_joy & J_UP) && !(prev_joy & J_UP)) {
                if (nivel_seleccionado > 0) {
                    nivel_seleccionado--;
                    actualizar_cursor_menu();
                    play_sound_jump();
                }
            } else if ((cur_joy & (J_A | J_START)) && !(prev_joy & (J_A | J_START))) {
                play_sound_coin();
                iniciar_nivel(nivel_seleccionado);
            }

        } else if (estado_juego == ESTADO_JUGANDO) {
            ply_px = ply_fp >> 8;

            /* 1. Movimiento horizontal */
            if (cur_joy & J_RIGHT) {
                pldir = 1;
                next_x = plx + WALK_SPEED_PX;
                check_x = next_x + 13;
                if (!point_is_solid(check_x, ply_px + 3) &&
                    !point_is_solid(check_x, ply_px + 8) &&
                    !point_is_solid(check_x, ply_px + 14)) {
                    plx = next_x;
                    if (plx > (MAP_W_PX - 16)) plx = MAP_W_PX - 16;
                }
            } else if (cur_joy & J_LEFT) {
                pldir = 0;
                next_x = plx - WALK_SPEED_PX;
                check_x = next_x + 2;
                if (!point_is_solid(check_x, ply_px + 3) &&
                    !point_is_solid(check_x, ply_px + 8) &&
                    !point_is_solid(check_x, ply_px + 14)) {
                    plx = next_x;
                    if (plx < 0) plx = 0;
                }
            }

            /* 2. Salto (Boton B o Flecha Arriba) */
            if ((cur_joy & (J_B | J_UP)) && !(prev_joy & (J_B | J_UP))) {
                if (on_ground) {
                    vel_y = JUMP_IMPULSE;
                    on_ground = 0;
                    play_sound_jump();
                }
            }

            /* 3. Disparo de proyectil con boton A */
            if ((cur_joy & J_A) && !(prev_joy & J_A)) {
                if (!proyectil.activo) {
                    proyectil.activo = 1;
                    proyectil.dir = pldir;
                    proyectil.x = (pldir == 1) ? (plx + 14) : (plx - 6);
                    proyectil.y = ply_px + 4;
                    proyectil.distancia = 0;
                    play_sound_shoot();
                }
            }

            /* 4. Actualizacion de fisicas y gravedad del jugador */
            if (!on_ground) {
                vel_y += GRAVITY_SUBPX;
                if (vel_y > MAX_FALL_SPEED) vel_y = MAX_FALL_SPEED;
            }

            ply_fp += vel_y;
            ply_px = ply_fp >> 8;

            /* Colision con techo */
            if (vel_y < 0) {
                check_y = ply_px + 2;
                if (point_is_solid(plx + 3, check_y) || point_is_solid(plx + 12, check_y)) {
                    vel_y = 0;
                    ply_px = (INT16)(((check_y >> 3) + 1) << 3) - 2;
                    ply_fp = ply_px << 8;
                }
            }

            /* Colision con suelo y plataformas */
            if (vel_y >= 0) {
                check_y = ply_px + 15;
                if (point_is_solid(plx + 3, check_y) || point_is_solid(plx + 12, check_y)) {
                    vel_y = 0;
                    on_ground = 1;
                    ply_px = (INT16)((check_y >> 3) << 3) - 16;
                    ply_fp = ply_px << 8;
                } else {
                    if (point_is_solid(plx + 3, ply_px + 16) || point_is_solid(plx + 12, ply_px + 16)) {
                        vel_y = 0;
                        on_ground = 1;
                    } else {
                        on_ground = 0;
                    }
                }
            }

            /* 5. Actualizacion del proyectil */
            if (proyectil.activo) {
                if (proyectil.dir == 1) {
                    proyectil.x += 3;
                } else {
                    proyectil.x -= 3;
                }
                proyectil.distancia += 3;

                /* Desaparece si alcanza el rango maximo o choca con muro solido */
                if (proyectil.distancia >= 100 || point_is_solid(proyectil.x + 4, proyectil.y + 4)) {
                    proyectil.activo = 0;
                    move_sprite(SPR_PROYECTIL, 0, 0);
                } else {
                    /* Comprobar impacto contra obstaculos */
                    for (i = 0; i < NUM_OBSTACULOS; i++) {
                        if (obstaculos[i].activo) {
                            if ((proyectil.x + 7 >= obstaculos[i].x) &&
                                (proyectil.x <= obstaculos[i].x + 7) &&
                                (proyectil.y + 12 >= obstaculos[i].y) &&
                                (proyectil.y <= obstaculos[i].y + 12)) {
                                /* Destruir obstaculo */
                                obstaculos[i].activo = 0;
                                move_sprite(obstaculos[i].sprite_id, 0, 0);
                                proyectil.activo = 0;
                                move_sprite(SPR_PROYECTIL, 0, 0);
                                play_sound_hit();
                                break;
                            }
                        }
                    }
                }
            }

            /* 5b. Actualizacion de patrullaje Y GRAVEDAD de los enemigos */
            update_obstaculos();

            /* 6. Comprobacion de colision del jugador con obstaculos */
            for (i = 0; i < NUM_OBSTACULOS; i++) {
                if (obstaculos[i].activo) {
                    if ((plx + 13 >= obstaculos[i].x) &&
                        (plx + 2 <= obstaculos[i].x + 7) &&
                        (ply_px + 14 >= obstaculos[i].y) &&
                        (ply_px + 2 <= obstaculos[i].y + 7)) {
                        /* Muerte por contacto con obstaculo peligroso */
                        estado_juego = ESTADO_GAME_OVER;
                        play_sound_death();
                        mostrar_game_over();
                        break;
                    }
                }
            }

            /* 6b. Recogida de monedas al tocarlas */
            if (estado_juego == ESTADO_JUGANDO) {
                check_coin_collection(plx, ply_px);
            }

            /* 7. Comprobacion de muerte por caida en hueco */
            if (estado_juego == ESTADO_JUGANDO && ply_px >= PIT_DEATH_Y) {
                estado_juego = ESTADO_GAME_OVER;
                play_sound_death();
                mostrar_game_over();
            }

            /* 8. Comprobacion de victoria al alcanzar la meta */
            if (estado_juego == ESTADO_JUGANDO && plx >= GOAL_X) {
                estado_juego = ESTADO_VICTORIA;
                play_sound_win();
                mostrar_victoria();
            }

            /* 9. Camara, streaming y renderizado en pantalla */
            if (estado_juego == ESTADO_JUGANDO) {
                camera_x = plx - 72;
                if (camera_x < 0) camera_x = 0;
                if (camera_x > MAX_CAM_X) camera_x = MAX_CAM_X;

                update_vram_streaming(camera_x);
                move_bkg((UINT8)(camera_x & 255), 0);

                /* Renderizar jugador */
                spr_screen_x = plx - camera_x + 8;
                spr_screen_y = ply_px + 16;
                moverJugador(spr_screen_x, spr_screen_y);

                /* Renderizar proyectil */
                if (proyectil.activo) {
                    spr_screen_x = proyectil.x - camera_x + 8;
                    spr_screen_y = proyectil.y + 16;
                    if (spr_screen_x >= 4 && spr_screen_x <= 164) {
                        move_sprite(SPR_PROYECTIL, (UINT8)spr_screen_x, (UINT8)spr_screen_y);
                    } else {
                        move_sprite(SPR_PROYECTIL, 0, 0);
                    }
                }

                /* Renderizar obstaculos activos en pantalla */
                for (i = 0; i < NUM_OBSTACULOS; i++) {
                    if (obstaculos[i].activo) {
                        spr_screen_x = obstaculos[i].x - camera_x + 8;
                        spr_screen_y = obstaculos[i].y + 16;
                        if (spr_screen_x >= 0 && spr_screen_x <= 168) {
                            move_sprite(obstaculos[i].sprite_id, (UINT8)spr_screen_x, (UINT8)spr_screen_y);
                        } else {
                            move_sprite(obstaculos[i].sprite_id, 0, 0);
                        }
                    }
                }
            }

        } else {
            /* ESTADO_GAME_OVER o ESTADO_VICTORIA */
            if ((cur_joy & J_START) && !(prev_joy & J_START)) {
                /* START: Reintentar el mismo nivel */
                iniciar_nivel(nivel_actual);
            } else if ((cur_joy & (J_B | J_SELECT)) && !(prev_joy & (J_B | J_SELECT))) {
                /* B o SELECT: Regresar al menu de seleccion de nivel */
                mostrar_menu_seleccion();
            }
        }

        prev_joy = cur_joy;
        wait_vbl_done();
    }
}
