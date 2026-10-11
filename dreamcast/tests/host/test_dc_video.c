/*----------------------
 | test_dc_video.c
 | Description: Host tests for the Dreamcast VRAM mirror, ordering-table walker and libgpu wiring: wraparound, MoveImage overlap, ClearImage backgrounds and rects, DrawOTag/DrawPrim submission and VSync flips.
 | Author: suinevere
 | Dependencies: vram_mirror.h, ot_walk.h, gpu_tag.h, gpu_decode.h, pvr_submit.h, libgpu_dc.c, gpu_decode.c, gpu_census.c, lom_prefix.h (forced include)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "vram_mirror.h"
#include "ot_walk.h"
#include "gpu_tag.h"
#include "gpu_decode.h"
#include "pvr_submit.h"
#include "libgpu_dc.h"
#include "libetc.h"
#include "vsync_pacer.h"

static int failures;
static int s_halts;
static char s_last_halt[64];
static int s_bg_calls;
static uint16_t s_bg_colour;
static int s_rect_calls;
static int s_rect[4];
static uint16_t s_rect_colour;
static int s_prim_calls;
static DcPrim s_last_prim;
static DcGpuState s_last_state;
static int s_origin_at_prim[2];
static int s_flips;
static int s_origin_calls;
static int s_origin[2];

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

void pvr_submit_prim(const DcPrim *prim, const DcGpuState *state)
{
    s_prim_calls++;
    s_last_prim = *prim;
    s_last_state = *state;
    s_origin_at_prim[0] = s_origin[0];
    s_origin_at_prim[1] = s_origin[1];
}

void pvr_submit_flip(void)
{
    s_flips++;
}

void pvr_submit_origin(int x, int y)
{
    s_origin_calls++;
    s_origin[0] = x;
    s_origin[1] = y;
}

void pvr_submit_background(uint16_t ps1_colour)
{
    s_bg_calls++;
    s_bg_colour = ps1_colour;
}

void pvr_submit_rect(int x, int y, int w, int h, uint16_t ps1_colour)
{
    s_rect_calls++;
    s_rect[0] = x;
    s_rect[1] = y;
    s_rect[2] = w;
    s_rect[3] = h;
    s_rect_colour = ps1_colour;
}

static void reset_calls(void)
{
    s_bg_calls = 0;
    s_rect_calls = 0;
    s_prim_calls = 0;
    s_flips = 0;
}

static void put_draw_area(int x, int y, int w, int h)
{
    DRAWENV env;

    SetDefDrawEnv(&env, x, y, w, h);
    PutDrawEnv(&env);
}

void dc_halt(const char *message)
{
    s_halts++;
    strncpy(s_last_halt, message, sizeof(s_last_halt) - 1);
    s_last_halt[sizeof(s_last_halt) - 1] = '\0';
}

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static void test_load_store_round_trip(void)
{
    RECT rect = { 100, 10, 4, 2 };
    u_long src[4 * 2];
    u_long dst[4 * 2];
    int i;

    for (i = 0; i < 8; i++)
    {
        ((unsigned short *)src)[i] = (unsigned short)(0x1000 + i);
    }
    CHECK(LoadImage(&rect, src) == 0);
    CHECK(StoreImage(&rect, dst) == 0);
    for (i = 0; i < 8; i++)
    {
        CHECK(((unsigned short *)dst)[i] == (unsigned short)(0x1000 + i));
    }
}

static void test_reloading_identical_pixels_keeps_the_generation(void)
{
    RECT rect = { 600, 300, 4, 2 };
    u_long src[4 * 2];
    uint32_t before;
    int i;

    for (i = 0; i < 8; i++)
    {
        ((unsigned short *)src)[i] = (unsigned short)(0x2000 + i);
    }
    LoadImage(&rect, src);
    before = vram_mirror_generation(600, 300, 4, 2);
    LoadImage(&rect, src);
    CHECK(vram_mirror_generation(600, 300, 4, 2) == before);
    ((unsigned short *)src)[5] = 0x1234;
    LoadImage(&rect, src);
    CHECK(vram_mirror_generation(600, 300, 4, 2) != before);
}

static void test_load_store_wraps_x(void)
{
    RECT rect = { 1022, 20, 4, 1 };
    u_long src[4];
    u_long dst[4];
    int i;

    for (i = 0; i < 4; i++)
    {
        ((unsigned short *)src)[i] = (unsigned short)(0x2000 + i);
    }
    LoadImage(&rect, src);
    CHECK(*vram_mirror_at(1022, 20) == 0x2000);
    CHECK(*vram_mirror_at(1023, 20) == 0x2001);
    CHECK(*vram_mirror_at(0, 20) == 0x2002);
    CHECK(*vram_mirror_at(1, 20) == 0x2003);
    StoreImage(&rect, dst);
    for (i = 0; i < 4; i++)
    {
        CHECK(((unsigned short *)dst)[i] == (unsigned short)(0x2000 + i));
    }
}

static void test_move_image_copies(void)
{
    RECT rect = { 200, 30, 3, 2 };
    u_long src[3 * 2];
    u_long dst[3 * 2];
    int i;

    for (i = 0; i < 6; i++)
    {
        ((unsigned short *)src)[i] = (unsigned short)(0x3000 + i);
    }
    LoadImage(&rect, src);
    CHECK(MoveImage(&rect, 300, 40) == 0);
    rect.x = 300;
    rect.y = 40;
    StoreImage(&rect, dst);
    for (i = 0; i < 6; i++)
    {
        CHECK(((unsigned short *)dst)[i] == (unsigned short)(0x3000 + i));
    }
}

static void test_load_wraps_y(void)
{
    RECT rect = { 500, 510, 1, 4 };
    u_long src[2];
    int i;

    for (i = 0; i < 4; i++)
    {
        ((unsigned short *)src)[i] = (unsigned short)(0x4000 + i);
    }
    LoadImage(&rect, src);
    CHECK(*vram_mirror_at(500, 510) == 0x4000);
    CHECK(*vram_mirror_at(500, 511) == 0x4001);
    CHECK(*vram_mirror_at(500, 0) == 0x4002);
    CHECK(*vram_mirror_at(500, 1) == 0x4003);
}

static void test_load_wraps_both_axes(void)
{
    RECT rect = { 1022, 510, 4, 4 };
    u_long src[8];
    int i;

    for (i = 0; i < 16; i++)
    {
        ((unsigned short *)src)[i] = (unsigned short)(0x5000 + i);
    }
    LoadImage(&rect, src);
    CHECK(*vram_mirror_at(1022, 510) == 0x5000);
    CHECK(*vram_mirror_at(1, 510) == 0x5003);
    CHECK(*vram_mirror_at(1023, 511) == 0x5005);
    CHECK(*vram_mirror_at(0, 0) == 0x500A);
    CHECK(*vram_mirror_at(1, 1) == 0x500F);
}

static void check_move_as_if_temporary(int sx, int sy, int w, int h, int dx, int dy)
{
    static unsigned short before[16][16];
    RECT rect = { (short)sx, (short)sy, (short)w, (short)h };
    int row;
    int col;

    for (row = 0; row < h + 4; row++)
    {
        for (col = 0; col < w + 4; col++)
        {
            RECT one = { (short)(sx - 2 + col), (short)(sy - 2 + row), 1, 1 };
            u_long v = (u_long)(0x6000 + row * 16 + col);

            LoadImage(&one, &v);
        }
    }
    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            before[row][col] = *vram_mirror_at(sx + col, sy + row);
        }
    }
    CHECK(MoveImage(&rect, dx, dy) == 0);
    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            CHECK(*vram_mirror_at(dx + col, dy + row) == before[row][col]);
        }
    }
}

static void check_column_move(int x, int sy, int h, int dy)
{
    static unsigned short before[512];
    static unsigned short expected[512];
    static u_long column[256];
    RECT all = { (short)x, 0, 1, 512 };
    RECT rect = { (short)x, (short)sy, 1, (short)h };
    int row;

    for (row = 0; row < 512; row++)
    {
        ((unsigned short *)column)[row] = (unsigned short)(0x7000 + row);
    }
    LoadImage(&all, column);
    for (row = 0; row < 512; row++)
    {
        before[row] = *vram_mirror_at(x, row);
        expected[row] = before[row];
    }
    for (row = 0; row < h; row++)
    {
        expected[(dy + row) % 512] = before[(sy + row) % 512];
    }
    CHECK(MoveImage(&rect, x, dy) == 0);
    for (row = 0; row < 512; row++)
    {
        CHECK(*vram_mirror_at(x, row) == expected[row]);
    }
}

static void test_move_overlap_as_if_temporary(void)
{
    check_move_as_if_temporary(600, 100, 4, 4, 601, 101);
    check_move_as_if_temporary(602, 120, 4, 4, 600, 119);
    check_move_as_if_temporary(610, 140, 6, 3, 612, 140);
    check_move_as_if_temporary(700, 510, 2, 4, 700, 0);
    check_move_as_if_temporary(1022, 0, 4, 2, 0, 0);
    check_column_move(800, 0, 300, 256);
    check_column_move(810, 10, 300, 267);
    check_column_move(820, 5, 400, 133);
}

static void test_move_full_height_rotates(void)
{
    static u_long column[256];
    RECT rect = { 800, 0, 1, 512 };
    int row;

    for (row = 0; row < 512; row++)
    {
        ((unsigned short *)column)[row] = (unsigned short)row;
    }
    LoadImage(&rect, column);
    CHECK(MoveImage(&rect, 800, 6) == 0);
    for (row = 0; row < 512; row++)
    {
        CHECK(*vram_mirror_at(800, 6 + row) == (unsigned short)row);
    }
}

static void test_clear_image_colour(void)
{
    RECT rect = { 400, 50, 2, 2 };

    CHECK(ClearImage(&rect, 0xF8, 0, 0x08) == 0);
    CHECK(*vram_mirror_at(400, 50) == 0x041F);
    CHECK(*vram_mirror_at(401, 51) == 0x041F);
}

static void test_ot_counts_nonempty_packets(void)
{
    u_long chain[3];
    uintptr_t base = (uintptr_t)chain;

    ot_walk_set_base(base);
    setlen(&chain[0], 0);
    setaddr(&chain[0], (uint32_t)sizeof(u_long));
    setlen(&chain[1], 3);
    setaddr(&chain[1], (uint32_t)(2 * sizeof(u_long)));
    setlen(&chain[2], 3);
    setaddr(&chain[2], GPU_TAG_END);
    CHECK(ot_walk_count(&chain[0]) == 2);
    ot_walk_set_base(0);
}

static void test_ot_follows_host_mapped_packet(void)
{
    u_long packet[4];
    u_long head[1];
    uintptr_t base = (uintptr_t)packet - 0xF00000u;

    ot_walk_set_base(base);
    setlen(packet, 3);
    setaddr(packet, GPU_TAG_END);
    setlen(head, 0);
    setaddr(head, 0xF00000u);
    CHECK(ot_walk_count(head) == 1);
    ot_walk_set_base(0);
}

static void test_ot_self_loop_halts(void)
{
    u_long tag[1];
    uintptr_t tag_addr = (uintptr_t)tag;
    uint32_t field = (uint32_t)(tag_addr & 0x00FFFFFFu);

    ot_walk_set_base(tag_addr - field);
    setlen(tag, 0);
    setaddr(tag, field);
    s_halts = 0;
    s_last_halt[0] = '\0';
    ot_walk_count(tag);
    CHECK(s_halts == 1);
    CHECK(strcmp(s_last_halt, "ot_walk: loop") == 0);
    ot_walk_set_base(0);
}

static void test_clear_whole_area_sets_background(void)
{
    RECT rect = { 0, 240, 320, 240 };

    put_draw_area(0, 240, 320, 240);
    reset_calls();
    ClearImage(&rect, 0x10, 0x20, 0x30);
    CHECK(s_bg_calls == 1);
    CHECK(s_bg_colour == 0x1882);
    CHECK(s_rect_calls == 0);
    CHECK(*vram_mirror_at(5, 300) == 0x1882);
}

static void test_clear_part_of_area_emits_rect(void)
{
    RECT rect = { 300, 230, 40, 20 };

    put_draw_area(0, 240, 320, 240);
    reset_calls();
    ClearImage(&rect, 0xF8, 0, 0);
    CHECK(s_bg_calls == 0);
    CHECK(s_rect_calls == 1);
    CHECK(s_rect[0] == 300 && s_rect[1] == 0 && s_rect[2] == 20 && s_rect[3] == 10);
    CHECK(s_rect_colour == 0x001F);
}

static void test_clear_outside_area_draws_nothing(void)
{
    RECT rect = { 0, 0, 320, 240 };

    put_draw_area(0, 240, 320, 240);
    reset_calls();
    ClearImage(&rect, 0, 0, 0);
    CHECK(s_bg_calls == 0);
    CHECK(s_rect_calls == 0);
}

static void test_put_draw_env_isbg_sets_background(void)
{
    DRAWENV env;

    SetDefDrawEnv(&env, 0, 0, 320, 240);
    env.isbg = 1;
    env.r0 = 0;
    env.g0 = 0xF8;
    env.b0 = 0;
    reset_calls();
    PutDrawEnv(&env);
    CHECK(s_bg_calls == 1);
    CHECK(s_bg_colour == 0x03E0);
    env.isbg = 0;
    reset_calls();
    PutDrawEnv(&env);
    CHECK(s_bg_calls == 0);
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
}

static void test_draw_otag_submits_with_env_state(void)
{
    u_long chain[1 + sizeof(TILE) / sizeof(u_long)];
    TILE *tile = (TILE *)&chain[1];

    gpu_decode_set_base((uintptr_t)chain);
    setlen(&chain[0], 0);
    setaddr(&chain[0], (uint32_t)sizeof(u_long));
    make_tile(tile, 10, 20);
    setaddr(tile, GPU_TAG_END);
    put_draw_area(0, 240, 320, 240);
    reset_calls();
    DrawOTag(&chain[0]);
    CHECK(s_prim_calls == 1);
    CHECK(s_last_state.area_y1 == 240 && s_last_state.area_y2 == 479);
    CHECK(s_last_state.ofs_y == 240);
    CHECK(s_last_prim.shape == DC_SHAPE_RECT);
    CHECK(s_last_prim.v[0].x == 10 && s_last_prim.v[0].y == 260);
    CHECK(dc_frame_primitives() == 1);
    gpu_decode_set_base(0);
}

static void test_draw_prim_submits_one_packet(void)
{
    TILE tile;

    make_tile(&tile, 4, 6);
    setaddr(&tile, GPU_TAG_END);
    put_draw_area(0, 0, 320, 240);
    reset_calls();
    DrawPrim(&tile);
    CHECK(s_prim_calls == 1);
    CHECK(s_last_prim.v[0].x == 4 && s_last_prim.v[0].y == 6);
}

static void put_disp_area(int x, int y, int w, int h)
{
    DISPENV env;

    SetDefDispEnv(&env, x, y, w, h);
    PutDispEnv(&env);
}

static void test_origin_follows_the_display_showing_the_draw_area(void)
{
    put_disp_area(0, 0, 320, 240);
    put_disp_area(0, 232, 320, 240);
    s_origin_calls = 0;
    put_draw_area(0, 240, 320, 224);
    CHECK(s_origin_calls == 1);
    CHECK(s_origin[0] == 0 && s_origin[1] == 8);
    put_disp_area(0, 0, 320, 240);
    put_draw_area(0, 8, 320, 224);
    CHECK(s_origin[0] == 0 && s_origin[1] == 8);
    put_draw_area(400, 300, 64, 64);
    CHECK(s_origin[0] == 0 && s_origin[1] == 0);
}

static void test_draw_area_packet_moves_the_origin_for_later_prims(void)
{
    u_long chain[2 + 1 + sizeof(TILE) / sizeof(u_long)];
    TILE *tile = (TILE *)&chain[3];

    put_disp_area(0, 0, 320, 240);
    put_disp_area(0, 240, 320, 240);
    put_draw_area(0, 240, 320, 240);
    gpu_decode_set_base((uintptr_t)chain);
    setlen(&chain[0], 1);
    setaddr(&chain[0], (uint32_t)(2 * sizeof(u_long)));
    chain[1] = 0xE3000000u | (280u << 10) | 40u;
    setlen(&chain[2], 0);
    setaddr(&chain[2], (uint32_t)(3 * sizeof(u_long)));
    make_tile(tile, 50, 50);
    setaddr(tile, GPU_TAG_END);
    reset_calls();
    DrawOTag(&chain[0]);
    CHECK(s_prim_calls == 1);
    CHECK(s_last_state.area_x1 == 40 && s_last_state.area_y1 == 280);
    CHECK(s_origin_at_prim[0] == 40 && s_origin_at_prim[1] == 40);
    gpu_decode_set_base(0);
}

static void test_vsync_flips_only_on_waiting_modes(void)
{
    reset_calls();
    VSync(-1);
    CHECK(s_flips == 0);
    VSync(1);
    CHECK(s_flips == 0);
    VSync(0);
    CHECK(s_flips == 1);
}

int main(void)
{
    test_load_store_round_trip();
    test_load_store_wraps_x();
    test_reloading_identical_pixels_keeps_the_generation();
    test_move_image_copies();
    test_load_wraps_y();
    test_load_wraps_both_axes();
    test_move_overlap_as_if_temporary();
    test_move_full_height_rotates();
    test_clear_image_colour();
    test_ot_counts_nonempty_packets();
    test_ot_follows_host_mapped_packet();
    test_ot_self_loop_halts();
    test_clear_whole_area_sets_background();
    test_clear_part_of_area_emits_rect();
    test_clear_outside_area_draws_nothing();
    test_put_draw_env_isbg_sets_background();
    test_draw_otag_submits_with_env_state();
    test_draw_prim_submits_one_packet();
    test_vsync_flips_only_on_waiting_modes();
    test_origin_follows_the_display_showing_the_draw_area();
    test_draw_area_packet_moves_the_origin_for_later_prims();

    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
