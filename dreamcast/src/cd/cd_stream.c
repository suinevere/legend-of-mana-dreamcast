/*----------------------
 | cd_stream.c
 | Description: Paced sector delivery for CdlReadS: a sector becomes due every 60/150 of a vblank from the stream's start and is handed to the transfer callback through CdGetSector.
 | Author: suinevere
 | Dependencies: cd_stream.h, string.h
 | Globals: s_active, s_total, s_next, s_start_vbl, s_callback, s_fetch, s_ctx, s_current, s_offset, s_delivering
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <string.h>

#include "cd_stream.h"

/*----------------------
 | s_active / s_total / s_next / s_start_vbl
 | Description: Whether a stream runs, its length, the next sector to deliver, and the vblank it started on.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_active;
static unsigned s_total;
static unsigned s_next;
static unsigned s_start_vbl;

/*----------------------
 | s_callback / s_fetch / s_ctx
 | Description: The PS1 transfer callback and the sector source.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static CdStreamCallback s_callback;
static CdStreamFetch s_fetch;
static void *s_ctx;

/*----------------------
 | s_current / s_offset / s_delivering
 | Description: The sector being delivered, the bytes CdGetSector has read from it, and whether a delivery is running.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const uint8_t *s_current;
static unsigned s_offset;
static int s_delivering;

/*----------------------
 | cd_stream_start
 | Description: Records the stream and its start; nothing is due until time passes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: all of the above
 | Params: total_sectors, callback, now_vbl, fetch, ctx -- as cd_stream.h describes
 | Returns: N/A
 ----------------------*/
void cd_stream_start(unsigned total_sectors, CdStreamCallback callback, unsigned now_vbl, CdStreamFetch fetch, void *ctx)
{
    s_active = total_sectors > 0;
    s_total = total_sectors;
    s_next = 0;
    s_start_vbl = now_vbl;
    s_callback = callback;
    s_fetch = fetch;
    s_ctx = ctx;
    s_current = 0;
    s_offset = 0;
}

/*----------------------
 | cd_stream_service
 | Description: Delivers due, ready sectors in order up to the burst cap; a NULL callback result or the last sector ends the stream.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: all of the above
 | Params: now_vbl -- current vblank count
 | Returns: N/A
 ----------------------*/
void cd_stream_service(unsigned now_vbl)
{
    unsigned due, burst = 0;

    if (!s_active || s_delivering)
    {
        return;
    }
    due = (now_vbl - s_start_vbl) * CD_STREAM_SECTORS_PER_SECOND / CD_STREAM_VBLANKS_PER_SECOND;
    s_delivering = 1;
    while (s_active && s_next < due && burst < CD_STREAM_MAX_BURST)
    {
        const uint8_t *sector = s_fetch(s_ctx, s_next);
        unsigned char *more;

        if (sector == 0)
        {
            break;
        }
        s_current = sector;
        s_offset = 0;
        s_next++;
        burst++;
        more = s_callback(CD_STREAM_SECTOR_BYTES, (s_total - s_next) * CD_STREAM_SECTOR_BYTES);
        s_current = 0;
        if (more == 0 || s_next >= s_total)
        {
            s_active = 0;
        }
    }
    s_delivering = 0;
}

/*----------------------
 | CdGetSector
 | Description: Copies the next words of the sector being delivered.
 | Author: suinevere
 | Dependencies: memcpy
 | Globals: s_current, s_offset
 | Params: dst -- destination, words -- 32-bit words to copy
 | Returns: 1, or 0 with no sector being delivered or too few bytes left
 ----------------------*/
int CdGetSector(void *dst, int words)
{
    unsigned bytes = (unsigned)words * 4u;

    if (s_current == 0 || words < 0 || s_offset + bytes > CD_STREAM_SECTOR_BYTES)
    {
        return 0;
    }
    memcpy(dst, s_current + s_offset, bytes);
    s_offset += bytes;
    return 1;
}

/*----------------------
 | cd_stream_stop
 | Description: Ends the stream; later service calls deliver nothing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_active, s_current
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void cd_stream_stop(void)
{
    s_active = 0;
    s_current = 0;
}

/*----------------------
 | cd_stream_active
 | Description: Whether a stream is running.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_active
 | Params: N/A
 | Returns: 1 while running, else 0
 ----------------------*/
int cd_stream_active(void)
{
    return s_active;
}

/*----------------------
 | cd_stream_delivered
 | Description: Sectors delivered since the stream started.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_next
 | Params: N/A
 | Returns: sector count
 ----------------------*/
unsigned cd_stream_delivered(void)
{
    return s_next;
}
