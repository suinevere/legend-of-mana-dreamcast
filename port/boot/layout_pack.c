/*----------------------
 | layout_pack.c
 | Description: Table of sector-strided resource packs on the Saturn disc (WMAP's layouts and per-cell layout images) mapping PS1 resource ids onto pack cells.
 | Author: suinevere
 | Dependencies: layout_pack.h
 | Globals: s_packs
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "layout_pack.h"

/*----------------------
 | s_packs
 | Description: The packs: layouts, one sector each, read by cdrom_queue_read; layout images, 37 sectors each, read by cdrom_stream.
 | Author: suinevere
 | Dependencies: LAYOUT_PACK_FIRST, LAYOUT_IMAGE_PACK_FIRST
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const ResourcePack s_packs[] = {
    { LAYOUT_PACK_FIRST, LAYOUT_PACK_COUNT, 1, LAYOUT_PACK_LAYOUT_BYTES, RESOURCE_PACK_QUEUE_READ },
    { LAYOUT_IMAGE_PACK_FIRST, LAYOUT_PACK_COUNT, LAYOUT_IMAGE_PACK_STRIDE, LAYOUT_IMAGE_PACK_BYTES, RESOURCE_PACK_STREAM },
};

/*----------------------
 | resource_pack_find
 | Description: Returns the pack whose id range holds a resource id and stores its cell, or 0 for an id in no pack.
 | Author: suinevere
 | Dependencies: s_packs
 | Globals: s_packs
 | Params: resource_index -- PS1 resource id, cell -- receives the cell
 | Returns: pack or 0
 ----------------------*/
const ResourcePack *resource_pack_find(int resource_index, int *cell)
{
    unsigned i;

    for (i = 0; i < sizeof(s_packs) / sizeof(s_packs[0]); i++)
    {
        int offset = resource_index - s_packs[i].first;

        if (offset >= 0 && offset < s_packs[i].count)
        {
            *cell = offset;
            return &s_packs[i];
        }
    }
    return 0;
}
