/*----------------------
 | cd_stream_kos.h
 | Description: The KallistiOS read-ahead source behind cd_stream.c.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_CD_STREAM_KOS_H
#define LOM_CD_STREAM_KOS_H

#include <stdint.h>

/*----------------------
 | cd_stream_kos_open
 | Description: Opens a raw resource and starts reading it ahead on a thread; a stream already open is closed first.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: resource_index -- PS1 resource, total_sectors -- receives its length in sectors
 | Returns: 1, or 0 when the resource is not a raw file on this disc
 ----------------------*/
int cd_stream_kos_open(int resource_index, unsigned *total_sectors);

/*----------------------
 | cd_stream_kos_fetch
 | Description: A CdStreamFetch: returns a sector once read, freeing the slots before it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ctx -- unused, sector -- sector number
 | Returns: the sector, or NULL while unread
 ----------------------*/
const uint8_t *cd_stream_kos_fetch(void *ctx, unsigned sector);

/*----------------------
 | cd_stream_kos_close
 | Description: Stops and joins the reader and closes the file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void cd_stream_kos_close(void);

#endif
