/*----------------------
 | movie_probe_dc.c
 | Description: Test-image decode-speed probe: decodes the intro's first frames straight from disc through libpress_dc.c and logs the per-frame decode time.
 | Author: suinevere
 | Dependencies: cdrom_dc.h, dc_system.h, watch.h, sdk/libpress.h
 | Globals: g_dc_movie_probe, s_sectors, s_frame, s_vlc, s_slice
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdint.h>
#include <string.h>

#include "cdrom_dc.h"
#include "dc_system.h"
#include "libpress_dc.h"
#include "watch.h"

/*----------------------
 | u_char / u_short / u_long
 | Description: The Psy-Q short type names libpress.h uses, repeated identically to include/common.h so this file builds without it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned long u_long;

#include "sdk/libpress.h"

/*----------------------
 | PROBE_RESOURCE / PROBE_SECTORS / PROBE_BATCH
 | Description: The intro's resource, the sectors probed (150 frames of 10), and the sectors read per disc access.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PROBE_RESOURCE 0x16A0
#define PROBE_SECTORS 1500
#define PROBE_BATCH 64

/*----------------------
 | STREAM_HEADER_BYTES / STREAM_PAYLOAD_BYTES / VIDEO_SECTOR_TYPE
 | Description: The movie stream header ahead of each sector's payload, the payload size, and the header type of a video sector.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STREAM_HEADER_BYTES 32
#define STREAM_PAYLOAD_BYTES (CDROM_DC_SECTOR_BYTES - STREAM_HEADER_BYTES)
#define VIDEO_SECTOR_TYPE 0x8001

/*----------------------
 | PROBE_COLUMNS / PROBE_SLICE_WORDS
 | Description: Output columns of a 320x240 frame and the words of one 24-bit column, as movie.c sizes them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PROBE_COLUMNS 20
#define PROBE_SLICE_WORDS (24 * 240 / 2)

/*----------------------
 | s_sectors / s_frame / s_vlc / s_slice
 | Description: One disc batch, one frame's assembled bitstream, the decoded MDEC codes, and one output column.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint8_t s_sectors[PROBE_BATCH * CDROM_DC_SECTOR_BYTES];
static u_long s_frame[20480 / 4];
static u_long s_vlc[81920 / 4];
static u_long s_slice[PROBE_SLICE_WORDS];

/*----------------------
 | decode_frame
 | Description: Decodes the assembled frame into all 20 columns, as the full-screen movie does.
 | Author: suinevere
 | Dependencies: DecDCTvlc2, DecDCTin, DecDCTout
 | Globals: s_frame, s_vlc, s_slice
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void decode_frame(void)
{
    int column;

    DecDCTvlc2(s_frame, s_vlc, 0);
    DecDCTin(s_vlc, 1);
    for (column = 0; column < PROBE_COLUMNS; column++)
    {
        DecDCTout(s_slice, PROBE_SLICE_WORDS);
    }
}

/*----------------------
 | movie_probe_run
 | Description: Reads the probed sectors in batches, assembles each video frame by chunk index, decodes it, and logs the decode timing.
 | Author: suinevere
 | Dependencies: cdrom_dc_read_sectors, decode_frame, libpress_dc_stats, dc_log, DecDCTReset, DecDCTvlcBuild
 | Globals: s_sectors, s_frame
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void movie_probe_run(void)
{
    unsigned first, k, frames, us_avg, us_max;

    DecDCTReset(0);
    DecDCTvlcBuild(0);
    libpress_dc_stats(&frames, &us_avg, &us_max);
    for (first = 0; first < PROBE_SECTORS; first += PROBE_BATCH)
    {
        unsigned count = PROBE_SECTORS - first < PROBE_BATCH ? PROBE_SECTORS - first : PROBE_BATCH;

        if (cdrom_dc_read_sectors(PROBE_RESOURCE, first, count, s_sectors) != (int)count)
        {
            dc_log("movie probe cannot read resource 0x%X", PROBE_RESOURCE);
            return;
        }
        for (k = 0; k < count; k++)
        {
            const uint8_t *sector = &s_sectors[k * CDROM_DC_SECTOR_BYTES];
            uint16_t type, chunk, chunks;

            memcpy(&type, sector + 2, 2);
            memcpy(&chunk, sector + 4, 2);
            memcpy(&chunks, sector + 6, 2);
            if (type != VIDEO_SECTOR_TYPE || (unsigned)(chunk + 1) * STREAM_PAYLOAD_BYTES > sizeof(s_frame))
            {
                continue;
            }
            memcpy((uint8_t *)s_frame + chunk * STREAM_PAYLOAD_BYTES, sector + STREAM_HEADER_BYTES, STREAM_PAYLOAD_BYTES);
            if (chunk + 1 == chunks)
            {
                decode_frame();
            }
        }
    }
    libpress_dc_stats(&frames, &us_avg, &us_max);
    dc_log("movie probe frames %u us_avg %u us_max %u", frames, us_avg, us_max);
}

/*----------------------
 | g_dc_movie_probe
 | Description: The pad-script hook "probe movie" calls.
 | Author: suinevere
 | Dependencies: movie_probe_run
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void (*g_dc_movie_probe)(void) = movie_probe_run;
