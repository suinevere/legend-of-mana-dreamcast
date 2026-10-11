/*----------------------
 | test_dc_gpu_tag.c
 | Description: Host test that Dreamcast packet tags built with the prefix's setaddr round-trip through gpu_tag_addr and gpu_tag_pointer across all 16 MB of RAM.
 | Author: suinevere
 | Dependencies: lom_prefix.h (forced include), gpu_tag.h, lom_platform_memory.h, main_globals_dc.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>

#include "gpu_tag.h"

static int failures;

#define CHECK(cond)                                              \
    do                                                           \
    {                                                            \
        if (!(cond))                                             \
        {                                                        \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                          \
        }                                                        \
    } while (0)

static uint32_t round_trip(uint32_t packet)
{
    u_long tag[1] = {0};
    setlen(tag, 5);
    setaddr(tag, packet);
    CHECK(gpu_tag_len((uint32_t)tag[0]) == 5);
    return gpu_tag_pointer(gpu_tag_addr((uint32_t)tag[0]));
}

int main(void)
{
    u_long end_tag[1] = {0};

    CHECK(GPU_TAG_RAM_BASE == 0x8C000000u);
    CHECK(&lom_primary_upper[0] == &lom_render_arena[0]);
    CHECK(round_trip(0x8CF12340u) == 0x8CF12340u);
    CHECK(round_trip(0x8C010000u) == 0x8C010000u);
    CHECK(round_trip(0x8C800000u) == 0x8C800000u);

    setaddr(end_tag, GPU_TAG_END);
    CHECK(gpu_tag_addr((uint32_t)end_tag[0]) == GPU_TAG_END);
    CHECK(getaddr(end_tag) == GPU_TAG_END);

    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
