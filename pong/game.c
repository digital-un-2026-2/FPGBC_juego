/*
 * game.c - Pong portable.
 *
 * Reglas para que compile igual en PC y en el FemtoRV:
 *  - Solo enteros (nada de float/double): el RV32I base no tiene FPU.
 *  - Sin malloc, sin printf, sin librerias del SO: solo <stdint.h>.
 *  - Sin multiplicar/dividir en el camino caliente (el RV32I base puede no
 *    tener extension M): se usan desplazamientos y tablas.
 *  - Todo el hardware pasa por hal.h.
 */
#include "game.h"
#include "hal.h"
#include "tiles.h"

/* Geometria (en pixeles) */
#define FIELD_TOP      24                       /* debajo de la pared superior */
#define FIELD_BOTTOM   (SCREEN_H - 8)           /* encima de la pared inferior */
#define PADDLE_W       8
#define PADDLE_H       32
#define BALL_S         8
#define P1_X           8
#define P2_X           (SCREEN_W - 8 - PADDLE_W)

/* Reglas */
#define WIN_SCORE      7
#define PLAYER_SPEED   2
#define CPU_SPEED      1
#define BALL_MAX_SPEED 4

/* IDs de sprites (posiciones en la OAM) */
#define SPR_P1   0   /* 0..3 */
#define SPR_P2   4   /* 4..7 */
#define SPR_BALL 8

/* Celdas del marcador en el mapa de fondo */
/* (se calculan desde MAP_W para que sigan centrados si cambia la resolucion) */
#define SCORE1_TX (MAP_W / 2 - 4)
#define SCORE2_TX (MAP_W / 2 + 3)
#define SCORE_TY  1
#define NET_TX    (MAP_W / 2 - 1)   /* el tile NET lleva la linea en su borde derecho */

enum { ST_SERVE, ST_PLAY, ST_OVER };

static int16_t p1_y, p2_y;
static int16_t ball_x, ball_y;
static int8_t  ball_vx, ball_vy;
static int8_t  serve_dir;
static uint8_t score1, score2;
static uint8_t state, hits;
static uint8_t frame_count, prev_btn;

static int16_t clamp_paddle(int16_t y)
{
    if (y < FIELD_TOP) return FIELD_TOP;
    if (y > FIELD_BOTTOM - PADDLE_H) return FIELD_BOTTOM - PADDLE_H;
    return y;
}

static void draw_hud(void)
{
    hal_set_bg(SCORE1_TX, SCORE_TY, (uint8_t)(TILE_DIGIT0 + score1));
    hal_set_bg(SCORE2_TX, SCORE_TY, (uint8_t)(TILE_DIGIT0 + score2));
}

static void draw_sprites(void)
{
    uint8_t i;
    for (i = 0; i < 4; i++) {
        hal_set_sprite((uint8_t)(SPR_P1 + i), P1_X, (int16_t)(p1_y + (i << 3)), TILE_SOLID);
        hal_set_sprite((uint8_t)(SPR_P2 + i), P2_X, (int16_t)(p2_y + (i << 3)), TILE_SOLID);
    }
    hal_set_sprite(SPR_BALL, ball_x, ball_y, TILE_BALL);
}

static void reset_ball(void)
{
    ball_x  = (SCREEN_W - BALL_S) / 2;
    ball_y  = (FIELD_TOP + FIELD_BOTTOM - BALL_S) / 2;
    ball_vx = 0;
    ball_vy = 0;
    hits    = 0;
}

static void game_reset(void)
{
    score1 = 0;
    score2 = 0;
    p1_y = p2_y = (FIELD_TOP + FIELD_BOTTOM - PADDLE_H) / 2;
    serve_dir = 1;
    reset_ball();
    state = ST_SERVE;
    draw_hud();
}

static void serve(void)
{
    /* "Aleatorio" barato: el contador de cuadros en el instante de sacar */
    static const int8_t vy_tab[4] = { -1, 1, -2, 2 };
    ball_vx = (serve_dir > 0) ? 2 : -2;
    ball_vy = vy_tab[frame_count & 3];
    state = ST_PLAY;
}

static void point_for(uint8_t who)
{
    if (who == 1) { score1++; serve_dir =  1; }   /* la bola va hacia quien perdio */
    else          { score2++; serve_dir = -1; }
    draw_hud();
    reset_ball();
    state = (score1 >= WIN_SCORE || score2 >= WIN_SCORE) ? ST_OVER : ST_SERVE;
}

static uint8_t ball_hits(int16_t px, int16_t py)
{
    return (ball_x < px + PADDLE_W) && (ball_x + BALL_S > px) &&
           (ball_y < py + PADDLE_H) && (ball_y + BALL_S > py);
}

static void paddle_bounce(int16_t paddle_y, int8_t dir)
{
    int16_t off = (int16_t)((ball_y + BALL_S / 2) - (paddle_y + PADDLE_H / 2));
    int8_t  s;

    if (hits < 255) hits++;
    s = (int8_t)(2 + (hits >> 2));            /* +1 de velocidad cada 4 golpes */
    if (s > BALL_MAX_SPEED) s = BALL_MAX_SPEED;

    ball_vx = (dir > 0) ? s : (int8_t)-s;
    /* el angulo depende de donde pego en la raqueta (off/8, simetrico, sin dividir) */
    ball_vy = (off < 0) ? (int8_t)-((-off) >> 3) : (int8_t)(off >> 3);
}

static void move_player(uint8_t b)
{
    if (b & BTN_UP)   p1_y -= PLAYER_SPEED;
    if (b & BTN_DOWN) p1_y += PLAYER_SPEED;
    p1_y = clamp_paddle(p1_y);
}

static void move_cpu(void)
{
    if (ball_vx > 0) {                        /* solo reacciona si la bola viene hacia ella */
        int16_t target = ball_y + BALL_S / 2;
        int16_t center = p2_y + PADDLE_H / 2;
        if (target < center - 3)      p2_y -= CPU_SPEED;
        else if (target > center + 3) p2_y += CPU_SPEED;
        p2_y = clamp_paddle(p2_y);
    }
}

static void update_ball(void)
{
    ball_x += ball_vx;
    ball_y += ball_vy;

    /* paredes */
    if (ball_y < FIELD_TOP) {
        ball_y = FIELD_TOP;
        ball_vy = (int8_t)-ball_vy;
    } else if (ball_y > FIELD_BOTTOM - BALL_S) {
        ball_y = FIELD_BOTTOM - BALL_S;
        ball_vy = (int8_t)-ball_vy;
    }

    /* raquetas */
    if (ball_vx < 0 && ball_hits(P1_X, p1_y)) {
        ball_x = P1_X + PADDLE_W;
        paddle_bounce(p1_y, 1);
    } else if (ball_vx > 0 && ball_hits(P2_X, p2_y)) {
        ball_x = P2_X - BALL_S;
        paddle_bounce(p2_y, -1);
    }

    /* puntos */
    if (ball_x + BALL_S < 0)      point_for(2);
    else if (ball_x > SCREEN_W)   point_for(1);
}

void game_init(void)
{
    uint8_t x, y;

    hal_load_tiles(0, TILE_COUNT, game_tiles);

    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < MAP_W; x++)
            hal_set_bg(x, y, TILE_EMPTY);

    for (x = 0; x < MAP_W; x++) {
        hal_set_bg(x, 2, TILE_WALL);              /* pared superior */
        hal_set_bg(x, MAP_H - 1, TILE_WALL);      /* pared inferior */
    }
    for (y = 3; y < MAP_H - 1; y++)
        hal_set_bg(NET_TX, y, TILE_NET);          /* red central */

    game_reset();
}

void game_update(void)
{
    uint8_t b       = hal_buttons();
    uint8_t pressed = (uint8_t)(b & (uint8_t)~prev_btn);   /* flanco de subida */
    prev_btn = b;
    frame_count++;

    switch (state) {
    case ST_SERVE:
        move_player(b);
        if (pressed & (BTN_START | BTN_A)) serve();
        break;
    case ST_PLAY:
        move_player(b);
        move_cpu();
        update_ball();
        break;
    case ST_OVER:
        /* parpadea el marcador del ganador; START reinicia */
        if (score1 > score2)
            hal_set_bg(SCORE1_TX, SCORE_TY, (frame_count & 16) ? TILE_EMPTY : (uint8_t)(TILE_DIGIT0 + score1));
        else
            hal_set_bg(SCORE2_TX, SCORE_TY, (frame_count & 16) ? TILE_EMPTY : (uint8_t)(TILE_DIGIT0 + score2));
        if (pressed & BTN_START) game_reset();
        break;
    }

    draw_sprites();
}