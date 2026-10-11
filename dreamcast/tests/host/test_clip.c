/*----------------------
 | test_clip.c
 | Description: Host tests for CPU draw-area clipping of decoded PS1 primitives: rect trimming, quad/triangle Sutherland-Hodgman clipping, line thickening and triangle-strip ordering.
 | Author: suinevere
 | Dependencies: clip.h, gpu_decode.h, clip.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "clip.h"
#include "gpu_decode.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static float fabsf_local(float x)
{
    return x < 0.0f ? -x : x;
}

static void make_area(DcGpuState *state, int x1, int y1, int x2, int y2)
{
    memset(state, 0, sizeof(*state));
    state->area_x1 = (int16_t)x1;
    state->area_y1 = (int16_t)y1;
    state->area_x2 = (int16_t)x2;
    state->area_y2 = (int16_t)y2;
}

static void test_rect_trimmed_right(void)
{
    DcGpuState state;
    DcPrim prim;
    DcPoly poly;
    int rc;

    make_area(&state, 0, 0, 319, 239);
    memset(&prim, 0, sizeof(prim));
    prim.shape = DC_SHAPE_RECT;
    prim.nverts = 2;
    prim.v[0].x = 300;
    prim.v[0].y = 10;
    prim.v[0].u = 0;
    prim.v[0].v = 0;
    prim.v[0].r = 10;
    prim.v[0].g = 20;
    prim.v[0].b = 30;
    prim.v[1].x = 332;
    prim.v[1].y = 42;
    prim.v[1].u = 99;
    prim.v[1].v = 99;

    rc = clip_prim(&prim, &state, &poly);
    CHECK(rc != 0);
    CHECK(poly.count == 4);
    CHECK(fabsf_local(poly.v[0].x - 300.0f) < 0.01f);
    CHECK(fabsf_local(poly.v[1].x - 320.0f) < 0.01f);
    CHECK(fabsf_local(poly.v[0].u - 0.0f) < 0.01f);
    CHECK(fabsf_local(poly.v[1].u - 20.0f) < 0.01f);
    CHECK(poly.v[0].r == 10 && poly.v[0].g == 20 && poly.v[0].b == 30);
}

static void test_rect_fully_outside(void)
{
    DcGpuState state;
    DcPrim prim;
    DcPoly poly;
    int rc;

    make_area(&state, 0, 0, 319, 239);
    memset(&prim, 0, sizeof(prim));
    prim.shape = DC_SHAPE_RECT;
    prim.nverts = 2;
    prim.v[0].x = 400;
    prim.v[0].y = 10;
    prim.v[1].x = 432;
    prim.v[1].y = 42;

    rc = clip_prim(&prim, &state, &poly);
    CHECK(rc == 0);
    CHECK(poly.count == 0);
}

static void test_quad_inside_second_area(void)
{
    DcGpuState state;
    DcPrim prim;
    DcPoly poly;
    int rc;

    make_area(&state, 0, 256, 319, 495);
    memset(&prim, 0, sizeof(prim));
    prim.shape = DC_SHAPE_QUAD;
    prim.nverts = 4;
    prim.v[0].x = 50;
    prim.v[0].y = 266;
    prim.v[1].x = 150;
    prim.v[1].y = 266;
    prim.v[2].x = 50;
    prim.v[2].y = 350;
    prim.v[3].x = 150;
    prim.v[3].y = 350;

    rc = clip_prim(&prim, &state, &poly);
    CHECK(rc != 0);
    CHECK(poly.count == 4);
    CHECK(fabsf_local(poly.v[0].x - 50.0f) < 0.01f && fabsf_local(poly.v[0].y - 10.0f) < 0.01f);
    CHECK(fabsf_local(poly.v[1].x - 150.0f) < 0.01f && fabsf_local(poly.v[1].y - 10.0f) < 0.01f);
    CHECK(fabsf_local(poly.v[2].x - 150.0f) < 0.01f && fabsf_local(poly.v[2].y - 94.0f) < 0.01f);
    CHECK(fabsf_local(poly.v[3].x - 50.0f) < 0.01f && fabsf_local(poly.v[3].y - 94.0f) < 0.01f);
}

static void test_triangle_crosses_left_edge(void)
{
    DcGpuState state;
    DcPrim prim;
    DcPoly poly;
    int rc;
    unsigned i;
    int found;
    float min_x;

    make_area(&state, 0, 0, 319, 239);
    memset(&prim, 0, sizeof(prim));
    prim.shape = DC_SHAPE_TRI;
    prim.nverts = 3;
    prim.v[0].x = -10;
    prim.v[0].y = 50;
    prim.v[0].u = 0;
    prim.v[1].x = 10;
    prim.v[1].y = 50;
    prim.v[1].u = 100;
    prim.v[2].x = 50;
    prim.v[2].y = 10;
    prim.v[2].u = 200;

    rc = clip_prim(&prim, &state, &poly);
    CHECK(rc != 0);
    CHECK(poly.count >= 3);

    min_x = poly.v[0].x;
    found = 0;
    for (i = 0; i < poly.count; i++)
    {
        if (poly.v[i].x < min_x)
        {
            min_x = poly.v[i].x;
        }
        if (fabsf_local(poly.v[i].x - 0.0f) < 0.01f && fabsf_local(poly.v[i].u - 50.0f) < 0.01f)
        {
            found = 1;
        }
    }
    CHECK(fabsf_local(min_x - 0.0f) < 0.01f);
    CHECK(found);
}

static void test_quad_far_vertex_clipped_not_dropped(void)
{
    DcGpuState state;
    DcPrim prim;
    DcPoly poly;
    int rc;

    make_area(&state, 0, 0, 319, 239);
    memset(&prim, 0, sizeof(prim));
    prim.shape = DC_SHAPE_QUAD;
    prim.nverts = 4;
    prim.v[0].x = -1008;
    prim.v[0].y = 50;
    prim.v[1].x = 50;
    prim.v[1].y = 50;
    prim.v[2].x = 5;
    prim.v[2].y = 100;
    prim.v[3].x = 50;
    prim.v[3].y = 100;

    rc = clip_prim(&prim, &state, &poly);
    CHECK(rc != 0);
    CHECK(poly.count >= 3);
    CHECK(poly.count <= DC_CLIP_MAX_VERTS);
}

static void test_line_becomes_one_pixel_tall_quad(void)
{
    DcGpuState state;
    DcPrim prim;
    DcPoly poly;
    int rc;
    unsigned i;
    float min_y;
    float max_y;

    make_area(&state, 0, 0, 319, 239);
    memset(&prim, 0, sizeof(prim));
    prim.shape = DC_SHAPE_LINE;
    prim.nverts = 2;
    prim.v[0].x = 10;
    prim.v[0].y = 10;
    prim.v[1].x = 20;
    prim.v[1].y = 10;

    rc = clip_prim(&prim, &state, &poly);
    CHECK(rc != 0);
    CHECK(poly.count >= 3);

    min_y = poly.v[0].y;
    max_y = poly.v[0].y;
    for (i = 1; i < poly.count; i++)
    {
        if (poly.v[i].y < min_y)
        {
            min_y = poly.v[i].y;
        }
        if (poly.v[i].y > max_y)
        {
            max_y = poly.v[i].y;
        }
    }
    CHECK(fabsf_local((max_y - min_y) - 1.0f) < 0.01f);
}

static void test_strip_order_five(void)
{
    uint8_t order[8];
    unsigned n;

    n = clip_strip_order(5, order);
    CHECK(n == 5);
    CHECK(order[0] == 0 && order[1] == 1 && order[2] == 4 && order[3] == 2 && order[4] == 3);
}

int main(void)
{
    test_rect_trimmed_right();
    test_rect_fully_outside();
    test_quad_inside_second_area();
    test_triangle_crosses_left_edge();
    test_quad_far_vertex_clipped_not_dropped();
    test_line_becomes_one_pixel_tall_quad();
    test_strip_order_five();

    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
