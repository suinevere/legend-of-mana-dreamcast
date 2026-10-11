/*----------------------
 | overlay_header.c
 | Description: Decodes the shared OVLnnC.BIN header, with no platform dependency so both ports and host tests compile it.
 | Author: suinevere
 | Dependencies: overlay_header.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "overlay_header.h"

/*----------------------
 | read_be32
 | Description: Reads a big-endian 32-bit word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- four bytes
 | Returns: the decoded word
 ----------------------*/
static unsigned read_be32(const unsigned char *p)
{
    return ((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) | ((unsigned)p[2] << 8) | (unsigned)p[3];
}

/*----------------------
 | overlay_header_parse
 | Description: Decodes a big-endian 32-byte OVLnnC.BIN header and accepts only the 'LOMC' magic at version 1.
 | Author: suinevere
 | Dependencies: read_be32
 | Globals: N/A
 | Params: raw -- the header bytes; out -- decoded fields
 | Returns: 0, or -1 on bad magic or version
 ----------------------*/
int overlay_header_parse(const unsigned char raw[32], OverlayHeader *out)
{
    out->magic = read_be32(raw);
    out->version = read_be32(raw + 4);
    out->code_base = read_be32(raw + 8);
    out->code_size = read_be32(raw + 12);
    out->data_size = read_be32(raw + 16);
    out->bss_start = read_be32(raw + 20);
    out->bss_end = read_be32(raw + 24);
    return out->magic == OVERLAY_HEADER_MAGIC && out->version == 1 ? 0 : -1;
}
