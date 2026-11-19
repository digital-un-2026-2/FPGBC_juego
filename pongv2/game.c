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
 * Botones en todo el juego: START = elegir / sacar / pausar / continuar,
 * SELECT = volver. Las flechas mueven el cursor y la raqueta de J1. A y B solo
 * se usan en el 1 VS 1 LOCAL, como flechas de J2 (A sube, B baja).
 *
 * Menu por secciones:
 *
 *   TITULO: CONTRA IA / 1 VS 1
 *     |-- CONTRA IA --> FACIL / NORMAL / DIFICIL --START--> partida contra la CPU
 *     '-- 1 VS 1 ----> LOCAL / CABLE
 *                        |-- LOCAL --START--> partida de dos en la misma consola
 *                        '-- CABLE --START--> CONECTANDO --> partida entre dos consolas
 *   (SELECT en una seccion vuelve a la anterior)
 *
 * Partida (igual en todos los modos):
 *
 *   SAQUE --START--> JUEGO --punto--> SAQUE
 *                     |  ^
 *               START |  | START
 *                     v  |
 *                    PAUSA --SELECT--> TITULO
 *
 *   FIN (GANASTE/PERDISTE o GANA J1/J2) --START--> SAQUE (revancha)
 *                                       --SELECT-> TITULO
 *
 * 1 VS 1 por CABLE (dos consolas unidas por el cable de enlace, ver link.h):
 *   - Las dos consolas se saludan con un numero al azar. La del numero mayor
 *     es el MAESTRO (J1, raqueta izquierda): ella sola calcula la partida.
 *   - La otra es el ESCLAVO (J2, raqueta derecha): cada cuadro le manda sus
 *     botones al maestro y dibuja el estado que el maestro le devuelve.
 *   - Cualquiera de los dos puede sacar, pausar, pedir revancha o volver al menu.
 *   - SELECT cancela la busqueda; 2 s sin datos = SIN CONEXION.
 */
#include "game.h"
#include "hal.h"
#include "tiles.h"
#include "link.h"

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

/* 1 VS 1: cuadros sin recibir nada antes de decir "SIN CONEXION" (2 s) */
#define LINK_TIMEOUT   120

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

enum { ST_TITLE, ST_SERVE, ST_PLAY, ST_PAUSE, ST_OVER, ST_LINK_WAIT, ST_LOST };

/* Quien calcula la partida */
enum { ROL_CPU,       /* un jugador contra la CPU                          */
       ROL_LOCAL,     /* 1 VS 1 LOCAL: dos jugadores en la misma consola   */
       ROL_MAESTRO,   /* 1 VS 1 CABLE: esta consola es J1 y calcula todo   */
       ROL_ESCLAVO }; /* 1 VS 1 CABLE: esta consola es J2 y solo dibuja    */

/* Botones de J2 en el modo LOCAL */
#define BTN_J2_UP    BTN_A
#define BTN_J2_DOWN  BTN_B

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

/* Secciones del menu */
enum { PAG_MODO, PAG_IA, PAG_1VS1 };
enum { MODO_IA, MODO_1VS1, MODO_COUNT };
enum { ENL_LOCAL, ENL_CABLE, ENL_COUNT };

static const char *const nombres_modo[MODO_COUNT]  = { "CONTRA IA", "1 VS 1" };
static const char *const nombres_dif[DIF_COUNT]    = { "FACIL", "NORMAL", "DIFICIL" };
static const char *const nombres_enl[ENL_COUNT]    = { "LOCAL", "CABLE" };

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

static uint8_t menu_pag;                        /* seccion del menu que se ve */
static uint8_t modo_sel, dif_sel, enl_sel;      /* opcion marcada en cada seccion */
static const struct Dificultad *dif;            /* dificultad de la partida */

static uint8_t p1_shake, p2_shake;              /* cuadros restantes de temblor */
static uint8_t ball_blink;                      /* cuadros restantes de parpadeo */

/* 1 VS 1 */
static uint8_t  rol;                            /* ROL_CPU, ROL_LOCAL, ROL_MAESTRO o ROL_ESCLAVO */
static uint16_t azar = 0xACE1;                  /* numero pseudoaleatorio (xorshift) */
static uint16_t mi_id;                          /* numero con el que se saluda */
static uint8_t  remoto_btn, remoto_prev;        /* botones del esclavo (en el maestro) */
static uint8_t  sin_datos;                      /* cuadros sin recibir paquetes */
static uint8_t  seq_tx;                         /* contador de paquetes enviados */

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
    /* En 1 VS 1 cada consola marca con "TU" su propio marcador */
    if (rol == ROL_MAESTRO)      draw_text(SCORE1_TX, 0, "TU");
    else if (rol == ROL_ESCLAVO) draw_text(SCORE2_TX, 0, "TU");
    else if (rol == ROL_LOCAL) {                    /* en LOCAL: quien es quien */
        draw_text(SCORE1_TX - 1, 0, "J1");
        draw_text(SCORE2_TX, 0, "J2");
    }
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

/* Opciones y cursor de la seccion actual */
static uint8_t menu_count(void)
{
    if (menu_pag == PAG_IA)   return DIF_COUNT;
    if (menu_pag == PAG_1VS1) return ENL_COUNT;
    return MODO_COUNT;
}

static uint8_t *menu_sel(void)
{
    if (menu_pag == PAG_IA)   return &dif_sel;
    if (menu_pag == PAG_1VS1) return &enl_sel;
    return &modo_sel;
}

static const char *menu_name(uint8_t i)
{
    if (menu_pag == PAG_IA)   return nombres_dif[i];
    if (menu_pag == PAG_1VS1) return nombres_enl[i];
    return nombres_modo[i];
}

static void draw_menu_cursor(void)
{
    uint8_t i, ty = MENU_TY0, n = menu_count(), sel = *menu_sel();
    for (i = 0; i < n; i++) {
        hal_set_bg(MENU_TX - 2, ty, (i == sel) ? TILE_CURSOR : TILE_EMPTY);
        ty += 2;
    }
}

/* Dibuja la seccion 'pag' del menu (la bola de demostracion sigue rebotando) */
static void show_menu_page(uint8_t pag)
{
    uint8_t i, ty = MENU_TY0, n;

    menu_pag = pag;
    clear_screen();
    draw_text(6, 2, "P O N G");
    draw_text(7, 4, "FPGBC");

    n = menu_count();
    for (i = 0; i < n; i++) {
        draw_text(MENU_TX, ty, menu_name(i));
        ty += 2;
    }
    if (pag == PAG_MODO) {
        draw_text(5, 16, "PULSA START");
    } else {
        draw_text((pag == PAG_IA) ? 5 : 7, 6, (pag == PAG_IA) ? "CONTRA LA IA" : "1 VS 1");
        draw_text(4, 15, "START JUGAR");
        draw_text(3, 16, "SELECT VOLVER");
    }
    draw_menu_cursor();
}

static void show_title(void)
{
    state = ST_TITLE;
    rol   = ROL_CPU;
    hide_all_sprites();
    show_menu_page(PAG_MODO);

    /* La bola rebota por la pantalla como demostracion */
    ball_x  = 16;
    ball_y  = 40;
    ball_vx = 1;
    ball_vy = 1;
}

static void update_title(uint8_t pressed)
{
    uint8_t *sel = menu_sel();

    if ((pressed & BTN_DOWN) && *sel < menu_count() - 1) { (*sel)++; draw_menu_cursor(); }
    if ((pressed & BTN_UP)   && *sel > 0)                { (*sel)--; draw_menu_cursor(); }

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
    /* En 1 VS 1 no hay CPU: se usa la velocidad de bola de NORMAL */
    dif    = &dificultades[(rol == ROL_CPU) ? dif_sel : DIF_NORMAL];
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
    /* "Mi" marcador: J1 en modo CPU y en el maestro; J2 en el esclavo */
    uint8_t mio  = (rol == ROL_ESCLAVO) ? score2 : score1;
    uint8_t otro = (rol == ROL_ESCLAVO) ? score1 : score2;

    state = ST_OVER;
    if (rol == ROL_LOCAL)               /* en la misma consola no hay "yo" */
        draw_text(6, OVER_TY, (score1 > score2) ? "GANA J1" : "GANA J2");
    else if (mio > otro)
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

static void move_paddle(int16_t *py, uint8_t b)
{
    if (b & BTN_UP)   *py -= PLAYER_SPEED;
    if (b & BTN_DOWN) *py += PLAYER_SPEED;
    *py = clamp_paddle(*py);
}

static void move_player(uint8_t b)
{
    move_paddle(&p1_y, b);
}

/* Raqueta de J2 cuando la mueve una persona (no la CPU) */
static void move_p2(uint8_t b)
{
    if (rol == ROL_LOCAL) {
        /* misma consola: A sube y B baja */
        uint8_t b2 = 0;
        if (b & BTN_J2_UP)   b2 |= BTN_UP;
        if (b & BTN_J2_DOWN) b2 |= BTN_DOWN;
        move_paddle(&p2_y, b2);
    } else if (rol == ROL_MAESTRO) {
        move_paddle(&p2_y, remoto_btn);   /* las flechas de la otra consola */
    }
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

/* Parpadea el marcador del ganador en la pantalla de fin */
static void blink_winner(void)
{
    if (score1 > score2)
        hal_set_bg(SCORE1_TX, SCORE_TY, (frame_count & 16) ? TILE_EMPTY : (uint8_t)(TILE_DIGIT0 + score1));
    else
        hal_set_bg(SCORE2_TX, SCORE_TY, (frame_count & 16) ? TILE_EMPTY : (uint8_t)(TILE_DIGIT0 + score2));
}

static void show_pause(void)
{
    draw_text(8, PAUSE_TY, "PAUSA");
    draw_text(4, PAUSE_TY + 3, "SELECT MENU");
}

static void hide_pause(void)
{
    restore_row(PAUSE_TY);
    restore_row(PAUSE_TY + 3);
}

/* ------------------------------------------------------------------ */
/* 1 VS 1 por el cable de enlace                                       */
/* ------------------------------------------------------------------ */

/* xorshift de 16 bits (solo desplazamientos y XOR, sin multiplicar).
 * Se mezcla con los botones de cada cuadro: dos consolas casi nunca
 * llegan al mismo numero aunque se prendan al mismo tiempo. */
static void stir_random(uint8_t b)
{
    azar ^= (uint16_t)(azar << 7);
    azar ^= (uint16_t)(azar >> 9);
    azar ^= (uint16_t)(azar << 8);
    azar ^= (uint16_t)(b | ((uint16_t)frame_count << 8));
    if (azar == 0) azar = 0xACE1;
}

static void new_link_id(void)
{
    stir_random(0);
    mi_id = azar;
}

/* Descarta todo lo que haya llegado por el cable */
static void link_drain(void)
{
    uint8_t buf[LINK_MAX_DATOS];
    link_reset();
    while (link_poll(buf))
        ;
}

static void show_link_wait(void)
{
    state = ST_LINK_WAIT;
    hide_all_sprites();
    clear_screen();
    draw_text(7, 2, "1 VS 1");
    draw_text(5, 6, "CONECTANDO");
    draw_text(2, 10, "UNE LAS CONSOLAS");
    draw_text(2, 11, "CON EL CABLE");
    draw_text(3, 16, "SELECT CANCELA");
    link_drain();               /* restos de una partida anterior */
    new_link_id();
}

static void show_link_lost(void)
{
    state = ST_LOST;
    rol   = ROL_CPU;
    hide_all_sprites();
    clear_screen();
    draw_text(4, 6, "SIN CONEXION");
    draw_text(2, 10, "REVISA EL CABLE");
    draw_text(4, 14, "SELECT MENU");
}

static void start_1vs1(uint8_t nuevo_rol)
{
    rol         = nuevo_rol;
    remoto_btn  = 0;
    remoto_prev = 0;
    sin_datos   = 0;
    seq_tx      = 0;
    game_reset();               /* campo, marcador "TU", estado SAQUE */
}

/* Paquete de estado del maestro: todo lo que el esclavo necesita para dibujar */
static void send_state(uint8_t st)
{
    uint8_t d[LINK_LEN_ESTADO];
    d[0] = st;
    d[1] = (uint8_t)p1_y;
    d[2] = (uint8_t)p2_y;
    d[3] = (uint8_t)(ball_x + 32);          /* la bola puede estar un poco fuera */
    d[4] = (uint8_t)(ball_y + 32);          /* de la pantalla: se corre +32      */
    d[5] = score1;
    d[6] = score2;
    d[7] = (uint8_t)(((p1_shake & 2) ? 1 : 0) | ((p2_shake & 2) ? 2 : 0) | ((ball_blink & 4) ? 4 : 0));
    d[8] = seq_tx++;
    d[9] = 0;                               /* libre (sonido, etc.) */
    link_send(LINK_ESTADO, d);
}

/* Vuelve al titulo; si es el maestro, le avisa al esclavo */
static void go_to_title(void)
{
    if (rol == ROL_MAESTRO) {
        send_state(ST_TITLE);
        send_state(ST_TITLE);   /* dos veces por si una se pierde */
    }
    show_title();
}

static void update_link_wait(uint8_t pressed)
{
    uint8_t d[LINK_MAX_DATOS], t;

    if (pressed & BTN_SELECT) { show_title(); return; }

    /* Saluda 8 veces por segundo */
    if ((frame_count & 7) == 0) {
        d[0] = (uint8_t)(mi_id >> 8);
        d[1] = (uint8_t)mi_id;
        link_send(LINK_HOLA, d);
    }

    while ((t = link_poll(d)) != 0) {
        if (t == LINK_HOLA) {
            uint16_t otro = (uint16_t)((d[0] << 8) | d[1]);
            if (otro == mi_id) { new_link_id(); continue; }   /* empate: otro numero */
            start_1vs1((mi_id > otro) ? ROL_MAESTRO : ROL_ESCLAVO);
            return;
        }
        /* Si ya llegan paquetes de juego, el otro ya decidio su rol */
        if (t == LINK_ESTADO)  { start_1vs1(ROL_ESCLAVO); return; }
        if (t == LINK_BOTONES) { start_1vs1(ROL_MAESTRO); return; }
    }
}

/* Maestro: lee los botones que manda el esclavo.
 * Devuelve 0 si se perdio la conexion. */
static uint8_t read_slave(void)
{
    uint8_t d[LINK_MAX_DATOS], t;

    if (sin_datos < 255) sin_datos++;
    while ((t = link_poll(d)) != 0) {
        if (t == LINK_BOTONES) {
            remoto_btn = d[1];
            sin_datos  = 0;
        }
    }
    if (sin_datos > LINK_TIMEOUT) {
        show_link_lost();
        return 0;
    }
    return 1;
}

/* Esclavo: copia el estado del maestro y redibuja lo que cambio */
static void apply_state(const uint8_t *d)
{
    uint8_t nuevo = d[0];

    if (nuevo == ST_TITLE) { show_title(); return; }   /* el maestro salio al menu */
    if (nuevo > ST_OVER) return;                       /* estado desconocido */

    p1_y   = d[1];
    p2_y   = d[2];
    ball_x = (int16_t)d[3] - 32;
    ball_y = (int16_t)d[4] - 32;
    if (d[5] != score1 || d[6] != score2) {
        score1 = d[5];
        score2 = d[6];
        draw_hud();
    }
    p1_shake   = (d[7] & 1) ? 2 : 0;
    p2_shake   = (d[7] & 2) ? 2 : 0;
    ball_blink = (d[7] & 4) ? 4 : 0;

    if (nuevo != state) {
        if (state == ST_PAUSE) hide_pause();
        if (state == ST_OVER) { draw_field(); draw_hud(); }   /* revancha */
        if (nuevo == ST_PAUSE) show_pause();
        state = nuevo;
        if (nuevo == ST_OVER) show_game_over();
    }
}

static void update_slave(uint8_t b)
{
    uint8_t d[LINK_MAX_DATOS], t, llego = 0;

    /* 1. Mandar mis botones (todos los cuadros, aunque no cambien) */
    d[0] = seq_tx++;
    d[1] = b;
    link_send(LINK_BOTONES, d);

    /* 2. Aplicar lo que mando el maestro */
    while (rol == ROL_ESCLAVO && (t = link_poll(d)) != 0) {
        if (t == LINK_ESTADO) { apply_state(d); llego = 1; }
    }
    if (rol != ROL_ESCLAVO) { link_drain(); return; }  /* volvio al titulo */

    if (llego) sin_datos = 0;
    else if (++sin_datos > LINK_TIMEOUT) { show_link_lost(); return; }

    if (state == ST_OVER) blink_winner();
    draw_sprites();
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
    uint8_t rp      = 0;                                   /* lo que presiono J2 (1 VS 1) */
    prev_btn = b;
    frame_count++;
    stir_random(b);

    /* 1 VS 1: el esclavo no calcula nada, solo manda botones y dibuja */
    if (rol == ROL_ESCLAVO) {
        update_slave(b);
        return;
    }
    /* 1 VS 1: el maestro junta los botones de J2 con los suyos */
    if (rol == ROL_MAESTRO) {
        if (!read_slave()) return;
        rp = (uint8_t)(remoto_btn & (uint8_t)~remoto_prev);
        remoto_prev = remoto_btn;
    }

    switch (state) {
    case ST_TITLE:
        link_drain();                             /* nadie escucha el cable aqui */
        update_title(pressed);
        if (pressed & BTN_START) {
            if (menu_pag == PAG_MODO)
                show_menu_page((modo_sel == MODO_IA) ? PAG_IA : PAG_1VS1);
            else if (menu_pag == PAG_IA)
                game_reset();                     /* contra la CPU */
            else if (enl_sel == ENL_LOCAL)
                { rol = ROL_LOCAL; game_reset(); }
            else
                show_link_wait();                 /* por cable */
        } else if ((pressed & BTN_SELECT) && menu_pag != PAG_MODO) {
            show_menu_page(PAG_MODO);
        }
        return;                                   /* en el titulo no hay raquetas */

    case ST_LINK_WAIT:
        update_link_wait(pressed);
        return;

    case ST_LOST:
        link_drain();
        if (pressed & (BTN_SELECT | BTN_START)) show_title();
        return;

    case ST_SERVE:
        move_player(b);
        move_p2(b);
        if ((pressed | rp) & BTN_START) serve();
        break;

    case ST_PLAY:
        if ((pressed | rp) & BTN_START) {         /* pausa (cualquiera de los dos) */
            state = ST_PAUSE;
            show_pause();
            break;
        }
        move_player(b);
        if (rol == ROL_CPU) move_cpu();
        else                move_p2(b);
        update_ball();
        update_effects();
        break;

    case ST_PAUSE:
        if ((pressed | rp) & BTN_START) {         /* continuar */
            hide_pause();
            state = ST_PLAY;
        } else if ((pressed | rp) & BTN_SELECT) {
            go_to_title();
            return;
        }
        break;

    case ST_OVER:
        /* parpadea el marcador del ganador; START revancha, SELECT al menu */
        blink_winner();
        if ((pressed | rp) & BTN_START) {
            game_reset();
        } else if ((pressed | rp) & BTN_SELECT) {
            go_to_title();
            return;
        }
        break;
    }

    draw_sprites();
    if (rol == ROL_MAESTRO) send_state(state);
}
