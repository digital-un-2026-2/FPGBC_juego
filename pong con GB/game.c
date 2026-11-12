/*
 * game.c - Pong portable.
 *
 * Reglas para que compile igual en PC y en el FemtoRV:
 *  - Solo enteros (nada de float/double): el RV32I base no tiene FPU.
 *  - Sin malloc, sin printf, sin librerias del SO: solo <stdint.h>.
 *  - Sin multiplicar/dividir en el camino caliente (el RV32I base puede no
 *    tener extension M): se usan desplazamientos y tablas.
 *  - Todo el hardware pasa por hal.h.
 *
 * Flujo de pantallas:
 *
 *   TITULO --A/START--> SAQUE --A/START--> JUEGO --punto--> SAQUE
 *     ^                                     |  ^
 *     |                               START |  | START
 *     |                                     v  |
 *     +-------------SELECT-------------- PAUSA
 *     |
 *     +--SELECT/B-- FIN (GANASTE/PERDISTE) --START--> SAQUE (revancha)
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

/* Efectos visuales (en cuadros) */
#define SHAKE_FRAMES   6    /* la raqueta tiembla al golpear */
#define BLINK_FRAMES   24   /* la bola parpadea cuando acelera */

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

/* Filas de texto */
#define MENU_TX        7             /* columna de las opciones del menu */
#define MENU_TY0       8             /* fila de la primera opcion */
#define PAUSE_TY       8
#define OVER_TY        6

enum { ST_TITLE, ST_SERVE, ST_PLAY, ST_PAUSE, ST_OVER };

/* ------------------------------------------------------------------ */
/* Dificultad                                                          */
/* ------------------------------------------------------------------ */
enum { DIF_FACIL, DIF_NORMAL, DIF_DIFICIL, DIF_COUNT };

struct Dificultad {
    uint8_t vel_cpu;        /* velocidad normal de la raqueta de la CPU */
    uint8_t vel_perdiendo;  /* velocidad cuando la CPU va perdiendo por 2 o mas */
    uint8_t apunta;         /* 1 = busca pegar con la esquina para tiros diagonales */
    uint8_t salta_cuadros;  /* mascara: la CPU solo se mueve si (frame & mascara) == 0 */
    int8_t  vel_max_bola;   /* velocidad maxima de la bola */
};

static const struct Dificultad dificultades[DIF_COUNT] = {
    /* FACIL:   lenta, se mueve 1 de cada 2 cuadros, no apunta */
    { 1, 1, 0, 1, 3 },
    /* NORMAL:  el comportamiento original del juego */
    { 1, 2, 1, 0, 4 },
    /* DIFICIL: rapida, apunta siempre y la bola llega mas rapido */
    { 2, 3, 1, 0, 5 },
};

static const char *const nombres_dif[DIF_COUNT] = { "FACIL", "NORMAL", "DIFICIL" };

/* ------------------------------------------------------------------ */
/* Estado del juego                                                    */
/* ------------------------------------------------------------------ */
static int16_t p1_y, p2_y;
static int16_t ball_x, ball_y;
static int8_t  ball_vx, ball_vy;
static int8_t  serve_dir;
static uint8_t score1, score2;
static uint8_t state, hits;
static uint8_t frame_count, prev_btn;

static uint8_t dif_sel;                         /* opcion marcada en el menu */
static const struct Dificultad *dif;            /* dificultad de la partida */

static uint8_t p1_shake, p2_shake;              /* cuadros restantes de temblor */
static uint8_t ball_blink;                      /* cuadros restantes de parpadeo */

/* ------------------------------------------------------------------ */
/* Fondo y texto                                                       */
/* ------------------------------------------------------------------ */

/* Convierte un caracter en su tile. Letras A-Z, digitos 0-9; lo demas = vacio. */
static uint8_t char_tile(char c)
{
    if (c >= 'A' && c <= 'Z') return (uint8_t)(TILE_LETTER_A + (c - 'A'));
    if (c >= '0' && c <= '9') return (uint8_t)(TILE_DIGIT0 + (c - '0'));
    return TILE_EMPTY;
}

static void draw_text(uint8_t tx, uint8_t ty, const char *s)
{
    while (*s && tx < MAP_W) {
        hal_set_bg(tx, ty, char_tile(*s));
        tx++;
        s++;
    }
}

/* Repone una fila del campo tal como la dibuja draw_field() */
static void restore_row(uint8_t ty)
{
    uint8_t x;
    for (x = 0; x < MAP_W; x++)
        hal_set_bg(x, ty, TILE_EMPTY);
    if (ty == 2 || ty == MAP_H - 1) {
        for (x = 0; x < MAP_W; x++)
            hal_set_bg(x, ty, TILE_WALL);
    } else if (ty > 2 && ty < MAP_H - 1) {
        hal_set_bg(NET_TX, ty, TILE_NET);
    }
}

static void clear_screen(void)
{
    uint8_t x, y;
    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < MAP_W; x++)
            hal_set_bg(x, y, TILE_EMPTY);
}

static void draw_field(void)
{
    uint8_t y;
    for (y = 0; y < MAP_H; y++)
        restore_row(y);
}

static void draw_hud(void)
{
    hal_set_bg(SCORE1_TX, SCORE_TY, (uint8_t)(TILE_DIGIT0 + score1));
    hal_set_bg(SCORE2_TX, SCORE_TY, (uint8_t)(TILE_DIGIT0 + score2));
}

/* ------------------------------------------------------------------ */
/* Sprites                                                             */
/* ------------------------------------------------------------------ */

static void hide_all_sprites(void)
{
    uint8_t i;
    for (i = 0; i < MAX_SPRITES; i++)
        hal_hide_sprite(i);
}

static void draw_sprites(void)
{
    uint8_t i;
    /* Temblor: la raqueta se corre 1 px hacia afuera en cuadros alternos */
    int16_t x1 = (p1_shake & 2) ? (P1_X - 1) : P1_X;
    int16_t x2 = (p2_shake & 2) ? (P2_X + 1) : P2_X;

    for (i = 0; i < 4; i++) {
        hal_set_sprite((uint8_t)(SPR_P1 + i), x1, (int16_t)(p1_y + (i << 3)), TILE_SOLID);
        hal_set_sprite((uint8_t)(SPR_P2 + i), x2, (int16_t)(p2_y + (i << 3)), TILE_SOLID);
    }

    /* Parpadeo: la bola se oculta 4 cuadros de cada 8 mientras dura */
    if (state == ST_OVER || (ball_blink & 4))
        hal_hide_sprite(SPR_BALL);
    else
        hal_set_sprite(SPR_BALL, ball_x, ball_y, TILE_BALL);
}

/* ------------------------------------------------------------------ */
/* Pantalla de titulo y menu de dificultad                             */
/* ------------------------------------------------------------------ */

static void draw_menu_cursor(void)
{
    uint8_t i, ty = MENU_TY0;
    for (i = 0; i < DIF_COUNT; i++) {
        hal_set_bg(MENU_TX - 2, ty, (i == dif_sel) ? TILE_CURSOR : TILE_EMPTY);
        ty += 2;
    }
}

static void show_title(void)
{
    uint8_t i, ty = MENU_TY0;

    state = ST_TITLE;
    hide_all_sprites();
    clear_screen();

    draw_text(6, 2, "P O N G");
    draw_text(7, 4, "FPGBC");
    for (i = 0; i < DIF_COUNT; i++) {
        draw_text(MENU_TX, ty, nombres_dif[i]);
        ty += 2;
    }
    draw_text(5, 15, "PULSA START");
    draw_menu_cursor();

    /* La bola rebota por la pantalla como demostracion */
    ball_x  = 16;
    ball_y  = 40;
    ball_vx = 1;
    ball_vy = 1;
}

static void update_title(uint8_t pressed)
{
    if ((pressed & BTN_DOWN) && dif_sel < DIF_COUNT - 1) { dif_sel++; draw_menu_cursor(); }
    if ((pressed & BTN_UP)   && dif_sel > 0)             { dif_sel--; draw_menu_cursor(); }

    /* Bola de demostracion */
    ball_x += ball_vx;
    ball_y += ball_vy;
    if (ball_x <= 0 || ball_x >= SCREEN_W - BALL_S) ball_vx = (int8_t)-ball_vx;
    if (ball_y <= 0 || ball_y >= SCREEN_H - BALL_S) ball_vy = (int8_t)-ball_vy;
    hal_set_sprite(SPR_BALL, ball_x, ball_y, TILE_BALL);
}

/* ------------------------------------------------------------------ */
/* Partida                                                             */
/* ------------------------------------------------------------------ */

static int16_t clamp_paddle(int16_t y)
{
    if (y < FIELD_TOP) return FIELD_TOP;
    if (y > FIELD_BOTTOM - PADDLE_H) return FIELD_BOTTOM - PADDLE_H;
    return y;
}

static void reset_ball(void)
{
    ball_x     = (SCREEN_W - BALL_S) / 2;
    ball_y     = (FIELD_TOP + FIELD_BOTTOM - BALL_S) / 2;
    ball_vx    = 0;
    ball_vy    = 0;
    hits       = 0;
    ball_blink = 0;
}

static void game_reset(void)
{
    dif    = &dificultades[dif_sel];
    score1 = 0;
    score2 = 0;
    p1_y = p2_y = (FIELD_TOP + FIELD_BOTTOM - PADDLE_H) / 2;
    p1_shake = p2_shake = 0;
    serve_dir = 1;
    reset_ball();
    state = ST_SERVE;
    draw_field();
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

static void show_game_over(void)
{
    state = ST_OVER;
    if (score1 > score2)
        draw_text(6, OVER_TY, "GANASTE");
    else
        draw_text(6, OVER_TY, "PERDISTE");
    draw_text(5, OVER_TY + 4, "START OTRA");
    draw_text(4, OVER_TY + 6, "SELECT MENU");
}

static void point_for(uint8_t who)
{
    if (who == 1) { score1++; serve_dir =  1; }   /* la bola va hacia quien perdio */
    else          { score2++; serve_dir = -1; }
    draw_hud();
    reset_ball();
    if (score1 >= WIN_SCORE || score2 >= WIN_SCORE)
        show_game_over();
    else
        state = ST_SERVE;
}

static uint8_t ball_hits(int16_t px, int16_t py)
{
    return (ball_x < px + PADDLE_W) && (ball_x + BALL_S > px) &&
           (ball_y < py + PADDLE_H) && (ball_y + BALL_S > py);
}

static void paddle_bounce(int16_t paddle_y, int8_t dir)
{
    int16_t off = (int16_t)((ball_y + BALL_S / 2) - (paddle_y + PADDLE_H / 2));
    int8_t  s, prev_s;

    prev_s = (ball_vx < 0) ? (int8_t)-ball_vx : ball_vx;

    if (hits < 255) hits++;
    s = (int8_t)(2 + (hits >> 2));            /* +1 de velocidad cada 4 golpes */
    if (s > dif->vel_max_bola) s = dif->vel_max_bola;
    if (s > prev_s) ball_blink = BLINK_FRAMES; /* aviso visual: la bola acelero */

    ball_vx = (dir > 0) ? s : (int8_t)-s;
    /* el angulo depende de donde pego en la raqueta (off/8, simetrico, sin dividir) */
    ball_vy = (off < 0) ? (int8_t)-((-off) >> 3) : (int8_t)(off >> 3);

    /* Un golpe justo en el centro dejaba la bola totalmente horizontal y, si las
     * dos raquetas pegaban en el centro, el peloteo no terminaba nunca.
     * Se le da una inclinacion minima en un sentido "aleatorio". */
    if (ball_vy == 0) ball_vy = (frame_count & 1) ? 1 : -1;
}

static void move_player(uint8_t b)
{
    if (b & BTN_UP)   p1_y -= PLAYER_SPEED;
    if (b & BTN_DOWN) p1_y += PLAYER_SPEED;
    p1_y = clamp_paddle(p1_y);
}

static void move_cpu(void)
{
    int16_t target, center;
    int8_t  speed;
    /* score2 = CPU, score1 = Jugador */
    int8_t  score_diff = (int8_t)((int8_t)score2 - (int8_t)score1);

    /* En FACIL la CPU "pierde" cuadros: solo se mueve 1 de cada 2 */
    if (frame_count & dif->salta_cuadros) return;

    /* 1. Velocidad segun el marcador */
    if (score_diff >= 3)       speed = 1;                        /* se relaja si va ganando por mucho */
    else if (score_diff <= -2) speed = (int8_t)dif->vel_perdiendo; /* se esfuerza si va perdiendo */
    else                       speed = (int8_t)dif->vel_cpu;

    if (ball_vx > 0) {
        /* 2. ATAQUE: la bola viene hacia la CPU */
        int16_t offset = 0;

        if (dif->apunta && score_diff < 3) {
            /* Pone el centro de la raqueta 8 px por debajo (o por encima) de la bola:
             * la bola pega en la mitad de ARRIBA (o de abajo) y sale en diagonal
             * hacia arriba (o hacia abajo), lejos del centro del campo. */
            if (ball_y < (FIELD_TOP + FIELD_BOTTOM) / 2)
                offset = 8;   /* bola en la mitad superior -> la manda hacia arriba */
            else
                offset = -8;  /* bola en la mitad inferior -> la manda hacia abajo */
        }
        target = (int16_t)(ball_y + BALL_S / 2 + offset);
    } else {
        /* 3. REPOSICIONAMIENTO: cuando la bola se aleja, vuelve tranquila al centro */
        target = (FIELD_TOP + FIELD_BOTTOM) / 2;
        speed  = 1;
    }

    center = (int16_t)(p2_y + PADDLE_H / 2);
    if (target < center - 2)
        p2_y -= speed;
    else if (target > center + 2)
        p2_y += speed;

    p2_y = clamp_paddle(p2_y);
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
        p1_shake = SHAKE_FRAMES;
    } else if (ball_vx > 0 && ball_hits(P2_X, p2_y)) {
        ball_x = P2_X - BALL_S;
        paddle_bounce(p2_y, -1);
        p2_shake = SHAKE_FRAMES;
    }

    /* puntos */
    if (ball_x + BALL_S < 0)      point_for(2);
    else if (ball_x > SCREEN_W)   point_for(1);
}

static void update_effects(void)
{
    if (p1_shake)   p1_shake--;
    if (p2_shake)   p2_shake--;
    if (ball_blink) ball_blink--;
}

/* ------------------------------------------------------------------ */
/* Interfaz publica                                                    */
/* ------------------------------------------------------------------ */

void game_init(void)
{
    hal_load_tiles(0, TILE_COUNT, game_tiles);
    dif_sel = DIF_NORMAL;
    show_title();
}

void game_update(void)
{
    uint8_t b       = hal_buttons();
    uint8_t pressed = (uint8_t)(b & (uint8_t)~prev_btn);   /* flanco de subida */
    prev_btn = b;
    frame_count++;

    switch (state) {
    case ST_TITLE:
        update_title(pressed);
        if (pressed & (BTN_START | BTN_A)) game_reset();
        return;                                   /* en el titulo no hay raquetas */

    case ST_SERVE:
        move_player(b);
        if (pressed & (BTN_START | BTN_A)) serve();
        break;

    case ST_PLAY:
        if (pressed & BTN_START) {                /* pausa */
            state = ST_PAUSE;
            draw_text(8, PAUSE_TY, "PAUSA");
            draw_text(4, PAUSE_TY + 3, "SELECT MENU");
            break;
        }
        move_player(b);
        move_cpu();
        update_ball();
        update_effects();
        break;

    case ST_PAUSE:
        if (pressed & BTN_START) {                /* continuar */
            restore_row(PAUSE_TY);
            restore_row(PAUSE_TY + 3);
            state = ST_PLAY;
        } else if (pressed & BTN_SELECT) {
            show_title();
            return;
        }
        break;

    case ST_OVER:
        /* parpadea el marcador del ganador; START revancha, SELECT o B al menu */
        if (score1 > score2)
            hal_set_bg(SCORE1_TX, SCORE_TY, (frame_count & 16) ? TILE_EMPTY : (uint8_t)(TILE_DIGIT0 + score1));
        else
            hal_set_bg(SCORE2_TX, SCORE_TY, (frame_count & 16) ? TILE_EMPTY : (uint8_t)(TILE_DIGIT0 + score2));
        if (pressed & BTN_START) {
            game_reset();
        } else if (pressed & (BTN_SELECT | BTN_B)) {
            show_title();
            return;
        }
        break;
    }

    draw_sprites();
}
