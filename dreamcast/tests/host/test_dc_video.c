/*----------------------
 | test_dc_video.c
 | Description: Host tests for the Dreamcast VRAM mirror and ordering-table walker: load/store round trips, PS1 x-coordinate wraparound, MoveImage copies, ClearImage colour packing, non-empty packet counting and self-loop halting.
 | Author: suinevere
 | Dependencies: vram_mirror.h, ot_walk.h, gpu_tag.h, libgpu_dc.c (LoadImage, StoreImage, MoveImage, ClearImage), lom_prefix.h (forced include)
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

static int failures;
static int s_halts;
static char s_last_halt[64];

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

int main(void)
{
    test_load_store_round_trip();
    test_load_store_wraps_x();
    test_move_image_copies();
    test_clear_image_colour();
    test_ot_counts_nonempty_packets();
    test_ot_follows_host_mapped_packet();
    test_ot_self_loop_halts();

    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
