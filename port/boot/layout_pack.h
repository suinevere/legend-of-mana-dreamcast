/*----------------------
 | layout_pack.h
 | Description: Table of sector-strided resource packs on the Saturn disc (WMAP's layouts and per-cell layout images) mapping PS1 resource ids onto pack cells.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_LAYOUT_PACK_H
#define LOM_LAYOUT_PACK_H

/*----------------------
 | LAYOUT_PACK_FIRST
 | Description: Resource id of layout 0, which also names the packed layout file on the disc.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LAYOUT_PACK_FIRST 0x12EA

/*----------------------
 | LAYOUT_PACK_LAYOUT_BYTES
 | Description: Bytes of one converted layout at the start of its sector.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LAYOUT_PACK_LAYOUT_BYTES 1208

/*----------------------
 | LAYOUT_IMAGE_PACK_FIRST
 | Description: Resource id of cell 0's layout image, which also names the packed image file; must match wmap_resources.LAYOUT_IMAGE_FIRST.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LAYOUT_IMAGE_PACK_FIRST 0x14DE

/*----------------------
 | LAYOUT_IMAGE_PACK_BYTES / LAYOUT_IMAGE_PACK_STRIDE
 | Description: Bytes of one converted layout image and the whole sectors each takes in the pack; must match wmap_resources.LAYOUT_IMAGE_BYTES and LAYOUT_IMAGE_STRIDE_SECTORS.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LAYOUT_IMAGE_PACK_BYTES 73792
#define LAYOUT_IMAGE_PACK_STRIDE 37

/*----------------------
 | LAYOUT_PACK_COUNT
 | Description: Cells in each WMAP pack, one per WSEL grid cell.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LAYOUT_PACK_COUNT 361

/*----------------------
 | RESOURCE_PACK_QUEUE_READ / RESOURCE_PACK_STREAM
 | Description: The cdrom call a pack's ids are read through: cdrom_queue_read or cdrom_stream.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RESOURCE_PACK_QUEUE_READ 1
#define RESOURCE_PACK_STREAM 2

/*----------------------
 | ResourcePack
 | Description: One pack: first resource id (also its file's id), cell count, sectors per cell, bytes read per cell, and the cdrom call that reads it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    int first;
    int count;
    int stride_sectors;
    int item_bytes;
    int read_path;
} ResourcePack;

/*----------------------
 | resource_pack_find
 | Description: Returns the pack whose id range holds a resource id and stores its cell, or 0 for an id in no pack.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: resource_index -- PS1 resource id, cell -- receives the cell
 | Returns: pack or 0
 ----------------------*/
const ResourcePack *resource_pack_find(int resource_index, int *cell);

#endif
