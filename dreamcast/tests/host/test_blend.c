/*----------------------
 | test_blend.c
 | Description: Host tests for the PS1-to-PowerVR blend pass table, vertex-colour scaling and the brightening excess pass.
 | Author: suinevere
 | Dependencies: blend.h, palette_bank.h, blend.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "blend.h"
#include "palette_bank.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static void check_pass(const BlendPass *p, uint8_t src, uint8_t dst, uint8_t alpha, uint8_t variant)
{
    CHECK(p->src == src);
    CHECK(p->dst == dst);
    CHECK(p->alpha == alpha);
    CHECK(p->variant == variant);
    CHECK(p->cover == 0);
}

static void check_cover(const BlendPass *p, uint8_t variant)
{
    CHECK(p->src == BL_INVDSTCOLOR);
    CHECK(p->dst == BL_INVSRCALPHA);
    CHECK(p->alpha == 255);
    CHECK(p->variant == variant);
    CHECK(p->cover == 1);
}

static void test_not_semi(void)
{
    BlendPass out[BLEND_MAX_PASSES];
    unsigned n;

    n = blend_plan(0, 0, 0, STP_NONE, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_OPAQUE);

    n = blend_plan(1, 0, 3, STP_MIXED, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_OPAQUE);
}

static void test_untextured_semi(void)
{
    BlendPass out[BLEND_MAX_PASSES];
    unsigned n;

    n = blend_plan(0, 1, 0, STP_NONE, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_SRCALPHA, BL_INVSRCALPHA, 128, PAL_OPAQUE);

    n = blend_plan(0, 1, 1, STP_NONE, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_ONE, BL_ONE, 255, PAL_OPAQUE);

    n = blend_plan(0, 1, 2, STP_NONE, 1, out);
    CHECK(n == 3);
    check_cover(&out[0], PAL_OPAQUE);
    check_pass(&out[1], BL_ONE, BL_ONE, 255, PAL_OPAQUE);
    check_cover(&out[2], PAL_OPAQUE);

    n = blend_plan(0, 1, 3, STP_NONE, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_SRCALPHA, BL_ONE, 64, PAL_OPAQUE);
}

static void test_shadow_multiply(void)
{
    BlendPass out[BLEND_MAX_PASSES];

    CHECK(blend_shadow_plan(out) == 1);
    check_pass(&out[0], BL_ZERO, BL_INVSRCCOLOR, 255, PAL_OPAQUE);
    CHECK(out[0].cover == 0);
}

static void test_textured_semi_stp_none(void)
{
    BlendPass out[BLEND_MAX_PASSES];
    unsigned n;

    n = blend_plan(1, 1, 0, STP_NONE, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_OPAQUE);

    n = blend_plan(1, 1, 2, STP_NONE, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_OPAQUE);
}

static void test_textured_semi_abr0(void)
{
    BlendPass out[BLEND_MAX_PASSES];
    unsigned n;

    n = blend_plan(1, 1, 0, STP_ALL, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_HALF);

    n = blend_plan(1, 1, 0, STP_MIXED, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_HALF);
}

static void test_textured_semi_stp_all(void)
{
    BlendPass out[BLEND_MAX_PASSES];
    unsigned n;

    n = blend_plan(1, 1, 1, STP_ALL, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_SRCALPHA, BL_ONE, 255, PAL_STP_ONLY);

    n = blend_plan(1, 1, 2, STP_ALL, 1, out);
    CHECK(n == 3);
    check_cover(&out[0], PAL_COVER);
    check_pass(&out[1], BL_SRCALPHA, BL_ONE, 255, PAL_STP_ONLY);
    check_cover(&out[2], PAL_COVER);

    n = blend_plan(1, 1, 2, STP_ALL, 0, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_ZERO, BL_INVSRCCOLOR, 255, PAL_STP_ONLY);

    n = blend_plan(1, 1, 3, STP_ALL, 1, out);
    CHECK(n == 1);
    check_pass(&out[0], BL_SRCALPHA, BL_ONE, 64, PAL_STP_ONLY);
}

static void test_textured_semi_stp_mixed(void)
{
    BlendPass out[BLEND_MAX_PASSES];
    unsigned n;

    n = blend_plan(1, 1, 1, STP_MIXED, 1, out);
    CHECK(n == 2);
    check_pass(&out[0], BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_NONSTP_ONLY);
    check_pass(&out[1], BL_SRCALPHA, BL_ONE, 255, PAL_STP_ONLY);

    n = blend_plan(1, 1, 2, STP_MIXED, 1, out);
    CHECK(n == 4);
    check_pass(&out[0], BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_NONSTP_ONLY);
    check_cover(&out[1], PAL_COVER);
    check_pass(&out[2], BL_SRCALPHA, BL_ONE, 255, PAL_STP_ONLY);
    check_cover(&out[3], PAL_COVER);

    n = blend_plan(1, 1, 3, STP_MIXED, 1, out);
    CHECK(n == 2);
    check_pass(&out[0], BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_NONSTP_ONLY);
    check_pass(&out[1], BL_SRCALPHA, BL_ONE, 64, PAL_STP_ONLY);
}

static void test_vertex_colour(void)
{
    CHECK(blend_vertex_colour(128, 1, 0) == 255);
    CHECK(blend_vertex_colour(64, 1, 0) == 128);
    CHECK(blend_vertex_colour(200, 1, 0) == 255);
    CHECK(blend_vertex_colour(10, 1, 1) == 255);
    CHECK(blend_vertex_colour(10, 0, 0) == 10);
}

static void test_excess_colour(void)
{
    CHECK(blend_excess_colour(255, 1, 0) == 254);
    CHECK(blend_excess_colour(192, 1, 0) == 128);
    CHECK(blend_excess_colour(128, 1, 0) == 0);
    CHECK(blend_excess_colour(64, 1, 0) == 0);
    CHECK(blend_excess_colour(255, 1, 1) == 0);
    CHECK(blend_excess_colour(255, 0, 0) == 0);
}

static void test_excess_pass(void)
{
    BlendPass out[BLEND_MAX_PASSES];
    BlendPass excess;
    unsigned n;

    n = blend_plan(1, 0, 0, STP_NONE, 1, out);
    CHECK(n == 1);
    CHECK(blend_excess_pass(&out[0], &excess) == 1);
    check_pass(&excess, BL_SRCALPHA, BL_ONE, 255, PAL_OPAQUE);

    n = blend_plan(1, 1, 2, STP_ALL, 1, out);
    CHECK(n == 3);
    CHECK(blend_excess_pass(&out[0], &excess) == 0);
    CHECK(blend_excess_pass(&out[1], &excess) == 1);
    check_pass(&excess, BL_SRCALPHA, BL_ONE, 255, PAL_STP_ONLY);
    CHECK(blend_excess_pass(&out[2], &excess) == 0);

    n = blend_plan(1, 1, 3, STP_ALL, 1, out);
    CHECK(blend_excess_pass(&out[0], &excess) == 1);
    check_pass(&excess, BL_SRCALPHA, BL_ONE, 64, PAL_STP_ONLY);
}

int main(void)
{
    test_not_semi();
    test_untextured_semi();
    test_shadow_multiply();
    test_textured_semi_stp_none();
    test_textured_semi_abr0();
    test_textured_semi_stp_all();
    test_textured_semi_stp_mixed();
    test_vertex_colour();
    test_excess_colour();
    test_excess_pass();

    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
