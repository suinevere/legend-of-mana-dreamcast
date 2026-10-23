/*----------------------
 | cdrom_dc.h
 | Description: Dreamcast implementation of the decomp's cdrom.h API: stdio reads of converted resources from the disc, keyed by PS1 resource index.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_CDROM_DC_H
#define LOM_CDROM_DC_H

/*----------------------
 | LOM_CD_ROOT
 | Description: Disc mount point every resource path is built under; a host test overrides it with -D to point at a temp fixture directory.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_CD_ROOT
#define LOM_CD_ROOT "/cd"
#endif

#include <stdio.h>

/*----------------------
 | CDROM_DC_SECTOR_BYTES
 | Description: Bytes of one PS1 Mode 2 Form 1 sector's user data, as raw resources keep them on the Dreamcast disc.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CDROM_DC_SECTOR_BYTES 2048

/*----------------------
 | cdrom_dc_open_resource
 | Description: Opens a raw resource's disc file for sector-wise reading.
 | Author: suinevere
 | Dependencies: resource_index_find, cd_open
 | Globals: N/A
 | Params: resource_index -- PS1 resource, size_bytes -- receives its byte size
 | Returns: the open file, or NULL when the resource is not a raw file on this disc
 ----------------------*/
FILE *cdrom_dc_open_resource(int resource_index, unsigned *size_bytes);

/*----------------------
 | cdrom_dc_read_sectors
 | Description: Reads count whole sectors of a raw resource starting at sector first.
 | Author: suinevere
 | Dependencies: cdrom_dc_open_resource
 | Globals: N/A
 | Params: resource_index -- PS1 resource, first -- first sector, count -- sectors, dst -- count * 2048 bytes
 | Returns: sectors read, or -1 when the resource cannot be opened
 ----------------------*/
int cdrom_dc_read_sectors(int resource_index, unsigned first, unsigned count, void *dst);

/*----------------------
 | cdrom_dc_set_poll
 | Description: Registers the function cdrom_process_state calls first, so a worker's finished MDEC decode is delivered while the movie loop polls; null for none.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_process_poll
 | Params: poll -- function to call, or null
 | Returns: N/A
 ----------------------*/
void cdrom_dc_set_poll(void (*poll)(void));

/*----------------------
 | cdrom_dc_vsync_tick
 | Description: Delivers the stream sectors due by now; VSync calls it once per wait.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void cdrom_dc_vsync_tick(void);

#endif
