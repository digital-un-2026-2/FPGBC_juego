/*
 * hal_pc.c - Simulador en PC del hardware de la consola, usando raylib.
 *
 * Idea clave: raylib NO dibuja sprites aqui. Solo se usa para (1) abrir la
 * ventana, (2) leer el teclado y (3) mostrar una imagen de 160x144.
 * Los tiles, el mapa de fondo y los sprites se renderizan "a mano" en
 * render(), igual que lo haria tu periferico de video en la FPGA.
 */
#include "raylib.h"
#include "hal.h"

/* Escala entera: cuantos pixeles reales ocupa cada pixel logico.
 * Se toma la mas pequena entre ancho y alto para que la imagen quepa en el panel. */
#define SCALE_X (PANEL_SIZE / SCREEN_W)
#define SCALE_Y (PANEL_SIZE / SCREEN_H)
#define SCALE   (SCALE_X < SCALE_Y ? SCALE_X : SCALE_Y)

/* Si el panel no se llena exacto, la imagen se centra y quedan bordes negros */
#define OFFSET_X ((PANEL_SIZE - SCREEN_W * SCALE) / 2)
#define OFFSET_Y ((PANEL_SIZE - SCREEN_H * SCALE) / 2)

/* "Memoria de video" simulada */
static uint8_t tiles[MAX_TILES][16];
static uint8_t bgmap[MAP_H][MAP_W];
static struct { int16_t x, y; uint8_t tile, visible; } oam[MAX_SPRITES];

/* Imagen final */
static uint8_t   indices[SCREEN_W * SCREEN_H];   /* color 0..3 por pixel */
static Color     pixels[SCREEN_W * SCREEN_H];    /* RGBA por pixel */
static Texture2D screen_tex;

/* Paleta estilo Game Boy (0 = mas claro, 3 = mas oscuro) */
static const Color palette[4] = {
    { 224, 248, 208, 255 },
    { 136, 192, 112, 255 },
    {  52, 104,  86, 255 },
    {   8,  24,  32, 255 },
};

void hal_init(void)
{
    Image img;
    InitWindow(PANEL_SIZE, PANEL_SIZE, "Pong FPGBC (simulador raylib)");
    SetTargetFPS(60);

    img = GenImageColor(SCREEN_W, SCREEN_H, BLACK);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    screen_tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(screen_tex, TEXTURE_FILTER_POINT);   /* pixeles nitidos al escalar */
}

void hal_shutdown(void)
{
    UnloadTexture(screen_tex);
    CloseWindow();
}

void hal_load_tiles(uint8_t first, uint8_t count, const uint8_t *data)
{
    unsigned i;
    if ((unsigned)first + count > MAX_TILES) return;
    for (i = 0; i < (unsigned)count * 16; i++)
        ((uint8_t *)tiles)[(unsigned)first * 16 + i] = data[i];
}

void hal_set_bg(uint8_t tx, uint8_t ty, uint8_t tile)
{
    if (tx < MAP_W && ty < MAP_H && tile < MAX_TILES) bgmap[ty][tx] = tile;
}

void hal_set_sprite(uint8_t id, int16_t x, int16_t y, uint8_t tile)
{
    if (id >= MAX_SPRITES || tile >= MAX_TILES) return;
    oam[id].x = x;
    oam[id].y = y;
    oam[id].tile = tile;
    oam[id].visible = 1;
}

void hal_hide_sprite(uint8_t id)
{
    if (id < MAX_SPRITES) oam[id].visible = 0;
}

uint8_t hal_buttons(void)
{
    uint8_t b = 0;
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) b |= BTN_UP;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) b |= BTN_DOWN;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) b |= BTN_LEFT;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) b |= BTN_RIGHT;
    if (IsKeyDown(KEY_Z))             b |= BTN_A;
    if (IsKeyDown(KEY_X))             b |= BTN_B;
    if (IsKeyDown(KEY_ENTER))         b |= BTN_START;
    if (IsKeyDown(KEY_RIGHT_SHIFT))   b |= BTN_SELECT;
    return b;
}

/* Color (0..3) del pixel (px,py) de un tile en formato 2bpp */
static uint8_t tile_pixel(uint8_t t, int px, int py)
{
    uint8_t lo  = tiles[t][py * 2];
    uint8_t hi  = tiles[t][py * 2 + 1];
    int     bit = 7 - px;
    return (uint8_t)((((hi >> bit) & 1) << 1) | ((lo >> bit) & 1));
}

static void render(void)
{
    int x, y, i, px, py;

    /* 1) Fondo: recorre cada pixel, busca su tile en el mapa */
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++)
            indices[y * SCREEN_W + x] = tile_pixel(bgmap[y >> 3][x >> 3], x & 7, y & 7);

    /* 2) Sprites: el id mas bajo queda encima (se dibuja al final) */
    for (i = MAX_SPRITES - 1; i >= 0; i--) {
        if (!oam[i].visible) continue;
        for (py = 0; py < 8; py++) {
            y = oam[i].y + py;
            if (y < 0 || y >= SCREEN_H) continue;
            for (px = 0; px < 8; px++) {
                uint8_t c;
                x = oam[i].x + px;
                if (x < 0 || x >= SCREEN_W) continue;
                c = tile_pixel(oam[i].tile, px, py);
                if (c) indices[y * SCREEN_W + x] = c;   /* color 0 = transparente */
            }
        }
    }

    /* 3) Indices de color -> RGBA */
    for (i = 0; i < SCREEN_W * SCREEN_H; i++)
        pixels[i] = palette[indices[i]];
}

int hal_frame(void)
{
    render();
    UpdateTexture(screen_tex, pixels);

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(screen_tex,
                   (Rectangle){ 0, 0, SCREEN_W, SCREEN_H },
                   (Rectangle){ OFFSET_X, OFFSET_Y, SCREEN_W * SCALE, SCREEN_H * SCALE },
                   (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndDrawing();                       /* con SetTargetFPS(60) esto hace de "VBlank" */

    return !WindowShouldClose();
}