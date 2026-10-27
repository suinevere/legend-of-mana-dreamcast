/*----------------------
 | overlay_region.h
 | Description: Pure Dreamcast overlay region lookup, fit check and file naming, kept apart from the KallistiOS loader so host tests can compile them.
 | Author: suinevere
 | Dependencies: overlay_header.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_OVERLAY_REGION_H
#define LOM_OVERLAY_REGION_H

#include "overlay_header.h"

/*----------------------
 | OverlayRegion
 | Description: One overlay's fixed arena placement: PS1 resource index, name, and where its code and its data blob load with the bytes each may take.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    int resource;
    const char *name;
    unsigned code_base;
    unsigned code_size;
    unsigned data_base;
    unsigned data_size;
} OverlayRegion;

/*----------------------
 | overlay_region_find
 | Description: Finds the region of a resource index in a table.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: table, count -- region table; resource -- PS1 CD resource index
 | Returns: the region, or 0 when the table has none
 ----------------------*/
const OverlayRegion *overlay_region_find(const OverlayRegion *table, unsigned count, int resource);

/*----------------------
 | overlay_region_check
 | Description: Rejects a header whose code or data does not fit its region or whose .bss leaves the data region.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: region -- arena placement; header -- decoded OVLnnC.BIN header; data_file_size -- size of OVLnn.BIN
 | Returns: 0 when the overlay fits, otherwise the overlay name
 ----------------------*/
const char *overlay_region_check(const OverlayRegion *region, const OverlayHeader *header, unsigned data_file_size);

/*----------------------
 | overlay_file_name
 | Description: Builds an overlay's file name, "OVLnnC.BIN" for its code or "OVLnn.BIN" for its data.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: resource -- PS1 CD resource index; code -- nonzero for the code file; out -- receives the name
 | Returns: N/A
 ----------------------*/
void overlay_file_name(int resource, int code, char out[16]);

#endif
