/*----------------------
 | overlay_loader_dc.c
 | Description: Dreamcast overlay loading: reads OVLnnC.BIN and OVLnn.BIN from /cd/OVL into the overlay's fixed arena regions, zeroes its .bss and flushes the caches over its code.
 | Author: suinevere
 | Dependencies: overlay_loader_dc.h, overlay_region.h, overlay_header.h, gen/overlay_regions.h, dc_system.h, arch/cache.h
 | Globals: s_regions
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arch/cache.h>

#include "overlay_loader_dc.h"
#include "overlay_region.h"
#include "overlay_header.h"
#include "overlay_regions.h"
#include "dc_system.h"

/*----------------------
 | OVERLAY_DIR
 | Description: Disc folder holding every OVLnn.BIN and OVLnnC.BIN, as mkdcdisc lays out cd/data.
 | Author: suinevere
 | Dependencies: makefile (lom.cdi)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define OVERLAY_DIR "/cd/OVL/"

/*----------------------
 | s_regions
 | Description: Every linked overlay's arena regions, generated from port/tools/targets.py by overlay_link.py --target dreamcast.
 | Author: suinevere
 | Dependencies: gen/overlay_regions.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const OverlayRegion s_regions[] = { OVERLAY_REGION_TABLE };

/*----------------------
 | overlay_halt
 | Description: Halts with a message naming the overlay and what went wrong.
 | Author: suinevere
 | Dependencies: dc_halt
 | Globals: N/A
 | Params: name -- overlay name; problem -- what failed
 | Returns: never
 ----------------------*/
static void overlay_halt(const char *name, const char *problem) __attribute__((noreturn));
static void overlay_halt(const char *name, const char *problem)
{
    static char message[64];

    snprintf(message, sizeof(message), "overlay %s %s", name, problem);
    dc_halt(message);
}

/*----------------------
 | overlay_open
 | Description: Opens an overlay's code or data file under OVERLAY_DIR for reading.
 | Author: suinevere
 | Dependencies: overlay_file_name
 | Globals: N/A
 | Params: resource -- PS1 CD resource index; code -- nonzero for OVLnnC.BIN
 | Returns: the open file, or 0
 ----------------------*/
static FILE *overlay_open(int resource, int code)
{
    char name[16];
    char path[32];

    overlay_file_name(resource, code, name);
    snprintf(path, sizeof(path), OVERLAY_DIR "%s", name);
    return fopen(path, "rb");
}

/*----------------------
 | overlay_loader_init
 | Description: Halts unless dreamcast.ld placed the overlay arena at the base the generated regions assume.
 | Author: suinevere
 | Dependencies: dc_halt
 | Globals: lom_overlay_arena
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void overlay_loader_init(void)
{
    if ((uintptr_t)lom_overlay_arena != OVERLAY_ARENA_BASE)
    {
        dc_halt("overlay arena misplaced");
    }
}

/*----------------------
 | load_overlay
 | Description: Reads the OVLnnC.BIN header, checks code and data against the overlay's regions, reads code to code_base and data to data_base, zeroes .bss, then flushes the d-cache and invalidates the i-cache over the code.
 | Author: suinevere
 | Dependencies: overlay_region_find, overlay_header_parse, overlay_region_check, overlay_open, overlay_halt, dcache_flush_range, icache_flush_range, dc_halt
 | Globals: s_regions
 | Params: resource_index -- PS1 CD resource index of the overlay
 | Returns: N/A
 ----------------------*/
void load_overlay(int resource_index)
{
    const OverlayRegion *region = overlay_region_find(s_regions, sizeof(s_regions) / sizeof(s_regions[0]), resource_index);
    unsigned char raw[OVERLAY_HEADER_SIZE];
    OverlayHeader header;
    FILE *code;
    FILE *data;
    long data_size;

    if (region == 0)
    {
        dc_halt("overlay not linked for the Dreamcast");
    }
    code = overlay_open(resource_index, 1);
    if (code == 0 || fread(raw, 1, sizeof(raw), code) != sizeof(raw) || overlay_header_parse(raw, &header) != 0)
    {
        overlay_halt(region->name, "code file unreadable");
    }
    data = overlay_open(resource_index, 0);
    if (data == 0 || fseek(data, 0, SEEK_END) != 0 || (data_size = ftell(data)) < 0 || fseek(data, 0, SEEK_SET) != 0)
    {
        overlay_halt(region->name, "data file unreadable");
    }
    if (overlay_region_check(region, &header, (unsigned)data_size) != 0)
    {
        overlay_halt(region->name, "does not fit its arena region");
    }
    if (fread((void *)(uintptr_t)region->code_base, 1, header.code_size, code) != header.code_size
        || fread((void *)(uintptr_t)region->data_base, 1, (size_t)data_size, data) != (size_t)data_size)
    {
        overlay_halt(region->name, "read short");
    }
    fclose(code);
    fclose(data);
    memset((void *)(uintptr_t)header.bss_start, 0, header.bss_end - header.bss_start);
    dcache_flush_range(region->code_base, header.code_size);
    icache_flush_range(region->code_base, header.code_size);
}
