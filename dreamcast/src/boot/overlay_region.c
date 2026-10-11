/*----------------------
 | overlay_region.c
 | Description: Pure Dreamcast overlay region lookup, fit check and file naming.
 | Author: suinevere
 | Dependencies: overlay_region.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "overlay_region.h"

/*----------------------
 | overlay_region_find
 | Description: Finds the region of a resource index in a table.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: table, count -- region table; resource -- PS1 CD resource index
 | Returns: the region, or 0 when the table has none
 ----------------------*/
const OverlayRegion *overlay_region_find(const OverlayRegion *table, unsigned count, int resource)
{
    unsigned i;

    for (i = 0; i < count; i++)
    {
        if (table[i].resource == resource)
        {
            return &table[i];
        }
    }
    return 0;
}

/*----------------------
 | overlay_region_check
 | Description: Rejects a header not linked at the region's code base, code or data larger than its region, a data file whose size disagrees, or .bss outside the data region.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: region -- arena placement; header -- decoded OVLnnC.BIN header; data_file_size -- size of OVLnn.BIN
 | Returns: 0 when the overlay fits, otherwise the overlay name
 ----------------------*/
const char *overlay_region_check(const OverlayRegion *region, const OverlayHeader *header, unsigned data_file_size)
{
    unsigned data_end = region->data_base + region->data_size;

    if (header->code_base != region->code_base || header->code_size > region->code_size)
    {
        return region->name;
    }
    if (data_file_size != header->data_size || data_file_size > region->data_size)
    {
        return region->name;
    }
    if (header->bss_start < region->data_base + data_file_size || header->bss_start > header->bss_end
        || header->bss_end > data_end)
    {
        return region->name;
    }
    return 0;
}

/*----------------------
 | overlay_file_name
 | Description: Builds "OVLnnC.BIN" or "OVLnn.BIN" from the resource index's last two decimal digits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: resource -- PS1 CD resource index; code -- nonzero for the code file; out -- receives the name
 | Returns: N/A
 ----------------------*/
void overlay_file_name(int resource, int code, char out[16])
{
    const char *suffix = code ? "C.BIN" : ".BIN";
    int pos = 3;

    out[0] = 'O';
    out[1] = 'V';
    out[2] = 'L';
    out[pos++] = (char)('0' + (resource / 10) % 10);
    out[pos++] = (char)('0' + resource % 10);
    while (*suffix)
    {
        out[pos++] = *suffix++;
    }
    out[pos] = 0;
}
