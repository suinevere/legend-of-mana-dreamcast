/*----------------------
 | resource_index.c
 | Description: Parses the big-endian RESINDEX.BIN table and names the Saturn file that holds each resource.
 | Author: suinevere
 | Dependencies: resource_index.h
 | Globals: s_entries, s_count
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "resource_index.h"

/*----------------------
 | RESOURCE_INDEX_MAX
 | Description: Most resources the index can hold; mirrors cdrom_saturn.c's CDROM_INDEX_BLOB_RESOURCE_MAX and convert_disc.py's RESOURCE_INDEX_MAX, which rejects a larger RESINDEX.BIN.
 |              The Dreamcast makefile overrides it with -D, since its disc carries every live PS1 resource; Saturn keeps this default.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef RESOURCE_INDEX_MAX
#define RESOURCE_INDEX_MAX 256
#endif

static ResourceEntry s_entries[RESOURCE_INDEX_MAX];
static int s_count;

/*----------------------
 | read_be
 | Description: Reads a big-endian unsigned integer of n bytes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- bytes, n -- width
 | Returns: value
 ----------------------*/
static uint32_t read_be(const uint8_t *p, int n)
{
    uint32_t value = 0;
    int i;

    for (i = 0; i < n; i++)
    {
        value = (value << 8) | p[i];
    }
    return value;
}

/*----------------------
 | resource_index_load
 | Description: Loads the index from RESINDEX.BIN bytes.
 | Author: suinevere
 | Dependencies: read_be
 | Globals: s_entries, s_count
 | Params: blob -- file bytes, len -- byte count
 | Returns: entry count, or -1 if the blob is malformed
 ----------------------*/
int resource_index_load(const uint8_t *blob, int len)
{
    uint32_t count;
    uint32_t i;

    s_count = 0;
    if (len < 4)
    {
        return -1;
    }
    count = read_be(blob, 4);
    if (count > RESOURCE_INDEX_MAX || (int)(4 + count * 8) > len)
    {
        return -1;
    }
    for (i = 0; i < count; i++)
    {
        const uint8_t *e = blob + 4 + i * 8;
        s_entries[i].index = (uint16_t)read_be(e, 2);
        s_entries[i].kind = e[2];
        s_entries[i].size = read_be(e + 4, 4);
    }
    s_count = (int)count;
    return s_count;
}

/*----------------------
 | resource_index_find
 | Description: Finds an exported resource by its PS1 resource index.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_entries, s_count
 | Params: index -- PS1 resource index
 | Returns: entry, or null when the Saturn disc does not carry it
 ----------------------*/
const ResourceEntry *resource_index_find(int index)
{
    int i;

    for (i = 0; i < s_count; i++)
    {
        if (s_entries[i].index == index)
        {
            return &s_entries[i];
        }
    }
    return 0;
}

/*----------------------
 | resource_folder
 | Description: Picks the disc folder that holds a resource's files: OVL for overlays, RES otherwise.
 | Author: suinevere
 | Dependencies: cd_dir.h
 | Globals: N/A
 | Params: kind -- RESOURCE_KIND_*
 | Returns: CD_FOLDER_OVL or CD_FOLDER_RES
 ----------------------*/
int resource_folder(int kind)
{
    return kind == RESOURCE_KIND_OVERLAY ? CD_FOLDER_OVL : CD_FOLDER_RES;
}

/*----------------------
 | resource_file_name
 | Description: Builds the 8.3 Saturn file name for a resource: OVLnn.BIN for overlays, RESnnnn.BIN otherwise.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: index -- PS1 resource index, kind -- RESOURCE_KIND_*, out -- 16-byte buffer
 | Returns: N/A
 ----------------------*/
void resource_file_name(int index, int kind, char out[16])
{
    const char *prefix = kind == RESOURCE_KIND_OVERLAY ? "OVL" : "RES";
    int digits = kind == RESOURCE_KIND_OVERLAY ? 2 : 4;
    int pos = 0;
    int d;

    while (*prefix)
    {
        out[pos++] = *prefix++;
    }
    for (d = digits - 1; d >= 0; d--)
    {
        int div = 1;
        int k;
        for (k = 0; k < d; k++)
        {
            div *= 10;
        }
        out[pos++] = (char)('0' + (index / div) % 10);
    }
    out[pos++] = '.';
    out[pos++] = 'B';
    out[pos++] = 'I';
    out[pos++] = 'N';
    out[pos] = 0;
}
