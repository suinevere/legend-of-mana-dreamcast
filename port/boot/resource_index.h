/*----------------------
 | resource_index.h
 | Description: Lookup over RESINDEX.BIN, which lists the PS1 CD resources a disc carries and their kind and size.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_RESOURCE_INDEX_H
#define LOM_RESOURCE_INDEX_H

#include <stdint.h>

/*----------------------
 | CD_FOLDER_*
 | Description: Disc folders: the root (no folder), OVL for overlay files and RES for every other resource.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CD_FOLDER_ROOT (-1)
#define CD_FOLDER_OVL 0
#define CD_FOLDER_RES 1

/*----------------------
 | RESOURCE_KIND_*
 | Description: Resource kinds written by tools/convert_disc.py: converted overlay data, decompressed stream data, raw bytes, deliberately skipped.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RESOURCE_KIND_OVERLAY 1
#define RESOURCE_KIND_STREAM 2
#define RESOURCE_KIND_RAW 3
#define RESOURCE_KIND_SKIP 4

/*----------------------
 | ResourceEntry
 | Description: One exported resource: PS1 resource index, kind and byte size of its disc file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    uint16_t index;
    uint8_t kind;
    uint32_t size;
} ResourceEntry;

int resource_index_load(const uint8_t *blob, int len);
const ResourceEntry *resource_index_find(int index);
int resource_folder(int kind);
void resource_file_name(int index, int kind, char out[16]);

#endif
