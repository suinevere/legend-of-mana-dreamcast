/*----------------------
 | cd_stream_kos.c
 | Description: Reads a streamed resource ahead of delivery on a KallistiOS thread, into a ring of sectors the paced stream fetches from.
 | Author: suinevere
 | Dependencies: cd_stream_kos.h, cdrom_dc.h, kos/thread.h
 | Globals: s_file, s_thread, s_ring, s_total, s_produced, s_consumed, s_stop
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>

#include <kos/thread.h>

#include "cd_stream_kos.h"
#include "cdrom_dc.h"

/*----------------------
 | RING_SECTORS / READ_CHUNK
 | Description: Sectors held ahead of delivery (128 KB, about 0.4 s of a movie), and sectors per disc read; the chunk divides the ring so a read never wraps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RING_SECTORS 64
#define READ_CHUNK 16

/*----------------------
 | s_file / s_thread / s_ring
 | Description: The open resource file, the reader thread, and the sector ring.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static FILE *s_file;
static kthread_t *s_thread;
static uint8_t s_ring[RING_SECTORS][CDROM_DC_SECTOR_BYTES] __attribute__((aligned(32)));

/*----------------------
 | s_total / s_produced / s_consumed / s_stop
 | Description: Sectors in the resource, sectors read so far, the sector delivery is on (earlier slots are free), and the reader's stop request.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_total;
static volatile unsigned s_produced;
static volatile unsigned s_consumed;
static volatile int s_stop;

/*----------------------
 | reader
 | Description: Reads chunks into free ring slots until the resource ends or a stop is asked, sleeping a millisecond when the ring is full.
 | Author: suinevere
 | Dependencies: fread, thd_sleep
 | Globals: s_file, s_ring, s_total, s_produced, s_consumed, s_stop
 | Params: arg -- unused
 | Returns: NULL
 ----------------------*/
static void *reader(void *arg)
{
    (void)arg;
    while (!s_stop && s_produced < s_total)
    {
        unsigned count = s_total - s_produced < READ_CHUNK ? s_total - s_produced : READ_CHUNK;
        size_t got;

        if (s_produced + count - s_consumed > RING_SECTORS)
        {
            thd_sleep(1);
            continue;
        }
        got = fread(s_ring[s_produced % RING_SECTORS], CDROM_DC_SECTOR_BYTES, count, s_file);
        if (got == 0)
        {
            break;
        }
        s_produced += (unsigned)got;
    }
    return NULL;
}

/*----------------------
 | cd_stream_kos_open
 | Description: Opens a raw resource unbuffered, so each chunk reads straight into the ring in one drive command, and starts reading it ahead; a stream already open is closed first.
 | Author: suinevere
 | Dependencies: cd_stream_kos_close, cdrom_dc_open_resource, setvbuf, thd_create
 | Globals: s_file, s_thread, s_total, s_produced, s_consumed, s_stop
 | Params: resource_index -- PS1 resource, total_sectors -- receives its length in sectors
 | Returns: 1, or 0 when the resource is not a raw file on this disc
 ----------------------*/
int cd_stream_kos_open(int resource_index, unsigned *total_sectors)
{
    unsigned size;

    cd_stream_kos_close();
    s_file = cdrom_dc_open_resource(resource_index, &size);
    if (s_file == NULL)
    {
        return 0;
    }
    setvbuf(s_file, NULL, _IONBF, 0);
    s_total = size / CDROM_DC_SECTOR_BYTES;
    s_produced = 0;
    s_consumed = 0;
    s_stop = 0;
    *total_sectors = s_total;
    s_thread = thd_create(0, reader, NULL);
    return 1;
}

/*----------------------
 | cd_stream_kos_fetch
 | Description: Returns a sector once the reader has it, freeing every slot before it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_ring, s_produced, s_consumed
 | Params: ctx -- unused, sector -- sector number
 | Returns: the sector, or NULL while it is unread
 ----------------------*/
const uint8_t *cd_stream_kos_fetch(void *ctx, unsigned sector)
{
    (void)ctx;
    if (sector >= s_produced)
    {
        return NULL;
    }
    s_consumed = sector;
    return s_ring[sector % RING_SECTORS];
}

/*----------------------
 | cd_stream_kos_close
 | Description: Stops and joins the reader and closes the file; nothing happens when no stream is open.
 | Author: suinevere
 | Dependencies: thd_join, fclose
 | Globals: s_file, s_thread, s_stop
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void cd_stream_kos_close(void)
{
    if (s_thread != NULL)
    {
        s_stop = 1;
        thd_join(s_thread, NULL);
        s_thread = NULL;
    }
    if (s_file != NULL)
    {
        fclose(s_file);
        s_file = NULL;
    }
}
