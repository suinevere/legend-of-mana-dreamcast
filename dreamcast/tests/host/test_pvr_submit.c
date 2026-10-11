/*----------------------
 | test_pvr_submit.c
 | Description: Host tests for the PowerVR submitter through libgpu with a fake backend: VSync(1) never flips, clear-only frames still open and show a scene, and colours above 0x80 brighten in an extra pass.
 | Author: suinevere
 | Dependencies: pvr_submit.h, pvr_backend.h, libgpu_dc.c, pvr_submit.c, gpu_decode.c, gpu_census.c, clip.c, blend.c, palette_bank.c, texture_cache.c, vram_mirror.c, lom_prefix.h (forced include)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gpu_decode.h"
#include "gpu_tag.h"
#include "pvr_submit.h"
#include "pvr_backend.h"
#include "palette_bank.h"
#include "texture_cache.h"
#include "libetc.h"
#include "vsync_pacer.h"

static int failures;
static int s_begins;
static int s_finishes;
static int s_backgrounds;
static uint8_t s_bg[3];
static int s_strips;
static int s_strips_in_scene;
static int s_strips_in_last_scene;
static int s_bg_in_open_scene;
static int s_open;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

void dc_halt(const char *message)
{
    (void)message;
}

static unsigned s_fake_vblanks;

void vsync_tick(void)
{
    s_fake_vblanks++;
}

unsigned vsync_count(void)
{
    return s_fake_vblanks;
}

int vsync_wait(int mode, VsyncClock clock)
{
    (void)clock;
    if (mode < 0)
    {
        return (int)s_fake_vblanks;
    }
    if (mode != 1)
    {
        s_fake_vblanks++;
    }
    return 0;
}

void dc_log(const char *format, ...)
{
    (void)format;
}

void frame_probe_scene(void)
{
}

void frame_probe_prim(void)
{
}

static void fake_scene_begin(unsigned scenes_closed)
{
    (void)scenes_closed;
    s_begins++;
    s_open = 1;
    s_strips_in_scene = 0;
}

static void fake_scene_finish(void)
{
    s_finishes++;
    s_open = 0;
    s_strips_in_last_scene = s_strips_in_scene;
}

static void fake_background(uint8_t r, uint8_t g, uint8_t b)
{
    s_backgrounds++;
    s_bg[0] = r;
    s_bg[1] = g;
    s_bg[2] = b;
    s_bg_in_open_scene = s_open;
}

static int s_headers;
static BlendPass s_header_log[8];
static uint32_t s_strip_colour[8];

static void fake_header(const DcSurfaceTexture *tex, const BlendPass *pass)
{
    (void)tex;
    if (s_headers < 8)
    {
        s_header_log[s_headers] = *pass;
    }
    s_headers++;
}

static void fake_vertex(float x, float y, float u, float v, uint32_t argb, int last)
{
    (void)x;
    (void)y;
    (void)u;
    (void)v;
    if (last && s_strips < 8)
    {
        s_strip_colour[s_strips] = argb & 0x00FFFFFFu;
    }
    if (last)
    {
        s_strips++;
        s_strips_in_scene++;
    }
}

static const PvrBackend s_fake = { fake_scene_begin, fake_scene_finish, fake_background, fake_header, fake_vertex };

static void *fake_alloc(unsigned bytes)
{
    static uint8_t pool[1 << 20];
    (void)bytes;
    return pool;
}

static void fake_set_entry(unsigned index, uint32_t argb)
{
    (void)index;
    (void)argb;
}

static const TextureOps s_texture_ops = { fake_alloc, 0, 0 };
static const PaletteOps s_palette_ops = { fake_set_entry };

static void reset(void)
{
    s_begins = 0;
    s_finishes = 0;
    s_backgrounds = 0;
    s_strips = 0;
    s_strips_in_last_scene = 0;
    s_headers = 0;
}

static void put_draw_area(int x, int y, int w, int h)
{
    DRAWENV env;

    SetDefDrawEnv(&env, x, y, w, h);
    PutDrawEnv(&env);
}

static void make_tile(TILE *tile, int x, int y)
{
    setlen(tile, 3);
    tile->r0 = 0x80;
    tile->g0 = 0x40;
    tile->b0 = 0x20;
    tile->code = 0x60;
    tile->x0 = (short)x;
    tile->y0 = (short)y;
    tile->w = 16;
    tile->h = 8;
    setaddr(tile, GPU_TAG_END);
}

static void draw_tile_ot(int x, int y)
{
    u_long chain[1 + sizeof(TILE) / sizeof(u_long)];

    gpu_decode_set_base((uintptr_t)chain);
    setlen(&chain[0], 0);
    setaddr(&chain[0], (uint32_t)sizeof(u_long));
    make_tile((TILE *)&chain[1], x, y);
    DrawOTag(&chain[0]);
    gpu_decode_set_base(0);
}

static void test_vsync1_does_not_split_the_scene(void)
{
    put_draw_area(0, 0, 320, 240);
    reset();
    draw_tile_ot(10, 10);
    VSync(1);
    draw_tile_ot(40, 40);
    VSync(0);
    CHECK(s_begins == 1);
    CHECK(s_finishes == 1);
    CHECK(s_strips_in_last_scene == 2);
}

static void test_negative_vsync_does_not_flip(void)
{
    put_draw_area(0, 0, 320, 240);
    reset();
    draw_tile_ot(10, 10);
    VSync(-1);
    CHECK(s_finishes == 0);
    VSync(0);
    CHECK(s_finishes == 1);
}

static void test_clear_only_frame_shows_its_background(void)
{
    RECT rect = { 0, 0, 320, 240 };

    put_draw_area(0, 0, 320, 240);
    reset();
    ClearImage(&rect, 0x10, 0x20, 0x30);
    VSync(0);
    CHECK(s_begins == 1);
    CHECK(s_finishes == 1);
    CHECK(s_backgrounds == 1);
    CHECK(s_bg_in_open_scene == 1);
    CHECK(s_bg[0] == 0x10 && s_bg[1] == 0x21 && s_bg[2] == 0x31);
}

static void test_isbg_only_frame_shows_its_background(void)
{
    DRAWENV env;

    SetDefDrawEnv(&env, 0, 0, 320, 240);
    env.isbg = 1;
    env.r0 = 0xF8;
    reset();
    PutDrawEnv(&env);
    VSync(0);
    CHECK(s_begins == 1);
    CHECK(s_finishes == 1);
    CHECK(s_bg[0] == 0xFF);
}

static void test_partial_clear_only_frame_draws_its_rect(void)
{
    RECT rect = { 0, 0, 32, 32 };

    put_draw_area(0, 0, 320, 240);
    reset();
    ClearImage(&rect, 0, 0, 0);
    VSync(0);
    CHECK(s_begins == 1);
    CHECK(s_finishes == 1);
    CHECK(s_strips_in_last_scene == 1);
}

static void test_subtractive_tile_inverts_adds_and_inverts(void)
{
    u_long chain[2 + 1 + sizeof(TILE) / sizeof(u_long)];
    TILE *tile = (TILE *)&chain[3];

    put_draw_area(0, 0, 320, 240);
    gpu_decode_set_base((uintptr_t)chain);
    setlen(&chain[0], 1);
    setaddr(&chain[0], (uint32_t)(2 * sizeof(u_long)));
    chain[1] = 0xE1000040u;
    setlen(&chain[2], 0);
    setaddr(&chain[2], (uint32_t)(3 * sizeof(u_long)));
    make_tile(tile, 8, 8);
    tile->code = 0x62;
    reset();
    DrawOTag(&chain[0]);
    gpu_decode_set_base(0);
    CHECK(s_strips == 3);
    CHECK(s_headers == 3);
    CHECK(s_header_log[0].src == BL_INVDSTCOLOR && s_header_log[0].dst == BL_INVSRCALPHA);
    CHECK(s_header_log[1].src == BL_ONE && s_header_log[1].dst == BL_ONE);
    CHECK(s_header_log[2].src == BL_INVDSTCOLOR && s_header_log[2].dst == BL_INVSRCALPHA);
    CHECK(s_strip_colour[0] == 0x00FFFFFFu);
    CHECK(s_strip_colour[1] == 0x00804020u);
    CHECK(s_strip_colour[2] == 0x00FFFFFFu);
    VSync(0);
}

static void test_bright_textured_sprite_adds_its_excess(void)
{
    u_long chain[2 + 1 + sizeof(SPRT) / sizeof(u_long)];
    SPRT *sprite = (SPRT *)&chain[3];

    put_draw_area(0, 0, 320, 240);
    gpu_decode_set_base((uintptr_t)chain);
    setlen(&chain[0], 1);
    setaddr(&chain[0], (uint32_t)(2 * sizeof(u_long)));
    chain[1] = 0xE1000100u;
    setlen(&chain[2], 0);
    setaddr(&chain[2], (uint32_t)(3 * sizeof(u_long)));
    setSprt(sprite);
    setRGB0(sprite, 0xC0, 0xC0, 0xC0);
    setXY0(sprite, 8, 8);
    setUV0(sprite, 0, 0);
    setWH(sprite, 16, 16);
    sprite->clut = 0;
    setaddr(sprite, GPU_TAG_END);
    reset();
    DrawOTag(&chain[0]);
    gpu_decode_set_base(0);
    CHECK(s_strips == 2);
    CHECK(s_headers == 2);
    CHECK(s_header_log[0].src == BL_SRCALPHA && s_header_log[0].dst == BL_INVSRCALPHA);
    CHECK(s_header_log[1].src == BL_SRCALPHA && s_header_log[1].dst == BL_ONE);
    CHECK(s_strip_colour[0] == 0x00FFFFFFu);
    CHECK(s_strip_colour[1] == 0x00808080u);
    VSync(0);
}

int main(void)
{
    texture_cache_init(&s_texture_ops);
    palette_bank_init(&s_palette_ops);
    pvr_submit_set_backend(&s_fake);
    test_vsync1_does_not_split_the_scene();
    test_negative_vsync_does_not_flip();
    test_clear_only_frame_shows_its_background();
    test_isbg_only_frame_shows_its_background();
    test_partial_clear_only_frame_draws_its_rect();
    test_subtractive_tile_inverts_adds_and_inverts();
    test_bright_textured_sprite_adds_its_excess();

    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
