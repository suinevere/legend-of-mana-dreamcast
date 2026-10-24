/*----------------------
 | test_pvr_submit.c
 | Description: Host tests for the PowerVR submitter through libgpu with a fake backend: VSync(1) never flips, clear-only frames still open and show a scene.
 | Also checks colours above 0x80 brighten in an extra pass, and a full-area clear over already-submitted prims redraws them with a full-area rect.
 | A scene whose flip wait expired skips the pending texture flush, leaving it for the next scene whose wait succeeds.
 | Author: suinevere
 | Dependencies: pvr_submit.h, pvr_backend.h, libgpu_dc.c, pvr_submit.c, gpu_decode.c, gpu_census.c, clip.c, blend.c, palette_bank.c, texture_cache.c, vram_mirror.c, frame_readback.c, lom_prefix.h (forced include)
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
static int s_scene_begin_result;
static int s_free_all_calls;

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

void vsync_wait_after(VsyncClock clock)
{
    (void)clock;
    s_fake_vblanks++;
}

void dc_log(const char *format, ...)
{
    (void)format;
}

/*----------------------
 | dc_time_us
 | Description: Host stand-in for the Dreamcast microsecond timer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 0
 ----------------------*/
unsigned long long dc_time_us(void)
{
    return 0;
}

static int s_probe_scenes;

void frame_probe_scene(void)
{
    s_probe_scenes++;
}

/*----------------------
 | s_probe_drawn / frame_probe_drawn
 | Description: Counts the closed scenes pvr_submit_flip reports as having drawn something.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_probe_drawn
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_probe_drawn;

void frame_probe_drawn(void)
{
    s_probe_drawn++;
}

/*----------------------
 | cdrom_dc_vsync_tick
 | Description: Host stand-in for the CD stream delivery VSync asks for.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void cdrom_dc_vsync_tick(void)
{
}

void frame_probe_prim(void)
{
}

void frame_probe_area(int kind, float pixels)
{
    (void)kind;
    (void)pixels;
}

static uint16_t s_frame_pixels[640 * 480];

const uint16_t *pvr_backend_frame_pixels(unsigned *pitch_px)
{
    *pitch_px = 640;
    return s_frame_pixels;
}

static int fake_scene_begin(unsigned scenes_closed)
{
    (void)scenes_closed;
    s_begins++;
    s_open = 1;
    s_strips_in_scene = 0;
    return s_scene_begin_result;
}

static int s_scene_shown_result;

static int fake_scene_shown(unsigned scenes_closed)
{
    (void)scenes_closed;
    return s_scene_shown_result;
}

/*----------------------
 | s_wait_shown_calls / fake_wait_shown
 | Description: Counts the backend waits for a closed scene to reach the screen; the wait shows it, as a flip arriving would.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_wait_shown_calls, s_scene_shown_result
 | Params: scenes_closed -- unused
 | Returns: 1
 ----------------------*/
static int s_wait_shown_calls;

static int fake_wait_shown(unsigned scenes_closed)
{
    (void)scenes_closed;
    s_wait_shown_calls++;
    s_scene_shown_result = 1;
    return 1;
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
static float s_strip_min_x, s_strip_max_x, s_strip_min_y, s_strip_max_y;
static float s_last_strip_min_x, s_last_strip_max_x, s_last_strip_min_y, s_last_strip_max_y;
static int s_strip_has_vertex;

static const DcSurfaceTexture *s_last_header_tex;

static const DcSurfaceTexture *s_first_header_tex;

static void fake_header(const DcSurfaceTexture *tex, const BlendPass *pass)
{
    if (s_headers == 0)
    {
        s_first_header_tex = tex;
    }
    s_last_header_tex = tex;
    if (s_headers < 8)
    {
        s_header_log[s_headers] = *pass;
    }
    s_headers++;
}

static void fake_vertex(float x, float y, float u, float v, uint32_t argb, int last)
{
    (void)u;
    (void)v;
    if (!s_strip_has_vertex)
    {
        s_strip_min_x = s_strip_max_x = x;
        s_strip_min_y = s_strip_max_y = y;
        s_strip_has_vertex = 1;
    }
    else
    {
        if (x < s_strip_min_x) { s_strip_min_x = x; }
        if (x > s_strip_max_x) { s_strip_max_x = x; }
        if (y < s_strip_min_y) { s_strip_min_y = y; }
        if (y > s_strip_max_y) { s_strip_max_y = y; }
    }
    if (last && s_strips < 8)
    {
        s_strip_colour[s_strips] = argb & 0x00FFFFFFu;
    }
    if (last)
    {
        s_last_strip_min_x = s_strip_min_x;
        s_last_strip_max_x = s_strip_max_x;
        s_last_strip_min_y = s_strip_min_y;
        s_last_strip_max_y = s_strip_max_y;
        s_strip_has_vertex = 0;
        s_strips++;
        s_strips_in_scene++;
    }
}

/*----------------------
 | s_frame_texture / s_frame_textures / s_frame_size / fake_frame_texture
 | Description: Records each RGB24 frame texture load and hands back one fixed surface.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_frame_texture, s_frame_textures, s_frame_size
 | Params: pixels -- RGB565 rows, width, height -- frame size
 | Returns: the fixed surface
 ----------------------*/
static DcSurfaceTexture s_frame_texture = { (void *)0x1000, TEX_FMT_RGB565_FRAME, -1 };
static int s_frame_textures;
static unsigned s_frame_size[2];

static const DcSurfaceTexture *fake_frame_texture(const uint16_t *pixels, unsigned width, unsigned height)
{
    (void)pixels;
    s_frame_textures++;
    s_frame_size[0] = width;
    s_frame_size[1] = height;
    return &s_frame_texture;
}

static void fake_strip(const PvrVertex *verts, unsigned count)
{
    unsigned i;

    for (i = 0; i < count; i++)
    {
        fake_vertex(verts[i].x, verts[i].y, verts[i].u, verts[i].v, verts[i].argb, i + 1 == count);
    }
}

/*----------------------
 | s_mark_strip / s_patch_box / s_patches / fake_strip_mark / fake_patch_strip
 | Description: Marks hand back the index of the next strip; a patch records the box it rewrote that strip to.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_strips, s_mark_strip, s_patch_box, s_patches
 | Params: mark -- index from fake_strip_mark, verts, count -- the rewritten corners
 | Returns: fake_strip_mark -- an opaque mark
 ----------------------*/
static int s_mark_strip = -1;
static float s_patch_box[4];
static int s_patches;

static void *fake_strip_mark(void)
{
    s_mark_strip = s_strips;
    return (void *)(uintptr_t)(s_strips + 1);
}

static void fake_patch_strip(void *mark, const PvrVertex *verts, unsigned count)
{
    unsigned i;

    (void)mark;
    s_patch_box[0] = s_patch_box[2] = verts[0].x;
    s_patch_box[1] = s_patch_box[3] = verts[0].y;
    for (i = 1; i < count; i++)
    {
        s_patch_box[0] = verts[i].x < s_patch_box[0] ? verts[i].x : s_patch_box[0];
        s_patch_box[1] = verts[i].y < s_patch_box[1] ? verts[i].y : s_patch_box[1];
        s_patch_box[2] = verts[i].x > s_patch_box[2] ? verts[i].x : s_patch_box[2];
        s_patch_box[3] = verts[i].y > s_patch_box[3] ? verts[i].y : s_patch_box[3];
    }
    s_patches++;
}

/*----------------------
 | s_screen_tex / s_captures / s_capture_slot / fake_capture_screen / fake_screen_texture
 | Description: Screen slots that count captures and remember the last slot captured into.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_screen_tex, s_captures, s_capture_slot
 | Params: slot -- 0 or 1
 | Returns: fake_capture_screen -- 1; fake_screen_texture -- the slot's surface
 ----------------------*/
static DcSurfaceTexture s_screen_tex[2] = { { (void *)0x2000, TEX_FMT_RGB565_FRAME, -1 }, { (void *)0x3000, TEX_FMT_RGB565_FRAME, -1 } };
static int s_captures;
static unsigned s_capture_slot;

static int fake_capture_screen(unsigned slot)
{
    s_captures++;
    s_capture_slot = slot;
    return 1;
}

static const DcSurfaceTexture *fake_screen_texture(unsigned slot)
{
    return &s_screen_tex[slot];
}

static const PvrBackend s_fake = { fake_scene_begin, fake_scene_finish, fake_background, fake_header, fake_strip, fake_scene_shown, fake_wait_shown, fake_frame_texture,
                                   fake_strip_mark, fake_patch_strip, fake_capture_screen, fake_screen_texture };

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

static void fake_free_all(void)
{
    s_free_all_calls++;
}

static const TextureOps s_texture_ops = { fake_alloc, fake_free_all, 0 };
static const PaletteOps s_palette_ops = { fake_set_entry };

static void reset(void)
{
    s_begins = 0;
    s_finishes = 0;
    s_backgrounds = 0;
    s_strips = 0;
    s_strips_in_last_scene = 0;
    s_headers = 0;
    s_scene_begin_result = 1;
    s_scene_shown_result = 0;
    s_free_all_calls = 0;
    s_strip_has_vertex = 0;
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
    VSync(0);
    CHECK(s_strips == 3);
    CHECK(s_headers == 3);
    CHECK(s_header_log[0].src == BL_INVDSTCOLOR && s_header_log[0].dst == BL_INVSRCALPHA);
    CHECK(s_header_log[1].src == BL_ONE && s_header_log[1].dst == BL_ONE);
    CHECK(s_header_log[2].src == BL_INVDSTCOLOR && s_header_log[2].dst == BL_INVSRCALPHA);
    CHECK(s_strip_colour[0] == 0x00FFFFFFu);
    CHECK(s_strip_colour[1] == 0x00804020u);
    CHECK(s_strip_colour[2] == 0x00FFFFFFu);
}

static void test_subtractive_run_shares_one_inversion_box(void)
{
    u_long chain[2 + 3 * (1 + sizeof(TILE) / sizeof(u_long))];
    TILE *a = (TILE *)&chain[3];
    TILE *b = (TILE *)&chain[3 + 1 + sizeof(TILE) / sizeof(u_long)];
    TILE *c = (TILE *)&chain[3 + 2 * (1 + sizeof(TILE) / sizeof(u_long))];
    unsigned tile_words = 1 + sizeof(TILE) / sizeof(u_long);

    put_draw_area(0, 0, 320, 240);
    gpu_decode_set_base((uintptr_t)chain);
    setlen(&chain[0], 1);
    setaddr(&chain[0], (uint32_t)(2 * sizeof(u_long)));
    chain[1] = 0xE1000040u;
    setlen(&chain[2], 0);
    setaddr(&chain[2], (uint32_t)(3 * sizeof(u_long)));
    make_tile(a, 8, 8);
    a->code = 0x62;
    setaddr(a, (uint32_t)((3 + tile_words) * sizeof(u_long)));
    make_tile(b, 100, 50);
    b->code = 0x62;
    setaddr(b, (uint32_t)((3 + 2 * tile_words) * sizeof(u_long)));
    make_tile(c, 40, 40);
    reset();
    s_patches = 0;
    DrawOTag(&chain[0]);
    gpu_decode_set_base(0);
    CHECK(s_strips == 5);
    CHECK(s_header_log[0].src == BL_INVDSTCOLOR);
    CHECK(s_header_log[1].src == BL_ONE && s_header_log[1].dst == BL_ONE);
    CHECK(s_header_log[2].src == BL_INVDSTCOLOR);
    CHECK(s_mark_strip == 0);
    CHECK(s_patches >= 1);
    CHECK(s_patch_box[0] <= 16.0f && s_patch_box[1] <= 16.0f && s_patch_box[2] >= 232.0f && s_patch_box[3] >= 116.0f);
    CHECK(s_last_strip_min_x == 80.0f && s_last_strip_min_y == 80.0f);
    VSync(0);
    CHECK(s_strips == 5);
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

static void test_clear_after_prims_redraws_full_area(void)
{
    RECT rect = { 0, 0, 320, 240 };

    put_draw_area(0, 0, 320, 240);
    reset();
    draw_tile_ot(10, 10);
    ClearImage(&rect, 0x08, 0x10, 0x18);
    VSync(0);
    CHECK(s_strips_in_last_scene == 2);
    CHECK(s_last_strip_min_x == 0.0f && s_last_strip_min_y == 0.0f);
    CHECK(s_last_strip_max_x == 640.0f && s_last_strip_max_y == 480.0f);
}

static void saturate_texture_cache(unsigned frame, int base_tpage)
{
    DcSurfaceTexture tex;
    int i;

    for (i = 0; i < 300; i++)
    {
        if (texture_cache_lookup(base_tpage + i, 0, 0, 0, DC_DEPTH_15, 0, frame, &tex) < 0)
        {
            return;
        }
    }
}

static void test_expired_flip_wait_defers_texture_flush(void)
{
    put_draw_area(0, 0, 320, 240);
    reset();
    saturate_texture_cache(9001, 4096);
    CHECK(s_free_all_calls == 0);
    s_scene_begin_result = 0;
    draw_tile_ot(10, 10);
    VSync(0);
    CHECK(s_free_all_calls == 0);
    s_scene_begin_result = 1;
    draw_tile_ot(10, 10);
    VSync(0);
    CHECK(s_free_all_calls == 1);
}

static void test_frame_rect_promotes_only_after_flip_is_confirmed(void)
{
    int16_t x, y, w, h, ox, oy;

    s_scene_begin_result = 1;
    put_draw_area(0, 0, 320, 240);
    draw_tile_ot(10, 10);
    VSync(0);
    put_draw_area(0, 0, 64, 48);
    draw_tile_ot(10, 10);
    VSync(0);
    CHECK(pvr_frame_rect(&x, &y, &w, &h, &ox, &oy) == 1);
    CHECK(w == 320 && h == 240);

    s_scene_begin_result = 0;
    put_draw_area(0, 0, 96, 72);
    draw_tile_ot(10, 10);
    CHECK(pvr_frame_rect(&x, &y, &w, &h, &ox, &oy) == 1);
    CHECK(w == 320 && h == 240);
    VSync(0);

    s_scene_begin_result = 1;
    put_draw_area(0, 0, 128, 96);
    draw_tile_ot(10, 10);
    CHECK(pvr_frame_rect(&x, &y, &w, &h, &ox, &oy) == 1);
    CHECK(w == 96 && h == 72);
    VSync(0);
    s_scene_begin_result = 1;
}

static void test_frame_rect_promotes_lazily_once_the_flip_is_shown(void)
{
    int16_t x, y, w, h, ox, oy;

    s_scene_begin_result = 1;
    s_scene_shown_result = 1;
    put_draw_area(0, 0, 160, 120);
    draw_tile_ot(10, 10);
    VSync(0);
    put_draw_area(0, 0, 200, 150);
    draw_tile_ot(10, 10);
    s_scene_shown_result = 0;
    VSync(0);
    CHECK(pvr_frame_rect(&x, &y, &w, &h, &ox, &oy) == 1);
    CHECK(w == 160 && h == 120);

    s_scene_shown_result = 1;
    CHECK(pvr_frame_rect(&x, &y, &w, &h, &ox, &oy) == 1);
    CHECK(w == 200 && h == 150);

    s_scene_begin_result = 1;
    s_scene_shown_result = 0;
}

static void test_frame_settle_waits_for_a_closed_scene_a_read_overlaps(void)
{
    int16_t x, y, w, h, ox, oy;

    s_scene_begin_result = 1;
    s_scene_shown_result = 1;
    put_draw_area(0, 0, 320, 240);
    draw_tile_ot(10, 10);
    VSync(0);
    put_draw_area(0, 240, 320, 240);
    draw_tile_ot(10, 10);
    s_scene_shown_result = 0;
    VSync(0);
    s_wait_shown_calls = 0;
    pvr_frame_settle(512, 0, 64, 64);
    CHECK(s_wait_shown_calls == 0);
    CHECK(pvr_frame_rect(&x, &y, &w, &h, &ox, &oy) == 1);
    CHECK(y == 0);
    pvr_frame_settle(0, 240, 320, 240);
    CHECK(s_wait_shown_calls == 1);
    CHECK(pvr_frame_rect(&x, &y, &w, &h, &ox, &oy) == 1);
    CHECK(y == 240 && h == 240);
    pvr_frame_settle(0, 240, 320, 240);
    CHECK(s_wait_shown_calls == 1);

    put_draw_area(0, 240, 320, 240);
    draw_tile_ot(10, 10);
    s_scene_shown_result = 0;
    VSync(0);
    pvr_frame_settle(0, 240, 320, 240);
    CHECK(s_wait_shown_calls == 1);
    s_scene_shown_result = 0;
}

/*----------------------
 | test_rgb24_frame_is_one_opaque_full_screen_scene
 | Description: An RGB24 frame loads its texture once and draws one full-screen strip with it in a scene of its own, counted by the probe.
 | Author: suinevere
 | Dependencies: pvr_submit_rgb24, reset
 | Globals: s_frame_textures, s_frame_size, s_last_header_tex, s_finishes, s_strips_in_last_scene, s_last_strip_*, s_probe_scenes
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_flip_reports_only_scenes_that_drew(void)
{
    s_scene_begin_result = 1;
    put_draw_area(0, 0, 320, 240);
    VSync(0);
    s_probe_drawn = 0;
    draw_tile_ot(10, 10);
    VSync(0);
    CHECK(s_probe_drawn == 1);
    ClearImage(&(RECT){ 0, 0, 320, 240 }, 0, 0, 0);
    VSync(0);
    CHECK(s_probe_drawn == 1);
}

static void test_rgb24_frame_is_one_opaque_full_screen_scene(void)
{
    static uint16_t pixels[512 * 256];
    int probes;

    reset();
    s_frame_textures = 0;
    probes = s_probe_scenes;
    pvr_submit_rgb24(pixels, 320, 240);
    CHECK(s_frame_textures == 1);
    CHECK(s_frame_size[0] == 320 && s_frame_size[1] == 240);
    CHECK(s_last_header_tex == &s_frame_texture);
    CHECK(s_finishes == 1);
    CHECK(s_strips_in_last_scene == 1);
    CHECK(s_last_strip_min_x == 0.0f && s_last_strip_max_x == 640.0f);
    CHECK(s_last_strip_min_y == 0.0f && s_last_strip_max_y == 480.0f);
    CHECK(s_probe_scenes == probes + 1);
}

/*----------------------
 | draw_cover_ot
 | Description: Draws one tile spanning the whole 320x240 draw area through an ordering table, semi-transparent or not.
 | Author: suinevere
 | Dependencies: make_tile, DrawOTag
 | Globals: N/A
 | Params: semi -- nonzero for a semi-transparent tile
 | Returns: N/A
 ----------------------*/
static void draw_cover_ot(int semi)
{
    u_long chain[1 + sizeof(TILE) / sizeof(u_long)];
    TILE *tile = (TILE *)&chain[1];

    gpu_decode_set_base((uintptr_t)chain);
    setlen(&chain[0], 0);
    setaddr(&chain[0], (uint32_t)sizeof(u_long));
    make_tile(tile, 0, 0);
    tile->w = 320;
    tile->h = 240;
    if (semi)
    {
        setSemiTrans(tile, 1);
    }
    DrawOTag(&chain[0]);
    gpu_decode_set_base(0);
}

static void put_keep_area(int y)
{
    DRAWENV env;

    SetDefDrawEnv(&env, 0, y, 320, 240);
    env.isbg = 0;
    PutDrawEnv(&env);
}

/*----------------------
 | test_keep_screen_scene_starts_with_what_its_area_last_showed
 | Description: A non-clearing scene whose first primitive is a blended full-area tile first draws a carried screen: the shown frame the first time, then the frame its own area showed one scene earlier.
 | Author: suinevere
 | Dependencies: put_keep_area, draw_cover_ot, draw_tile_ot, reset
 | Globals: s_captures, s_capture_slot, s_header_log, s_last_header_tex, s_scene_shown_result
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_keep_screen_scene_starts_with_what_its_area_last_showed(void)
{
    unsigned first;

    s_scene_begin_result = 1;
    s_scene_shown_result = 1;
    put_keep_area(0);
    draw_tile_ot(10, 10);
    VSync(0);
    put_keep_area(240);
    draw_tile_ot(10, 10);
    VSync(0);
    reset();
    s_scene_shown_result = 1;
    s_captures = 0;
    put_keep_area(0);
    draw_cover_ot(1);
    CHECK(s_captures == 1);
    first = s_capture_slot;
    CHECK(s_headers >= 2);
    CHECK(s_header_log[0].src == BL_SRCALPHA && s_header_log[0].dst == BL_INVSRCALPHA && s_header_log[0].alpha == 255);
    VSync(0);
    CHECK(s_strips_in_last_scene >= 2);

    s_headers = 0;
    put_keep_area(240);
    draw_cover_ot(1);
    CHECK(s_captures == 2);
    CHECK(s_capture_slot != first);
    CHECK(s_first_header_tex == &s_screen_tex[first]);
    VSync(0);

    s_headers = 0;
    s_last_header_tex = 0;
    put_keep_area(0);
    draw_cover_ot(1);
    CHECK(s_captures == 3);
    CHECK(s_capture_slot == first);
    CHECK(s_first_header_tex == &s_screen_tex[!first]);
    VSync(0);
    s_scene_shown_result = 0;
}

/*----------------------
 | test_keep_screen_carry_needs_a_blended_full_area_lead
 | Description: A clearing scene, an opaque full-area tile, or a blended full-area tile after several primitives does not carry the screen.
 | Author: suinevere
 | Dependencies: put_draw_area, put_keep_area, draw_cover_ot, draw_tile_ot
 | Globals: s_captures, s_scene_shown_result
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_keep_screen_carry_needs_a_blended_full_area_lead(void)
{
    DRAWENV env;

    s_scene_begin_result = 1;
    s_scene_shown_result = 1;
    put_keep_area(0);
    draw_tile_ot(10, 10);
    VSync(0);
    s_captures = 0;

    SetDefDrawEnv(&env, 0, 240, 320, 240);
    env.isbg = 1;
    PutDrawEnv(&env);
    draw_cover_ot(1);
    VSync(0);
    CHECK(s_captures == 0);

    put_keep_area(0);
    draw_cover_ot(0);
    VSync(0);
    CHECK(s_captures == 0);

    put_keep_area(240);
    draw_tile_ot(10, 10);
    draw_tile_ot(30, 10);
    draw_tile_ot(50, 10);
    draw_cover_ot(1);
    VSync(0);
    CHECK(s_captures == 0);
    s_scene_shown_result = 0;
}

/*----------------------
 | test_flip_leaves_no_scene_with_prims
 | Description: Once a scene with primitives is flipped, pvr_submit_scene_has_prims reports none until another primitive opens a scene.
 | Author: suinevere
 | Dependencies: put_draw_area, draw_tile_ot, pvr_submit_scene_has_prims
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_flip_leaves_no_scene_with_prims(void)
{
    s_scene_begin_result = 1;
    put_draw_area(0, 0, 320, 240);
    draw_tile_ot(10, 10);
    CHECK(pvr_submit_scene_has_prims() == 1);
    VSync(0);
    CHECK(pvr_submit_scene_has_prims() == 0);
}

int main(void)
{
    texture_cache_init(&s_texture_ops);
    palette_bank_init(&s_palette_ops);
    pvr_submit_set_backend(&s_fake);
    test_vsync1_does_not_split_the_scene();
    test_rgb24_frame_is_one_opaque_full_screen_scene();
    test_negative_vsync_does_not_flip();
    test_clear_only_frame_shows_its_background();
    test_isbg_only_frame_shows_its_background();
    test_partial_clear_only_frame_draws_its_rect();
    test_subtractive_tile_inverts_adds_and_inverts();
    test_subtractive_run_shares_one_inversion_box();
    test_bright_textured_sprite_adds_its_excess();
    test_clear_after_prims_redraws_full_area();
    test_expired_flip_wait_defers_texture_flush();
    test_frame_rect_promotes_only_after_flip_is_confirmed();
    test_frame_rect_promotes_lazily_once_the_flip_is_shown();
    test_flip_reports_only_scenes_that_drew();
    test_frame_settle_waits_for_a_closed_scene_a_read_overlaps();
    test_keep_screen_scene_starts_with_what_its_area_last_showed();
    test_keep_screen_carry_needs_a_blended_full_area_lead();
    test_flip_leaves_no_scene_with_prims();

    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
