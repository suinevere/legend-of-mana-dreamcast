/*----------------------
 | test_map_relocate.c
 | Description: Host checks for map_relocate: pointers before the texels are rebased, everything else and every non-map buffer is left alone.
 | Author: suinevere
 | Dependencies: src/cd/map_relocate.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "map_relocate.h"

#define BASE MAP_RELOCATE_PS1_BASE

static uint8_t s_map[0x1000] __attribute__((aligned(32)));

static uint32_t rd(uint32_t offset)
{
    uint32_t v;
    memcpy(&v, s_map + offset, 4);
    return v;
}

static void wr(uint32_t offset, uint32_t v)
{
    memcpy(s_map + offset, &v, 4);
}

static void build(void)
{
    memset(s_map, 0, sizeof(s_map));
    memcpy(s_map, "SKmapDat", 8);
    wr(0x08, 0x13);
    wr(0x0C, 0x40);
    wr(0x14, BASE + 0x100);
    wr(0x20, BASE + 0x40);
    wr(0x24, 0);
    wr(0x28, 0x12345678u);
    wr(0x2C, BASE + 0x100 + 0x40 + MAP_RELOCATE_TRAILER_SLACK);
    wr(0x30, BASE + 0x100 + 0x40 + MAP_RELOCATE_TRAILER_SLACK - 4);
    wr(0x100, BASE + 0x40);
}

int main(void)
{
    uint32_t real = (uint32_t)(uintptr_t)s_map;
    int n;

    build();
    n = map_relocate(s_map, sizeof(s_map));
    if (n != 3) { printf("FAIL count %d\n", n); return 1; }
    if (rd(0x14) != real + 0x100) { printf("FAIL texel pointer\n"); return 1; }
    if (rd(0x20) != real + 0x40) { printf("FAIL struct pointer\n"); return 1; }
    if (rd(0x24) != 0 || rd(0x28) != 0x12345678u) { printf("FAIL non-pointer changed\n"); return 1; }
    if (rd(0x2C) != BASE + 0x100 + 0x40 + MAP_RELOCATE_TRAILER_SLACK) { printf("FAIL past span\n"); return 1; }
    if (rd(0x30) != real + 0x100 + 0x40 + MAP_RELOCATE_TRAILER_SLACK - 4) { printf("FAIL last in span\n"); return 1; }
    if (rd(0x100) != BASE + 0x40) { printf("FAIL texel bytes changed\n"); return 1; }

    build();
    s_map[0] = 'X';
    if (map_relocate(s_map, sizeof(s_map)) != -1 || rd(0x20) != BASE + 0x40) { printf("FAIL non-map\n"); return 1; }

    build();
    wr(0x14, BASE + sizeof(s_map));
    if (map_relocate(s_map, sizeof(s_map)) != 0 || rd(0x20) != BASE + 0x40) { printf("FAIL bad texel pointer\n"); return 1; }

    if (map_relocate(s_map, 8) != -1) { printf("FAIL tiny buffer\n"); return 1; }

    puts("OK");
    return 0;
}
