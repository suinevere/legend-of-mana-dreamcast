/*----------------------
 | overlay_header.h
 | Description: The 32-byte OVLnnC.BIN header both ports read before an overlay's code; overlay_link.py packs it big-endian on every target.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_OVERLAY_HEADER_H
#define LOM_OVERLAY_HEADER_H

/*----------------------
 | OVERLAY_HEADER_MAGIC / OVERLAY_HEADER_SIZE
 | Description: The 'LOMC' magic and byte size of an OVLnnC.BIN header; must match overlay_link.py.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define OVERLAY_HEADER_MAGIC 0x4C4F4D43u
#define OVERLAY_HEADER_SIZE 32

/*----------------------
 | OverlayHeader
 | Description: Decoded OVLnnC.BIN header: code region base and size (0 when static), data file size and the absolute .bss range.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    unsigned magic, version, code_base, code_size, data_size, bss_start, bss_end;
} OverlayHeader;

/*----------------------
 | overlay_header_parse
 | Description: Decodes a big-endian 32-byte OVLnnC.BIN header.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: raw -- the header bytes; out -- decoded fields
 | Returns: 0, or -1 on bad magic or version
 ----------------------*/
int overlay_header_parse(const unsigned char raw[32], OverlayHeader *out);

#endif
