/*----------------------
 | test_gpu_decode.c
 | Description: Host tests for the PS1 GP0 packet decoder and primitive census: polygon, sprite, tile and polyline decoding, environment commands, 11-bit vertex sign extension, unknown codes, GP0 fill and OT walking.
 | Author: suinevere
 | Dependencies: gpu_decode.h, gpu_census.h, gpu_tag.h, pvr_submit.h (stubbed), vram_mirror.h, gpu_decode.c, gpu_census.c, libgpu_dc.c (SetDrawEnv), vram_mirror.c, ot_walk.c, vsync_pacer.c, lom_prefix.h (forced include)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gpu_decode.h"
#include "gpu_census.h"
#include "gpu_tag.h"
#include "pvr_submit.h"
#include "vram_mirror.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

#define MAX_RECORDED 8

static DcPrim s_recorded[MAX_RECORDED];
static DcGpuState s_recorded_state[MAX_RECORDED];
static unsigned s_recorded_count;
static char s_last_log[256];

void dc_halt(const char *message)
{
    (void)message;
}

void pvr_submit_prim(const DcPrim *prim, const DcGpuState *state)
{
    (void)prim;
    (void)state;
}

/*----------------------
 | pvr_submit_rgb24 / cdrom_dc_vsync_tick
 | Description: Host stand-ins for the RGB24 present and the CD stream delivery libgpu_dc.c calls.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: pixels, width, height -- ignored
 | Returns: N/A
 ----------------------*/
void pvr_submit_rgb24(const uint16_t *pixels, unsigned width, unsigned height)
{
    (void)pixels;
    (void)width;
    (void)height;
}

void cdrom_dc_vsync_tick(void)
{
}

void pvr_submit_flip(void)
{
}

void pvr_submit_origin(int x, int y)
{
    (void)x;
    (void)y;
}

void pvr_submit_background(uint16_t ps1_colour)
{
    (void)ps1_colour;
}

void pvr_submit_rect(int x, int y, int w, int h, uint16_t ps1_colour)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)ps1_colour;
}

int pvr_submit_scene_has_prims(void)
{
    return 0;
}

void pvr_frame_settle(int x, int y, int w, int h)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
}

int pvr_frame_rect(int16_t *x, int16_t *y, int16_t *w, int16_t *h, int16_t *origin_x, int16_t *origin_y)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)origin_x;
    (void)origin_y;
    return 0;
}

const uint16_t *pvr_backend_frame_pixels(unsigned *pitch_px)
{
    (void)pitch_px;
    return 0;
}

void dc_log(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(s_last_log, sizeof(s_last_log), format, args);
    va_end(args);
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

static void reset_recorder(void)
{
    s_recorded_count = 0;
}

static void record_sink(const DcPrim *prim, const DcGpuState *state, void *ctx)
{
    (void)ctx;
    if (s_recorded_count < MAX_RECORDED)
    {
        s_recorded[s_recorded_count] = *prim;
        s_recorded_state[s_recorded_count] = *state;
        s_recorded_count++;
    }
}

static uint32_t cw(unsigned r, unsigned g, unsigned b, unsigned code)
{
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)code << 24);
}

static uint32_t xyw(int x, int y)
{
    return ((uint32_t)(uint16_t)x) | (((uint32_t)(uint16_t)y) << 16);
}

static uint32_t uvw(unsigned u, unsigned v, unsigned hi)
{
    return (uint32_t)u | ((uint32_t)v << 8) | ((uint32_t)hi << 16);
}

static void test_poly_ft4(void)
{
    DcGpuState state;
    uint32_t words[9];

    gpu_decode_reset(&state);
    state.ofs_x = 5;
    state.ofs_y = 7;
    words[0] = cw(0x50, 0x60, 0x70, 0x2F);
    words[1] = xyw(10, 20);
    words[2] = uvw(1, 2, 0x0041);
    words[3] = xyw(74, 20);
    words[4] = uvw(50, 2, 0x0153);
    words[5] = xyw(10, 84);
    words[6] = uvw(1, 86, 0);
    words[7] = xyw(74, 84);
    words[8] = uvw(50, 86, 0);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 9, record_sink, NULL) == 9);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_QUAD);
    CHECK(s_recorded[0].nverts == 4);
    CHECK(s_recorded[0].textured == 1);
    CHECK(s_recorded[0].raw == 1);
    CHECK(s_recorded[0].semi == 1);
    CHECK(s_recorded[0].gouraud == 0);
    CHECK(s_recorded[0].abr == 2);
    CHECK(s_recorded[0].depth == DC_DEPTH_15);
    CHECK(s_recorded[0].tpage_x == 192);
    CHECK(s_recorded[0].tpage_y == 256);
    CHECK(s_recorded[0].clut_x == 16);
    CHECK(s_recorded[0].clut_y == 1);
    CHECK(s_recorded[0].v[0].x == 15 && s_recorded[0].v[0].y == 27);
    CHECK(s_recorded[0].v[0].u == 1 && s_recorded[0].v[0].v == 2);
    CHECK(s_recorded[0].v[1].x == 79 && s_recorded[0].v[1].y == 27);
    CHECK(s_recorded[0].v[1].u == 50 && s_recorded[0].v[1].v == 2);
    CHECK(s_recorded[0].v[2].x == 15 && s_recorded[0].v[2].y == 91);
    CHECK(s_recorded[0].v[3].x == 79 && s_recorded[0].v[3].y == 91);
    CHECK(s_recorded[0].v[0].r == 0x50 && s_recorded[0].v[0].g == 0x60 && s_recorded[0].v[0].b == 0x70);
    CHECK(s_recorded[0].v[3].r == 0x50 && s_recorded[0].v[3].g == 0x60 && s_recorded[0].v[3].b == 0x70);
}

static void test_poly_g3(void)
{
    DcGpuState state;
    uint32_t words[6];

    gpu_decode_reset(&state);
    words[0] = cw(10, 20, 30, 0x30);
    words[1] = xyw(100, 50);
    words[2] = cw(40, 50, 60, 0);
    words[3] = xyw(150, 50);
    words[4] = cw(70, 80, 90, 0);
    words[5] = xyw(100, 150);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 6, record_sink, NULL) == 6);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_TRI);
    CHECK(s_recorded[0].nverts == 3);
    CHECK(s_recorded[0].textured == 0);
    CHECK(s_recorded[0].gouraud == 1);
    CHECK(s_recorded[0].v[0].x == 100 && s_recorded[0].v[0].y == 50);
    CHECK(s_recorded[0].v[0].r == 10 && s_recorded[0].v[0].g == 20 && s_recorded[0].v[0].b == 30);
    CHECK(s_recorded[0].v[1].x == 150 && s_recorded[0].v[1].y == 50);
    CHECK(s_recorded[0].v[1].r == 40 && s_recorded[0].v[1].g == 50 && s_recorded[0].v[1].b == 60);
    CHECK(s_recorded[0].v[2].x == 100 && s_recorded[0].v[2].y == 150);
    CHECK(s_recorded[0].v[2].r == 70 && s_recorded[0].v[2].g == 80 && s_recorded[0].v[2].b == 90);
}

static void test_poly_gt4(void)
{
    DcGpuState state;
    uint32_t words[12];

    gpu_decode_reset(&state);
    words[0] = cw(5, 6, 7, 0x3C);
    words[1] = xyw(0, 0);
    words[2] = uvw(0, 0, 0x0010);
    words[3] = cw(15, 16, 17, 0);
    words[4] = xyw(40, 0);
    words[5] = uvw(10, 0, 0);
    words[6] = cw(25, 26, 27, 0);
    words[7] = xyw(40, 40);
    words[8] = uvw(20, 20, 0);
    words[9] = cw(35, 36, 37, 0);
    words[10] = xyw(0, 40);
    words[11] = uvw(30, 30, 0);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 12, record_sink, NULL) == 12);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_QUAD);
    CHECK(s_recorded[0].textured == 1);
    CHECK(s_recorded[0].gouraud == 1);
    CHECK(s_recorded[0].v[0].u == 0 && s_recorded[0].v[0].v == 0);
    CHECK(s_recorded[0].v[0].r == 5 && s_recorded[0].v[0].g == 6 && s_recorded[0].v[0].b == 7);
    CHECK(s_recorded[0].v[1].x == 40 && s_recorded[0].v[1].y == 0);
    CHECK(s_recorded[0].v[1].u == 10 && s_recorded[0].v[1].v == 0);
    CHECK(s_recorded[0].v[1].r == 15 && s_recorded[0].v[1].g == 16 && s_recorded[0].v[1].b == 17);
    CHECK(s_recorded[0].v[2].x == 40 && s_recorded[0].v[2].y == 40);
    CHECK(s_recorded[0].v[2].u == 20 && s_recorded[0].v[2].v == 20);
    CHECK(s_recorded[0].v[2].r == 25 && s_recorded[0].v[2].g == 26 && s_recorded[0].v[2].b == 27);
    CHECK(s_recorded[0].v[3].x == 0 && s_recorded[0].v[3].y == 40);
    CHECK(s_recorded[0].v[3].u == 30 && s_recorded[0].v[3].v == 30);
    CHECK(s_recorded[0].v[3].r == 35 && s_recorded[0].v[3].g == 36 && s_recorded[0].v[3].b == 37);
}

static void test_sprt(void)
{
    DcGpuState state;
    uint32_t words[5];

    gpu_decode_reset(&state);
    words[0] = 0xE1000000u | 0x00A2u;
    words[1] = cw(128, 128, 128, 0x64);
    words[2] = xyw(20, 30);
    words[3] = uvw(4, 5, 0x0020);
    words[4] = xyw(16, 24);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 5, record_sink, NULL) == 5);
    CHECK(state.tpage == 0x00A2u);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_RECT);
    CHECK(s_recorded[0].nverts == 2);
    CHECK(s_recorded[0].textured == 1);
    CHECK(s_recorded[0].v[0].x == 20 && s_recorded[0].v[0].y == 30);
    CHECK(s_recorded[0].v[0].u == 4 && s_recorded[0].v[0].v == 5);
    CHECK(s_recorded[0].v[1].x == 36 && s_recorded[0].v[1].y == 54);
    CHECK(s_recorded[0].v[1].u == 20 && s_recorded[0].v[1].v == 29);
    CHECK(s_recorded[0].clut_x == 512 && s_recorded[0].clut_y == 0);
    CHECK(s_recorded[0].abr == 1 && s_recorded[0].depth == DC_DEPTH_8);
    CHECK(s_recorded[0].tpage_x == 128 && s_recorded[0].tpage_y == 0);
}

static void test_sprt16(void)
{
    DcGpuState state;
    uint32_t words[4];

    gpu_decode_reset(&state);
    words[0] = 0xE1000000u | 0x0055u;
    words[1] = cw(10, 20, 30, 0x7C);
    words[2] = xyw(200, 10);
    words[3] = uvw(2, 3, 0x0005);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 4, record_sink, NULL) == 4);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_RECT);
    CHECK(s_recorded[0].v[0].x == 200 && s_recorded[0].v[0].y == 10);
    CHECK(s_recorded[0].v[1].x == 216 && s_recorded[0].v[1].y == 26);
    CHECK(s_recorded[0].v[0].u == 2 && s_recorded[0].v[0].v == 3);
    CHECK(s_recorded[0].v[1].u == 18 && s_recorded[0].v[1].v == 19);
    CHECK(s_recorded[0].clut_x == 80 && s_recorded[0].clut_y == 0);
}

static void test_tile(void)
{
    DcGpuState state;
    uint32_t words[3];

    gpu_decode_reset(&state);
    words[0] = cw(9, 8, 7, 0x60);
    words[1] = xyw(5, 5);
    words[2] = xyw(40, 24);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 3, record_sink, NULL) == 3);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_RECT);
    CHECK(s_recorded[0].textured == 0);
    CHECK(s_recorded[0].v[0].x == 5 && s_recorded[0].v[0].y == 5);
    CHECK(s_recorded[0].v[1].x == 45 && s_recorded[0].v[1].y == 29);
    CHECK(s_recorded[0].v[0].r == 9 && s_recorded[0].v[0].g == 8 && s_recorded[0].v[0].b == 7);
}

static void test_env_e3_e4_e5(void)
{
    DcGpuState state;
    uint32_t words[3];

    gpu_decode_reset(&state);
    words[0] = 0xE3000000u | 12u | (34u << 10);
    words[1] = 0xE4000000u | 312u | (234u << 10);
    words[2] = 0xE5000000u | 0x7FBu | (0x010u << 11);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 3, record_sink, NULL) == 3);
    CHECK(state.area_x1 == 12 && state.area_y1 == 34);
    CHECK(state.area_x2 == 312 && state.area_y2 == 234);
    CHECK(state.ofs_x == -5);
    CHECK(state.ofs_y == 16);
}

static void test_vertex_sign_extend(void)
{
    DcGpuState state;
    uint32_t words[4];

    gpu_decode_reset(&state);
    state.ofs_x = 16;
    words[0] = cw(1, 2, 3, 0x20);
    words[1] = xyw(0x400, 0);
    words[2] = xyw(-1000, 0);
    words[3] = xyw(-1000, 10);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 4, record_sink, NULL) == 4);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].v[0].x == -1008);
}

static void test_polyline(void)
{
    DcGpuState state;
    uint32_t words[5];

    gpu_decode_reset(&state);
    words[0] = cw(11, 22, 33, 0x48);
    words[1] = xyw(0, 0);
    words[2] = xyw(50, 0);
    words[3] = xyw(50, 50);
    words[4] = 0x55555555u;
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 5, record_sink, NULL) == 5);
    CHECK(s_recorded_count == 2);
    CHECK(s_recorded[0].shape == DC_SHAPE_LINE);
    CHECK(s_recorded[0].v[0].x == 0 && s_recorded[0].v[0].y == 0);
    CHECK(s_recorded[0].v[1].x == 50 && s_recorded[0].v[1].y == 0);
    CHECK(s_recorded[1].v[0].x == 50 && s_recorded[1].v[0].y == 0);
    CHECK(s_recorded[1].v[1].x == 50 && s_recorded[1].v[1].y == 50);
    CHECK(s_recorded[0].v[0].r == 11 && s_recorded[0].v[0].g == 22 && s_recorded[0].v[0].b == 33);
}

static void test_oversized_triangle_is_dropped(void)
{
    DcGpuState state;
    uint32_t words[4];

    gpu_decode_reset(&state);
    words[0] = cw(10, 20, 30, 0x20);
    words[1] = xyw(-600, 0);
    words[2] = xyw(424, 0);
    words[3] = xyw(0, 100);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 4, record_sink, NULL) == 4);
    CHECK(s_recorded_count == 0);
    words[2] = xyw(423, 0);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 4, record_sink, NULL) == 4);
    CHECK(s_recorded_count == 1);
    words[2] = xyw(100, 0);
    words[3] = xyw(0, 512);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 4, record_sink, NULL) == 4);
    CHECK(s_recorded_count == 0);
}

static void test_quad_with_one_oversized_half_keeps_the_other(void)
{
    DcGpuState state;
    uint32_t words[5];

    gpu_decode_reset(&state);
    words[0] = cw(10, 20, 30, 0x28);
    words[1] = xyw(0, 0);
    words[2] = xyw(100, 0);
    words[3] = xyw(0, 100);
    words[4] = xyw(100, 700);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 5, record_sink, NULL) == 5);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_TRI);
    CHECK(s_recorded[0].nverts == 3);
    CHECK(s_recorded[0].v[0].x == 0 && s_recorded[0].v[0].y == 0);
    CHECK(s_recorded[0].v[1].x == 100 && s_recorded[0].v[1].y == 0);
    CHECK(s_recorded[0].v[2].x == 0 && s_recorded[0].v[2].y == 100);
    words[1] = xyw(0, -500);
    words[4] = xyw(100, 400);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 5, record_sink, NULL) == 5);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_TRI);
    CHECK(s_recorded[0].v[0].x == 100 && s_recorded[0].v[0].y == 0);
    CHECK(s_recorded[0].v[1].x == 0 && s_recorded[0].v[1].y == 100);
    CHECK(s_recorded[0].v[2].x == 100 && s_recorded[0].v[2].y == 400);
    words[1] = xyw(0, 0);
    words[4] = xyw(100, 100);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 5, record_sink, NULL) == 5);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_QUAD);
}

static void test_oversized_line_is_dropped(void)
{
    DcGpuState state;
    uint32_t words[3];

    gpu_decode_reset(&state);
    words[0] = cw(11, 22, 33, 0x40);
    words[1] = xyw(-512, 0);
    words[2] = xyw(512, 0);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 3, record_sink, NULL) == 3);
    CHECK(s_recorded_count == 0);
    words[2] = xyw(511, 0);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 3, record_sink, NULL) == 3);
    CHECK(s_recorded_count == 1);
}

static void test_nop_and_unknown(void)
{
    DcGpuState state;
    uint32_t words[6];

    gpu_decode_reset(&state);
    words[0] = 0x00000000u;
    words[1] = 0xD0000000u;
    words[2] = cw(1, 1, 1, 0x20);
    words[3] = xyw(0, 0);
    words[4] = xyw(0, 0);
    words[5] = xyw(0, 0);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 6, record_sink, NULL) == 6);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].v[0].r == 1);
    CHECK(gpu_census_count(0xD0) == 1);
}

static void test_ot_skips_empty_middle(void)
{
    DcGpuState state;
    u_long mem[11];
    uintptr_t base = (uintptr_t)mem;

    mem[1] = cw(1, 1, 1, 0x20);
    mem[2] = xyw(0, 0);
    mem[3] = xyw(0, 0);
    mem[4] = xyw(0, 0);
    mem[7] = cw(2, 2, 2, 0x20);
    mem[8] = xyw(0, 0);
    mem[9] = xyw(0, 0);
    mem[10] = xyw(0, 0);
    setlen(&mem[0], 4);
    setaddr(&mem[0], (uint32_t)(5 * sizeof(u_long)));
    setlen(&mem[5], 0);
    setaddr(&mem[5], (uint32_t)(6 * sizeof(u_long)));
    setlen(&mem[6], 4);
    setaddr(&mem[6], GPU_TAG_END);
    gpu_decode_set_base(base);
    gpu_decode_reset(&state);
    reset_recorder();
    gpu_decode_ot(&state, &mem[0], record_sink, NULL);
    gpu_decode_set_base(0);
    CHECK(s_recorded_count == 2);
    CHECK(s_recorded[0].v[0].r == 1);
    CHECK(s_recorded[1].v[0].r == 2);
}

static void test_census_report_empty(void)
{
    s_last_log[0] = '\0';
    gpu_census_report();
    CHECK(strcmp(s_last_log, "census ") == 0);
}

static void test_env_e2_then_tile(void)
{
    DcGpuState state;
    uint32_t words[4];

    gpu_decode_reset(&state);
    words[0] = 0xE2000000u | 0x00001234u;
    words[1] = cw(9, 8, 7, 0x60);
    words[2] = xyw(5, 5);
    words[3] = xyw(40, 24);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 4, record_sink, NULL) == 4);
    CHECK(state.tex_window == 0x00001234u);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_RECT);
    CHECK(s_recorded[0].v[0].x == 5 && s_recorded[0].v[0].y == 5);
}

static void test_setdrawenv_roundtrip(void)
{
    DRAWENV env;
    DR_ENV pkt;
    DcGpuState state;

    setRECT(&env.clip, 10, 20, 256, 200);
    env.ofs[0] = -3;
    env.ofs[1] = 4;
    setRECT(&env.tw, 0, 0, 0, 0);
    env.tpage = 0x53;
    env.dtd = 1;
    env.dfe = 0;
    SetDrawEnv(&pkt, &env);
    gpu_decode_reset(&state);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, (const uint32_t *)pkt.code, 6, record_sink, NULL) == 6);
    CHECK(s_recorded_count == 0);
    CHECK(state.area_x1 == 10 && state.area_y1 == 20);
    CHECK(state.area_x2 == 265 && state.area_y2 == 219);
    CHECK(state.ofs_x == -3 && state.ofs_y == 4);
    CHECK(state.tpage == 0x253u);
    CHECK(state.tex_window == 0);
}

static void test_fill_rect_writes_mirror_without_a_primitive(void)
{
    DcGpuState state;
    uint32_t words[6];
    uint32_t before;

    gpu_decode_reset(&state);
    before = vram_mirror_generation(100, 40, 4, 2);
    words[0] = cw(0xF8, 0, 0x08, 0x02);
    words[1] = xyw(100, 40);
    words[2] = xyw(4, 2);
    words[3] = cw(9, 8, 7, 0x60);
    words[4] = xyw(5, 5);
    words[5] = xyw(40, 24);
    reset_recorder();
    CHECK(gpu_decode_packet(&state, words, 6, record_sink, NULL) == 6);
    CHECK(s_recorded_count == 1);
    CHECK(s_recorded[0].shape == DC_SHAPE_RECT);
    CHECK(s_recorded[0].v[0].x == 5 && s_recorded[0].v[0].y == 5);
    CHECK(gpu_census_count(0x02) == 1);
    CHECK(*vram_mirror_at(100, 40) == 0x041F);
    CHECK(vram_mirror_generation(100, 40, 4, 2) != before);
}

static void test_fill_rect_rounds_position_and_size_like_hardware(void)
{
    DcGpuState state;
    uint32_t words[3];
    uint32_t before;

    gpu_decode_reset(&state);
    before = vram_mirror_generation(32, 80, 16, 16);
    words[0] = cw(0xF8, 0, 0x08, 0x02);
    words[1] = xyw(0xFC23, 0xFE50);
    words[2] = xyw(0xF007, 0xFE10);
    gpu_decode_packet(&state, words, 3, 0, 0);
    CHECK(*vram_mirror_at(32, 80) == 0x041F);
    CHECK(*vram_mirror_at(47, 95) == 0x041F);
    CHECK(vram_mirror_generation(32, 80, 16, 16) != before);
}

int main(void)
{
    test_census_report_empty();
    test_poly_ft4();
    test_poly_g3();
    test_poly_gt4();
    test_sprt();
    test_sprt16();
    test_tile();
    test_env_e3_e4_e5();
    test_vertex_sign_extend();
    test_polyline();
    test_oversized_triangle_is_dropped();
    test_quad_with_one_oversized_half_keeps_the_other();
    test_oversized_line_is_dropped();
    test_nop_and_unknown();
    test_ot_skips_empty_middle();
    test_env_e2_then_tile();
    test_setdrawenv_roundtrip();
    test_fill_rect_writes_mirror_without_a_primitive();
    test_fill_rect_rounds_position_and_size_like_hardware();

    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
