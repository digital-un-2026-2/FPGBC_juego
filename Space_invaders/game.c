#include <gb/gb.h>
#include <gb/cgb.h>
#include "game.h"
#include "levels.h"
#include "sound.h"
#include "tiles.h"

#define SPR_OAM_PLAYER_L   0U
#define SPR_OAM_PLAYER_R   1U
#define SPR_OAM_UFO_L      2U
#define SPR_OAM_UFO_R      3U
#define SPR_OAM_PSHOT_0    4U
#define SPR_OAM_ESHOT_0    7U
#define SPR_OAM_FX_0       10U
#define SPR_OAM_ENEMY_BASE 13U
#define MAX_FX             3U

#define UFO_Y              12
#define MAX_NORMAL_PSHOTS  2U

struct FxSlot {
    int16_t x;
    int16_t y;
    uint8_t timer;
};

uint8_t  g_state;
uint8_t  g_level;
uint8_t  g_lives;
uint16_t g_score;
uint16_t g_high_score;
uint8_t  g_max_wave_reached;
int16_t  g_player_x;
uint8_t  g_player_speed;
uint8_t  g_player_invuln;
uint8_t  g_special_charge;
uint8_t  g_ufo_active;
int16_t  g_ufo_x;
struct Projectile g_p_shots[MAX_P_SHOTS];
struct Projectile g_e_shots[MAX_E_SHOTS];

static uint8_t  prev_joy;
static uint8_t  menu_opt;
static uint16_t frame_cnt;
static uint16_t rng_state = 0x5A3CU;
static uint8_t  state_timer;
static uint8_t  fire_cd;
static uint8_t  player_fire_anim;
static int8_t   form_dir;
static uint8_t  form_timer;
static uint8_t  anim_frame;
static uint8_t  enemy_fire_timer;
static uint8_t  diver_timer;
static uint16_t ufo_spawn_timer;
static int8_t   ufo_vx;
static struct FxSlot fx_pool[MAX_FX];

static uint8_t next_rng(void)
{
    rng_state ^= (uint16_t)(DIV_REG ^ frame_cnt);
    rng_state = (uint16_t)((rng_state << 3) | (rng_state >> 13));
    rng_state += 0x6D2BU;
    return (uint8_t)(rng_state & 0xFFU);
}

static void set_oam_sprite(uint8_t id, int16_t x, int16_t y, uint8_t tile, uint8_t pal)
{
    if (x > -8 && x < 160 && y > -8 && y < 144) {
        set_sprite_tile(id, tile);
        set_sprite_prop(id, pal);
        move_sprite(id, (uint8_t)(x + 8), (uint8_t)(y + 16));
    } else {
        hide_sprite(id);
    }
}

static void hide_all_sprites(void)
{
    uint8_t i;
    for (i = 0U; i < 40U; i++) {
        hide_sprite(i);
    }
}

static void clear_projectiles_only(void)
{
    uint8_t i;
    for (i = 0U; i < MAX_P_SHOTS; i++) {
        g_p_shots[i].active = 0U;
        hide_sprite((uint8_t)(SPR_OAM_PSHOT_0 + i));
    }
    for (i = 0U; i < MAX_E_SHOTS; i++) {
        g_e_shots[i].active = 0U;
        hide_sprite((uint8_t)(SPR_OAM_ESHOT_0 + i));
    }
    g_ufo_active = 0U;
    hide_sprite(SPR_OAM_UFO_L);
    hide_sprite(SPR_OAM_UFO_R);
}

static void clear_projectiles_and_fx(void)
{
    uint8_t i;
    clear_projectiles_only();
    for (i = 0U; i < MAX_FX; i++) {
        fx_pool[i].timer = 0U;
        hide_sprite((uint8_t)(SPR_OAM_FX_0 + i));
    }
}

static void spawn_fx(int16_t x, int16_t y)
{
    uint8_t i;
    uint8_t best = 0U;
    uint8_t min_t = 255U;

    for (i = 0U; i < MAX_FX; i++) {
        if (fx_pool[i].timer == 0U) {
            best = i;
            break;
        }
        if (fx_pool[i].timer < min_t) {
            min_t = fx_pool[i].timer;
            best = i;
        }
    }
    fx_pool[best].x = x;
    fx_pool[best].y = y;
    fx_pool[best].timer = 10U;
}

static void update_fx(void)
{
    uint8_t i;
    for (i = 0U; i < MAX_FX; i++) {
        uint8_t id = (uint8_t)(SPR_OAM_FX_0 + i);
        if (fx_pool[i].timer == 0U) {
            hide_sprite(id);
        } else {
            uint8_t tile = (fx_pool[i].timer > 5U) ? SPR_EXP_0 : SPR_EXP_1;
            set_oam_sprite(id, fx_pool[i].x, fx_pool[i].y, tile, PAL_SPR_YELLOW);
            fx_pool[i].timer--;
        }
    }
}

static void draw_hud_score(void)
{
    gfx_draw_number(2U, 0U, g_score, 5U, PAL_BG_UI);
    gfx_draw_number(11U, 0U, g_high_score, 5U, PAL_BG_YELLOW);
}

static void draw_hud_lives(void)
{
    uint8_t i;
    for (i = 0U; i < INIT_LIVES; i++) {
        uint8_t tx = (uint8_t)(2U + i);
        if (i < g_lives) {
            gfx_set_tile(tx, 17U, TILE_LIFE, PAL_BG_GREEN);
        } else {
            gfx_set_tile(tx, 17U, TILE_EMPTY, PAL_BG_UI);
        }
    }
}

static void draw_hud_special(void)
{
    uint8_t i;
    uint8_t pal = (g_special_charge >= 4U) ? PAL_BG_YELLOW : PAL_BG_CYAN;
    for (i = 0U; i < 4U; i++) {
        uint8_t tx = (uint8_t)(10U + i);
        if (i < g_special_charge) {
            gfx_set_tile(tx, 17U, TILE_BAR_FULL, pal);
        } else {
            gfx_set_tile(tx, 17U, TILE_BAR_EMPTY, PAL_BG_UI);
        }
    }
    gfx_draw_text(15U, 17U, (g_special_charge >= 4U) ? "OK!" : "   ", PAL_BG_YELLOW);
}

static void draw_hud_full(void)
{
    uint8_t x;
    for (x = 0U; x < 20U; x++) {
        gfx_set_tile(x, 0U, TILE_EMPTY, PAL_BG_UI);
        gfx_set_tile(x, 17U, TILE_EMPTY, PAL_BG_UI);
    }
    gfx_draw_text(0U, 0U, "P:", PAL_BG_UI);
    gfx_draw_text(8U, 0U, "HI:", PAL_BG_YELLOW);
    gfx_draw_text(17U, 0U, "N:", PAL_BG_CYAN);
    gfx_draw_number(19U, 0U, (uint16_t)(g_level + 1U), 1U, PAL_BG_CYAN);
    draw_hud_score();

    gfx_draw_text(0U, 17U, "V:", PAL_BG_UI);
    draw_hud_lives();
    gfx_draw_text(7U, 17U, "SP:", PAL_BG_CYAN);
    draw_hud_special();
}

static void add_score(uint16_t pts)
{
    if (pts >= 65000U || g_score > (uint16_t)(65000U - pts)) {
        g_score = 65000U;
    } else {
        g_score = (uint16_t)(g_score + pts);
    }
    if (g_score > g_high_score) {
        g_high_score = g_score;
    }
    draw_hud_score();
}

static void render_player(void)
{
    uint8_t firing;
    if (g_state == STATE_PLAYER_HIT || (g_player_invuln & 4U)) {
        hide_sprite(SPR_OAM_PLAYER_L);
        hide_sprite(SPR_OAM_PLAYER_R);
        return;
    }
    firing = (player_fire_anim > 0U) ? 1U : 0U;
    if (player_fire_anim > 0U) player_fire_anim--;

    set_oam_sprite(SPR_OAM_PLAYER_L, g_player_x, PLAYER_Y,
                   firing ? SPR_SHIP_FIRE_L : SPR_SHIP_L, PAL_SPR_PLAYER);
    set_oam_sprite(SPR_OAM_PLAYER_R, g_player_x + 8, PLAYER_Y,
                   firing ? SPR_SHIP_FIRE_R : SPR_SHIP_R, PAL_SPR_PLAYER);
}

static void render_enemies_and_boss(void)
{
    uint8_t i;

    if (g_boss.active) {
        uint8_t pal = (g_boss.flash_timer > 0U && (g_boss.flash_timer & 1U)) ? PAL_SPR_YELLOW : PAL_SPR_BOSS;
        int16_t bx = g_boss.x;
        int16_t by = g_boss.y;
        set_oam_sprite(SPR_OAM_ENEMY_BASE + 0U, bx,      by,     SPR_BOSS_TL, pal);
        set_oam_sprite(SPR_OAM_ENEMY_BASE + 1U, bx + 8,  by,     SPR_BOSS_TM, pal);
        set_oam_sprite(SPR_OAM_ENEMY_BASE + 2U, bx + 16, by,     SPR_BOSS_TM, pal);
        set_oam_sprite(SPR_OAM_ENEMY_BASE + 3U, bx + 24, by,     SPR_BOSS_TR, pal);
        set_oam_sprite(SPR_OAM_ENEMY_BASE + 4U, bx,      by + 8, SPR_BOSS_BL, pal);
        set_oam_sprite(SPR_OAM_ENEMY_BASE + 5U, bx + 8,  by + 8, SPR_BOSS_CORE, PAL_SPR_CYAN);
        set_oam_sprite(SPR_OAM_ENEMY_BASE + 6U, bx + 16, by + 8, SPR_BOSS_CORE, PAL_SPR_CYAN);
        set_oam_sprite(SPR_OAM_ENEMY_BASE + 7U, bx + 24, by + 8, SPR_BOSS_BR, pal);

        for (i = 0U; i < 6U; i++) {
            uint8_t oam = (uint8_t)(SPR_OAM_ENEMY_BASE + 8U + i);
            if (!g_enemies[i].active) {
                hide_sprite(oam);
            } else {
                uint8_t t = g_enemies[i].type;
                uint8_t tile = (t == EN_ARMORED) ? (uint8_t)(SPR_ARMOR_0 + anim_frame) : (uint8_t)(SPR_SHOOT_0 + anim_frame);
                uint8_t epal = (t == EN_ARMORED) ? ((g_enemies[i].hp <= 1U) ? PAL_SPR_RED : PAL_SPR_YELLOW) : PAL_SPR_CYAN;
                set_oam_sprite(oam, g_enemies[i].x, g_enemies[i].y, tile, epal);
            }
        }
        return;
    }

    for (i = 0U; i < MAX_ENEMIES; i++) {
        uint8_t oam = (uint8_t)(SPR_OAM_ENEMY_BASE + i);
        if (!g_enemies[i].active) {
            hide_sprite(oam);
        } else {
            uint8_t t = g_enemies[i].type;
            uint8_t tile = SPR_GRUNT_0;
            uint8_t epal = PAL_SPR_GREEN;
            if (t == EN_GRUNT) {
                tile = (uint8_t)(SPR_GRUNT_0 + anim_frame);
                epal = PAL_SPR_GREEN;
            } else if (t == EN_SHOOTER) {
                tile = (uint8_t)(SPR_SHOOT_0 + anim_frame);
                epal = PAL_SPR_CYAN;
            } else if (t == EN_ARMORED) {
                tile = (uint8_t)(SPR_ARMOR_0 + anim_frame);
                epal = (g_enemies[i].hp <= 1U) ? PAL_SPR_RED : PAL_SPR_YELLOW;
            } else {
                tile = (uint8_t)(SPR_DIVER_0 + anim_frame);
                epal = (g_enemies[i].hp <= 1U) ? PAL_SPR_RED : PAL_SPR_BOSS;
            }
            set_oam_sprite(oam, g_enemies[i].x, g_enemies[i].y, tile, epal);
        }
    }
}

static void render_title_sprites(void)
{
    uint8_t f = (uint8_t)((frame_cnt >> 4) & 1U);
    int16_t sway = ((frame_cnt & 63U) < 32U) ? 1 : 0;
    set_oam_sprite(SPR_OAM_ENEMY_BASE + 0U, 36 + sway, 40, (uint8_t)(SPR_GRUNT_0 + f), PAL_SPR_GREEN);
    set_oam_sprite(SPR_OAM_ENEMY_BASE + 1U, 36 + sway, 48, (uint8_t)(SPR_SHOOT_0 + f), PAL_SPR_CYAN);
    set_oam_sprite(SPR_OAM_ENEMY_BASE + 2U, 36 + sway, 56, (uint8_t)(SPR_ARMOR_0 + f), PAL_SPR_YELLOW);
    set_oam_sprite(SPR_OAM_UFO_L,           32 + sway, 64, SPR_UFO_L, PAL_SPR_RED);
    set_oam_sprite(SPR_OAM_UFO_R,           40 + sway, 64, SPR_UFO_R, PAL_SPR_RED);
}

static void draw_title_cursor(void)
{
    gfx_set_tile(5U, 11U, (menu_opt == 0U) ? TILE_CURSOR : TILE_EMPTY, PAL_BG_YELLOW);
    gfx_set_tile(5U, 12U, (menu_opt == 1U) ? TILE_CURSOR : TILE_EMPTY, PAL_BG_YELLOW);
    gfx_set_tile(5U, 13U, (menu_opt == 2U) ? TILE_CURSOR : TILE_EMPTY, PAL_BG_YELLOW);
}

static void enter_title(void)
{
    g_state = STATE_TITLE;
    DISPLAY_OFF;
    hide_all_sprites();
    clear_projectiles_and_fx();
    gfx_clear_screen(TILE_EMPTY, PAL_BG_UI);
    gfx_draw_starfield(1U, 16U);

    gfx_draw_text(3U, 2U, "SPACE INVADERS", PAL_BG_YELLOW);
    gfx_draw_text(6U, 5U, "= 10 PTS", PAL_BG_GREEN);
    gfx_draw_text(6U, 6U, "= 20 PTS", PAL_BG_CYAN);
    gfx_draw_text(6U, 7U, "= 40 PTS", PAL_BG_YELLOW);
    gfx_draw_text(6U, 8U, "=150 PTS", PAL_BG_RED);

    gfx_draw_text(7U, 11U, "START GAME", PAL_BG_UI);
    gfx_draw_text(7U, 12U, "HIGH SCORE", PAL_BG_UI);
    gfx_draw_text(7U, 13U, "CONTROLES", PAL_BG_UI);
    draw_title_cursor();

    gfx_draw_text(0U, 16U, "A:DISP B:ESP STA:PAU", PAL_BG_CYAN);
    render_title_sprites();

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    sound_set_music(MUS_MENU);
}

static void enter_high_score_screen(void)
{
    g_state = STATE_HIGH_SCORE;
    DISPLAY_OFF;
    hide_all_sprites();
    gfx_clear_screen(TILE_EMPTY, PAL_BG_UI);
    gfx_draw_starfield(0U, 17U);

    gfx_draw_text(2U, 3U, "RECORD DE SESION", PAL_BG_YELLOW);
    gfx_draw_text(5U, 6U, "HIGH SCORE", PAL_BG_CYAN);
    gfx_draw_number(7U, 8U, g_high_score, 5U, PAL_BG_UI);
    gfx_draw_text(4U, 11U, "OLEADA MAX:", PAL_BG_GREEN);
    gfx_draw_number(16U, 11U, (uint16_t)(g_max_wave_reached + 1U), 1U, PAL_BG_GREEN);
    gfx_draw_text(3U, 15U, "A O B: VOLVER", PAL_BG_UI);

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

static void enter_controls_screen(void)
{
    g_state = STATE_CONTROLS;
    DISPLAY_OFF;
    hide_all_sprites();
    gfx_clear_screen(TILE_EMPTY, PAL_BG_UI);
    gfx_draw_starfield(0U, 17U);

    gfx_draw_text(5U, 2U, "CONTROLES", PAL_BG_YELLOW);
    gfx_draw_text(1U, 5U,  "IZQ-DER: MOVER", PAL_BG_UI);
    gfx_draw_text(1U, 7U,  "A: DISPARO LASER", PAL_BG_CYAN);
    gfx_draw_text(1U, 9U,  "B: RAFAGA ESPECIAL", PAL_BG_GREEN);
    gfx_draw_text(1U, 11U, "START: PAUSA", PAL_BG_ORANGE);
    gfx_draw_text(3U, 15U, "A O B: VOLVER", PAL_BG_UI);

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

static void enter_level_intro(void)
{
    const struct LevelSpec *spec = level_get_spec(g_level);
    uint8_t len = 0U;
    uint8_t tx;

    if (g_level > g_max_wave_reached) g_max_wave_reached = g_level;
    while (spec->name[len]) len++;
    tx = (uint8_t)((20U - len) >> 1);

    g_state = STATE_LEVEL_INTRO;
    state_timer = 75U;

    DISPLAY_OFF;
    hide_all_sprites();
    clear_projectiles_and_fx();
    gfx_clear_screen(TILE_EMPTY, PAL_BG_UI);
    gfx_draw_starfield(1U, 16U);

    gfx_draw_text(6U, 6U, "OLEADA", PAL_BG_YELLOW);
    gfx_draw_number(13U, 6U, (uint16_t)(g_level + 1U), 1U, PAL_BG_YELLOW);
    gfx_draw_text(tx, 9U, spec->name, spec->has_boss ? PAL_BG_MAGENTA : PAL_BG_CYAN);
    gfx_draw_text(5U, 12U, "PREPARADO!", PAL_BG_UI);

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    level_load(g_level);
    sound_set_music(MUS_NONE);
}

static uint8_t compute_dynamic_tempo(void)
{
    const struct LevelSpec *spec = level_get_spec(g_level);
    uint8_t step = spec->base_step_frames;

    if (g_boss.active) return 10U;
    if (g_enemies_alive <= 2U) return 6U;
    if (g_enemies_alive <= 5U) return (uint8_t)(step >> 2) + 4U;
    if (g_enemies_alive <= 10U) return (uint8_t)(step >> 1);
    return step;
}

static void enter_playing_wave(void)
{
    const struct LevelSpec *spec = level_get_spec(g_level);

    DISPLAY_OFF;
    hide_all_sprites();
    clear_projectiles_and_fx();

    gfx_clear_screen(TILE_EMPTY, PAL_BG_UI);
    gfx_draw_starfield(1U, 16U);
    if (g_level == 0U) {
        barriers_init();
    }
    barriers_draw_all();
    draw_hud_full();

    g_player_x = 72;
    g_player_invuln = 30U;
    fire_cd = 0U;
    player_fire_anim = 0U;
    form_dir = 1;
    form_timer = spec->base_step_frames;
    anim_frame = 0U;
    enemy_fire_timer = spec->fire_interval;
    diver_timer = 160U;
    ufo_spawn_timer = 420U;
    g_state = STATE_PLAYING;

    render_player();
    render_enemies_and_boss();

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    sound_set_tempo(compute_dynamic_tempo());
    sound_set_music(spec->has_boss ? MUS_BOSS : MUS_GAME);
}

static void start_new_game(void)
{
    g_score = 0U;
    g_lives = INIT_LIVES;
    g_level = 0U;
    g_special_charge = 4U;
    enter_level_intro();
}

static void enter_end_screen(uint8_t is_victory)
{
    g_state = is_victory ? STATE_VICTORY : STATE_GAME_OVER;

    DISPLAY_OFF;
    hide_all_sprites();
    clear_projectiles_and_fx();
    gfx_clear_screen(TILE_EMPTY, PAL_BG_UI);
    gfx_draw_starfield(0U, 17U);

    if (is_victory) {
        gfx_draw_text(5U, 3U, "VICTORIA!", PAL_BG_YELLOW);
        gfx_draw_text(1U, 5U, "GALAXIA LIBERADA!", PAL_BG_GREEN);
    } else {
        gfx_draw_text(5U, 3U, "GAME OVER", PAL_BG_RED);
        gfx_draw_text(3U, 5U, "INVASION EXITOSA", PAL_BG_ORANGE);
    }
    gfx_draw_text(3U, 8U,  "PUNTOS:", PAL_BG_UI);
    gfx_draw_number(11U, 8U, g_score, 5U, PAL_BG_CYAN);
    gfx_draw_text(3U, 10U, "RECORD:", PAL_BG_UI);
    gfx_draw_number(11U, 10U, g_high_score, 5U, PAL_BG_YELLOW);
    gfx_draw_text(3U, 13U, "A: JUGAR DE NUEVO", PAL_BG_UI);
    gfx_draw_text(4U, 15U, "SELECT: MENU", PAL_BG_UI);

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    sound_set_music(MUS_NONE);
    sound_play_sfx(is_victory ? SFX_VICTORY : SFX_GAME_OVER);
}

static void trigger_player_hit(uint8_t invasion_reached)
{
    uint8_t i;
    if (invasion_reached) {
        g_lives = 0U;
    } else if (g_lives > 0U) {
        g_lives--;
    }
    draw_hud_lives();

    for (i = 0U; i < MAX_P_SHOTS; i++) {
        g_p_shots[i].active = 0U;
        hide_sprite((uint8_t)(SPR_OAM_PSHOT_0 + i));
    }
    for (i = 0U; i < MAX_E_SHOTS; i++) {
        g_e_shots[i].active = 0U;
        hide_sprite((uint8_t)(SPR_OAM_ESHOT_0 + i));
    }

    spawn_fx(g_player_x, PLAYER_Y);
    spawn_fx(g_player_x + 8, PLAYER_Y);
    sound_play_sfx(SFX_PLAYER_EXPLODE);
    g_state = STATE_PLAYER_HIT;
    state_timer = 50U;
    render_player();
}

static void check_wave_complete(void)
{
    if (g_boss.active) return;
    if (g_enemies_alive > 0U) return;

    clear_projectiles_only();
    g_state = STATE_LEVEL_CLEAR;
    state_timer = 90U;
    gfx_draw_text(1U, 9U, "OLEADA COMPLETADA!", PAL_BG_YELLOW);
    sound_set_music(MUS_NONE);
    sound_play_sfx(SFX_LEVEL_CLEAR);
}

static void destroy_enemy(uint8_t idx)
{
    struct Enemy *e = &g_enemies[idx];
    e->active = 0U;
    if (g_enemies_alive > 0U) g_enemies_alive--;
    add_score(level_enemy_points(e->type));
    spawn_fx(e->x, e->y);
    sound_play_sfx(SFX_ENEMY_EXPLODE);

    if (g_special_charge < 4U) {
        g_special_charge++;
        draw_hud_special();
    }
    sound_set_tempo(compute_dynamic_tempo());
}

static void spawn_enemy_shot(int16_t x, int16_t y, int8_t vx, int8_t vy)
{
    uint8_t i;
    for (i = 0U; i < MAX_E_SHOTS; i++) {
        if (!g_e_shots[i].active) {
            g_e_shots[i].x = x;
            g_e_shots[i].y = y;
            g_e_shots[i].vx = vx;
            g_e_shots[i].vy = vy;
            g_e_shots[i].active = 1U;
            g_e_shots[i].dmg = 1U;
            g_e_shots[i].is_special = 0U;
            sound_play_sfx(SFX_ENEMY_SHOT);
            return;
        }
    }
}

static void update_player_weapons(uint8_t pressed)
{
    uint8_t i;
    uint8_t normal_cnt = 0U;

    if (fire_cd > 0U) fire_cd--;

    for (i = 0U; i < MAX_P_SHOTS; i++) {
        if (g_p_shots[i].active && !g_p_shots[i].is_special) {
            normal_cnt++;
        }
    }

    if ((pressed & J_B) && g_special_charge >= 4U) {
        static const int8_t sp_vx[3] = { -1, 0, 1 };
        for (i = 0U; i < MAX_P_SHOTS; i++) {
            g_p_shots[i].x = g_player_x + 6;
            g_p_shots[i].y = PLAYER_Y - 6;
            g_p_shots[i].vx = sp_vx[i];
            g_p_shots[i].vy = -3;
            g_p_shots[i].active = 1U;
            g_p_shots[i].dmg = 2U;
            g_p_shots[i].is_special = 1U;
        }
        g_special_charge = 0U;
        player_fire_anim = 8U;
        draw_hud_special();
        sound_play_sfx(SFX_SPECIAL_SHOT);
    } else if ((pressed & J_A) && fire_cd == 0U && normal_cnt < MAX_NORMAL_PSHOTS) {
        for (i = 0U; i < MAX_P_SHOTS; i++) {
            if (!g_p_shots[i].active) {
                g_p_shots[i].x = g_player_x + 7;
                g_p_shots[i].y = PLAYER_Y - 6;
                g_p_shots[i].vx = 0;
                g_p_shots[i].vy = -3;
                g_p_shots[i].active = 1U;
                g_p_shots[i].dmg = 1U;
                g_p_shots[i].is_special = 0U;
                fire_cd = 10U;
                player_fire_anim = 6U;
                sound_play_sfx(SFX_PLAYER_SHOT);
                break;
            }
        }
    }
}

static void update_ufo(void)
{
    if (!g_ufo_active) {
        if (!g_boss.active) {
            if (ufo_spawn_timer > 0U) {
                ufo_spawn_timer--;
            } else {
                g_ufo_active = 1U;
                if (next_rng() & 1U) {
                    g_ufo_x = -14;
                    ufo_vx = 1;
                } else {
                    g_ufo_x = 158;
                    ufo_vx = -1;
                }
                ufo_spawn_timer = 520U;
                sound_play_sfx(SFX_UFO);
            }
        }
        hide_sprite(SPR_OAM_UFO_L);
        hide_sprite(SPR_OAM_UFO_R);
        return;
    }

    g_ufo_x += ufo_vx;
    if ((frame_cnt & 15U) == 0U) sound_play_sfx(SFX_UFO);

    if (g_ufo_x < -16 || g_ufo_x > 162) {
        g_ufo_active = 0U;
        hide_sprite(SPR_OAM_UFO_L);
        hide_sprite(SPR_OAM_UFO_R);
    } else {
        set_oam_sprite(SPR_OAM_UFO_L, g_ufo_x,     UFO_Y, SPR_UFO_L, PAL_SPR_RED);
        set_oam_sprite(SPR_OAM_UFO_R, g_ufo_x + 8, UFO_Y, SPR_UFO_R, PAL_SPR_RED);
    }
}

static void update_formation_and_boss(void)
{
    const struct LevelSpec *spec = level_get_spec(g_level);
    uint8_t i;
    uint8_t edge_hit = 0U;
    uint8_t drop_invaded = 0U;

    /* Jefe Final (Nivel 5) */
    if (g_boss.active) {
        if (g_boss.flash_timer > 0U) g_boss.flash_timer--;
        g_boss.x += g_boss.vx;
        if (g_boss.x <= 12) {
            g_boss.x = 12;
            g_boss.vx = 1;
        } else if (g_boss.x >= 116) {
            g_boss.x = 116;
            g_boss.vx = -1;
        }
        if (g_boss.fire_cd > 0U) {
            g_boss.fire_cd--;
        } else {
            g_boss.fire_cd = 56U;
            spawn_enemy_shot(g_boss.x + 4,  g_boss.y + 14, -1, 2);
            spawn_enemy_shot(g_boss.x + 15, g_boss.y + 14,  0, 2);
            spawn_enemy_shot(g_boss.x + 26, g_boss.y + 14,  1, 2);
        }
    }

    /* Paso periodico de formacion */
    if (form_timer > 0U) {
        form_timer--;
    } else {
        int16_t step_dx = (int16_t)form_dir * 4;
        form_timer = compute_dynamic_tempo();
        anim_frame ^= 1U;

        for (i = 0U; i < MAX_ENEMIES; i++) {
            int16_t nx;
            if (!g_enemies[i].active || g_enemies[i].mode != 0U) continue;
            nx = g_enemies[i].x + step_dx;
            if (nx < PLAY_LEFT || nx > (PLAY_RIGHT - 8)) {
                edge_hit = 1U;
                break;
            }
        }

        if (edge_hit) {
            form_dir = (int8_t)-form_dir;
            for (i = 0U; i < MAX_ENEMIES; i++) {
                if (!g_enemies[i].active || g_enemies[i].mode != 0U) continue;
                g_enemies[i].y += 6;
                barriers_crush_area(g_enemies[i].x, g_enemies[i].y, 8U, 8U);
                if (g_enemies[i].y >= INVADER_LOSE_Y) {
                    drop_invaded = 1U;
                }
            }
            if (drop_invaded) {
                render_enemies_and_boss();
                trigger_player_hit(1U);
                return;
            }
        } else {
            for (i = 0U; i < MAX_ENEMIES; i++) {
                if (!g_enemies[i].active || g_enemies[i].mode != 0U) continue;
                g_enemies[i].x += step_dx;
            }
        }
    }

    /* Enemigos en picada (Nivel 4) */
    if (spec->allow_divers) {
        if (diver_timer > 0U) {
            diver_timer--;
        } else {
            diver_timer = 150U;
            for (i = 0U; i < MAX_ENEMIES; i++) {
                if (g_enemies[i].active && g_enemies[i].type == EN_DIVER && g_enemies[i].mode == 0U) {
                    g_enemies[i].mode = 1U;
                    g_enemies[i].vy = 1;
                    g_enemies[i].vx = (g_enemies[i].x < g_player_x) ? 1 : -1;
                    break;
                }
            }
        }
    }

    for (i = 0U; i < MAX_ENEMIES; i++) {
        struct Enemy *e = &g_enemies[i];
        if (!e->active || e->mode == 0U) continue;

        if (frame_cnt & 1U) {
            e->y += e->vy;
            e->x += ((frame_cnt & 15U) < 8U) ? e->vx : (int8_t)-e->vx;
            if (e->x < PLAY_LEFT) e->x = PLAY_LEFT;
            if (e->x > PLAY_RIGHT - 8) e->x = PLAY_RIGHT - 8;
        }
        barriers_crush_area(e->x, e->y, 8U, 8U);

        if (g_player_invuln == 0U &&
            e->x + 7 >= g_player_x + 2 && e->x <= g_player_x + 13 &&
            e->y + 7 >= PLAYER_Y + 1 && e->y <= PLAYER_Y + 7) {
            destroy_enemy(i);
            render_enemies_and_boss();
            trigger_player_hit(0U);
            return;
        }
        if (e->y > 136) {
            e->y = 20;
        }
    }

    /* Disparos aleatorios de enemigos activos */
    if (enemy_fire_timer > 0U) {
        enemy_fire_timer--;
    } else if (g_enemies_alive > 0U) {
        uint8_t active_list[MAX_ENEMIES];
        uint8_t cnt = 0U;
        enemy_fire_timer = spec->fire_interval;
        for (i = 0U; i < MAX_ENEMIES; i++) {
            if (g_enemies[i].active) active_list[cnt++] = i;
        }
        if (cnt > 0U) {
            uint8_t pick = active_list[next_rng() % cnt];
            int8_t svx = 0;
            if (spec->aimed_shots) {
                if (g_enemies[pick].x + 12 < g_player_x) svx = 1;
                else if (g_enemies[pick].x > g_player_x + 12) svx = -1;
            }
            spawn_enemy_shot(g_enemies[pick].x + 3, g_enemies[pick].y + 8, svx, 2);
        }
    }
}

static void update_projectiles(void)
{
    uint8_t i, j;

    /* Proyectiles del jugador */
    for (i = 0U; i < MAX_P_SHOTS; i++) {
        struct Projectile *s = &g_p_shots[i];
        uint8_t sw;
        uint8_t hit = 0U;
        if (!s->active) {
            hide_sprite((uint8_t)(SPR_OAM_PSHOT_0 + i));
            continue;
        }

        sw = s->is_special ? 4U : 2U;
        s->x += s->vx;
        s->y += s->vy;

        if (s->y < PLAY_TOP || s->x < PLAY_LEFT || s->x > PLAY_RIGHT) {
            s->active = 0U;
            hide_sprite((uint8_t)(SPR_OAM_PSHOT_0 + i));
            continue;
        }

        if (barrier_check_hit(s->x, s->y, sw, 6U)) {
            s->active = 0U;
            sound_play_sfx(SFX_BARRIER_HIT);
            hide_sprite((uint8_t)(SPR_OAM_PSHOT_0 + i));
            continue;
        }

        /* Choque aire-aire con disparos enemigos */
        for (j = 0U; j < MAX_E_SHOTS; j++) {
            if (!g_e_shots[j].active) continue;
            if (s->x + (int16_t)sw > g_e_shots[j].x && s->x < g_e_shots[j].x + 4 &&
                s->y + 6 >= g_e_shots[j].y && s->y <= g_e_shots[j].y + 6) {
                g_e_shots[j].active = 0U;
                hide_sprite((uint8_t)(SPR_OAM_ESHOT_0 + j));
                if (!s->is_special) {
                    s->active = 0U;
                    hit = 1U;
                }
                spawn_fx(s->x - 2, s->y);
                add_score(10U);
                break;
            }
        }
        if (hit) {
            hide_sprite((uint8_t)(SPR_OAM_PSHOT_0 + i));
            continue;
        }

        /* Colision contra OVNI */
        if (g_ufo_active &&
            s->x + (int16_t)sw > g_ufo_x && s->x < g_ufo_x + 16 &&
            s->y + 6 >= UFO_Y && s->y <= UFO_Y + 8) {
            g_ufo_active = 0U;
            hide_sprite(SPR_OAM_UFO_L);
            hide_sprite(SPR_OAM_UFO_R);
            spawn_fx(g_ufo_x + 4, UFO_Y);
            add_score(s->is_special ? 300U : 150U);
            sound_play_sfx(SFX_ENEMY_EXPLODE);
            s->active = 0U;
            hide_sprite((uint8_t)(SPR_OAM_PSHOT_0 + i));
            continue;
        }

        /* Colision contra Jefe Final */
        if (g_boss.active &&
            s->x + (int16_t)sw > g_boss.x && s->x < g_boss.x + 32 &&
            s->y + 6 >= g_boss.y && s->y <= g_boss.y + 16) {
            if (g_boss.hp > s->dmg) {
                g_boss.hp -= s->dmg;
                g_boss.flash_timer = 6U;
                add_score(25U);
                sound_play_sfx(SFX_ENEMY_HIT);
            } else {
                g_boss.hp = 0U;
                g_boss.active = 0U;
                for (j = 0U; j < MAX_ENEMIES; j++) {
                    if (g_enemies[j].active) {
                        g_enemies[j].active = 0U;
                        spawn_fx(g_enemies[j].x, g_enemies[j].y);
                    }
                }
                g_enemies_alive = 0U;
                spawn_fx(g_boss.x + 8, g_boss.y + 4);
                add_score(500U);
                sound_play_sfx(SFX_ENEMY_EXPLODE);
            }
            s->active = 0U;
            hide_sprite((uint8_t)(SPR_OAM_PSHOT_0 + i));
            check_wave_complete();
            if (g_state != STATE_PLAYING) return;
            continue;
        }

        /* Colision contra enemigos de formacion (1 impacto maximo por proyectil) */
        for (j = 0U; j < MAX_ENEMIES; j++) {
            struct Enemy *e = &g_enemies[j];
            if (!e->active) continue;
            if (s->x + (int16_t)sw > e->x && s->x < e->x + 8 &&
                s->y + 6 >= e->y && s->y <= e->y + 8) {
                s->active = 0U;
                if (e->hp > s->dmg) {
                    e->hp -= s->dmg;
                    add_score(15U);
                    sound_play_sfx(SFX_ENEMY_HIT);
                } else {
                    e->hp = 0U;
                    destroy_enemy(j);
                }
                hit = 1U;
                break;
            }
        }
        if (hit) {
            hide_sprite((uint8_t)(SPR_OAM_PSHOT_0 + i));
            check_wave_complete();
            if (g_state != STATE_PLAYING) return;
            continue;
        }

        set_oam_sprite((uint8_t)(SPR_OAM_PSHOT_0 + i), s->x, s->y,
                       s->is_special ? SPR_SPSHOT : SPR_PSHOT,
                       s->is_special ? PAL_SPR_CYAN : PAL_SPR_YELLOW);
    }

    /* Proyectiles enemigos */
    for (i = 0U; i < MAX_E_SHOTS; i++) {
        struct Projectile *e = &g_e_shots[i];
        uint8_t espr;
        if (!e->active) {
            hide_sprite((uint8_t)(SPR_OAM_ESHOT_0 + i));
            continue;
        }

        e->x += e->vx;
        e->y += e->vy;
        if (e->y >= 132 || e->x < PLAY_LEFT || e->x > PLAY_RIGHT) {
            e->active = 0U;
            hide_sprite((uint8_t)(SPR_OAM_ESHOT_0 + i));
            continue;
        }

        if (barrier_check_hit(e->x, e->y, 3U, 6U)) {
            e->active = 0U;
            sound_play_sfx(SFX_BARRIER_HIT);
            hide_sprite((uint8_t)(SPR_OAM_ESHOT_0 + i));
            continue;
        }

        if (g_player_invuln == 0U &&
            e->x + 3 > g_player_x + 2 && e->x < g_player_x + 14 &&
            e->y + 6 >= PLAYER_Y + 2 && e->y <= PLAYER_Y + 7) {
            e->active = 0U;
            hide_sprite((uint8_t)(SPR_OAM_ESHOT_0 + i));
            trigger_player_hit(0U);
            return;
        }

        espr = (uint8_t)(SPR_ESHOT_0 + ((frame_cnt >> 2) & 1U));
        set_oam_sprite((uint8_t)(SPR_OAM_ESHOT_0 + i), e->x, e->y, espr, PAL_SPR_RED);
    }
}

void game_init(void)
{
    prev_joy = 0U;
    menu_opt = 0U;
    frame_cnt = 0U;
    g_score = 0U;
    g_high_score = 0U;
    g_max_wave_reached = 0U;
    g_lives = INIT_LIVES;
    g_level = 0U;
    g_player_speed = 2U;
    g_player_invuln = 0U;
    g_special_charge = 4U;
    enter_title();
}

void game_update(void)
{
    uint8_t joy = joypad();
    uint8_t pressed = (uint8_t)(joy & (uint8_t)~prev_joy);
    prev_joy = joy;
    frame_cnt++;

    update_fx();

    switch (g_state) {
    case STATE_TITLE:
        render_title_sprites();
        if (pressed & J_DOWN) {
            menu_opt = (uint8_t)((menu_opt + 1U) % 3U);
            draw_title_cursor();
            sound_play_sfx(SFX_MENU);
        } else if (pressed & J_UP) {
            menu_opt = (menu_opt == 0U) ? 2U : (uint8_t)(menu_opt - 1U);
            draw_title_cursor();
            sound_play_sfx(SFX_MENU);
        } else if (pressed & (J_A | J_START)) {
            sound_play_sfx(SFX_MENU);
            if (menu_opt == 0U) start_new_game();
            else if (menu_opt == 1U) enter_high_score_screen();
            else enter_controls_screen();
        }
        break;

    case STATE_HIGH_SCORE:
    case STATE_CONTROLS:
        if (pressed & (J_A | J_B | J_START | J_SELECT)) {
            sound_play_sfx(SFX_MENU);
            enter_title();
        }
        break;

    case STATE_LEVEL_INTRO:
        if (state_timer > 0U) state_timer--;
        if (state_timer == 0U || (pressed & (J_A | J_START))) {
            enter_playing_wave();
        }
        break;

    case STATE_PLAYING:
        if (pressed & J_START) {
            g_state = STATE_PAUSED;
            sound_set_paused(1U);
            sound_play_sfx(SFX_PAUSE);
            gfx_draw_text(7U, 7U,  "PAUSA", PAL_BG_YELLOW);
            gfx_draw_text(3U, 9U,  "START: SEGUIR", PAL_BG_UI);
            gfx_draw_text(4U, 10U, "SELECT: MENU", PAL_BG_UI);
            break;
        }

        if (g_player_invuln > 0U) g_player_invuln--;
        {
            int16_t nx = g_player_x;
            if (joy & J_LEFT)  nx -= (int16_t)g_player_speed;
            if (joy & J_RIGHT) nx += (int16_t)g_player_speed;
            if (nx < PLAYER_MIN_X) nx = PLAYER_MIN_X;
            if (nx > PLAYER_MAX_X) nx = PLAYER_MAX_X;
            g_player_x = nx;
        }

        update_player_weapons(pressed);
        update_ufo();
        update_formation_and_boss();
        if (g_state != STATE_PLAYING) break;

        update_projectiles();
        if (g_state != STATE_PLAYING) break;

        render_player();
        render_enemies_and_boss();
        break;

    case STATE_PAUSED:
        if (pressed & J_START) {
            gfx_draw_starfield(7U, 10U);
            sound_set_paused(0U);
            sound_play_sfx(SFX_MENU);
            g_state = STATE_PLAYING;
        } else if (pressed & J_SELECT) {
            enter_title();
        }
        break;

    case STATE_PLAYER_HIT:
        if (state_timer > 0U) state_timer--;
        if (state_timer == 0U) {
            if (g_lives == 0U) {
                enter_end_screen(0U);
            } else {
                g_player_x = 72;
                g_player_invuln = 90U;
                g_state = STATE_PLAYING;
                render_player();
            }
        }
        break;

    case STATE_LEVEL_CLEAR:
        if (state_timer > 0U) state_timer--;
        if (state_timer == 0U || (pressed & (J_A | J_START))) {
            g_level++;
            if (g_level >= NUM_LEVELS) {
                enter_end_screen(1U);
            } else {
                enter_level_intro();
            }
        }
        break;

    case STATE_GAME_OVER:
    case STATE_VICTORY:
        if (pressed & (J_A | J_START)) {
            sound_play_sfx(SFX_MENU);
            start_new_game();
        } else if (pressed & J_SELECT) {
            enter_title();
        }
        break;

    default:
        enter_title();
        break;
    }
}
