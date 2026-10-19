/*----------------------
 | cd_stream.h
 | Description: Paced delivery of a streamed resource's 2048-byte sectors to a PS1 CdlReadS transfer callback, with CdGetSector reading the current one.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_CD_STREAM_H
#define LOM_CD_STREAM_H

#include <stdint.h>

/*----------------------
 | CD_STREAM_SECTOR_BYTES / CD_STREAM_SECTORS_PER_SECOND / CD_STREAM_MAX_BURST
 | Description: Sector size, the PS1 drive's double-speed rate, and most sectors one service call delivers after a stall.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CD_STREAM_SECTOR_BYTES 2048
#define CD_STREAM_SECTORS_PER_SECOND 150
#define CD_STREAM_MAX_BURST 30

/*----------------------
 | CdStreamFetch / CdStreamCallback
 | Description: Supplies a stream sector once it is read (NULL until then); and the PS1 transfer callback, which returns NULL to stop the read.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef const uint8_t *(*CdStreamFetch)(void *ctx, unsigned sector);
typedef unsigned char *(*CdStreamCallback)(int bytes_transferred, unsigned bytes_remaining);

/*----------------------
 | cd_stream_set_refresh
 | Description: Sets the vblank refresh rate (60 or 50) cd_stream_service paces delivery against; defaults to 60 until called.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: hz -- vblanks per second
 | Returns: N/A
 ----------------------*/
void cd_stream_set_refresh(unsigned hz);

/*----------------------
 | cd_stream_start
 | Description: Starts delivering total_sectors sectors from fetch to callback, timed from now_vbl.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: total_sectors -- stream length, callback -- PS1 transfer callback, now_vbl -- current vblank count, fetch, ctx -- sector source
 | Returns: N/A
 ----------------------*/
void cd_stream_start(unsigned total_sectors, CdStreamCallback callback, unsigned now_vbl, CdStreamFetch fetch, void *ctx);

/*----------------------
 | cd_stream_service
 | Description: Delivers, in order, every sector due by now_vbl that fetch has ready, at most CD_STREAM_MAX_BURST; returns at once when called from inside a delivery.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: now_vbl -- current vblank count
 | Returns: N/A
 ----------------------*/
void cd_stream_service(unsigned now_vbl);

/*----------------------
 | CdGetSector
 | Description: Copies the next words of the sector being delivered.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: dst -- destination, words -- 32-bit words to copy
 | Returns: 1, or 0 with no sector being delivered or too few bytes left in it
 ----------------------*/
int CdGetSector(void *dst, int words);

/*----------------------
 | cd_stream_stop / cd_stream_active / cd_stream_delivered
 | Description: Ends the stream; whether one is running; sectors delivered since it started.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: cd_stream_active: 1 while running; cd_stream_delivered: sector count
 ----------------------*/
void cd_stream_stop(void);
int cd_stream_active(void);
unsigned cd_stream_delivered(void);

#endif
