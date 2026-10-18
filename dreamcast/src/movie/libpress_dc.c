/*----------------------
 | libpress_dc.c
 | Description: libpress on the Dreamcast: DecDCTvlc2 decodes a whole frame in software and DecDCTout runs the MDEC emulator, with callbacks delivered without re-entry.
 | Author: suinevere
 | Dependencies: libpress.h, mdec.h, str_vlc.h, dc_system.h
 | Globals: s_in, s_out_callback, s_in_callback_fn, s_in_callback, s_callback_pending, s_vlc_size, s_frame_open, s_frame_us, s_stat_frames, s_stat_us, s_stat_max, s_log_frames, s_log_us, s_log_max
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdint.h>

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

#include "dc_system.h"
#include "libpress_dc.h"
#include "mdec.h"
#include "str_vlc.h"

/*----------------------
 | VLC_OUTPUT_WORDS / VLC_INPUT_HALFWORDS
 | Description: The movie's largest decoded-frame buffer (STANDARD_VLC_INPUT_BYTES) in words, and the bitstream span read for one frame in halfwords.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VLC_OUTPUT_WORDS (81920 / 4)
#define VLC_INPUT_HALFWORDS (81920 / 2)

/*----------------------
 | LOG_INTERVAL_FRAMES
 | Description: Frames between decode timing log lines.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOG_INTERVAL_FRAMES 60

/*----------------------
 | s_in / s_out_callback / s_in_callback_fn
 | Description: The MDEC input command being decoded, and the installed output and input callbacks.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static MdecStream s_in;
static void (*s_out_callback)(void);
static void (*s_in_callback_fn)(void);

/*----------------------
 | s_in_callback / s_callback_pending
 | Description: Whether the output callback is running, and whether a DecDCTout finished while it ran.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_in_callback;
static int s_callback_pending;

/*----------------------
 | s_vlc_size
 | Description: The last DecDCTvlcSize2 value; decoding is never split, so it is only reported back.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_vlc_size;

/*----------------------
 | s_frame_open / s_frame_us
 | Description: Whether a frame is being timed, and its decode microseconds so far.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_frame_open;
static unsigned s_frame_us;

/*----------------------
 | s_stat_frames / s_stat_us / s_stat_max / s_log_frames / s_log_us / s_log_max / s_log_vlc_us
 | Description: Decode timing since the last libpress_dc_stats call, and since the last log line, with the log's VLC share.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_stat_frames;
static unsigned s_stat_us;
static unsigned s_stat_max;
static unsigned s_log_frames;
static unsigned s_log_us;
static unsigned s_log_max;
static unsigned s_log_vlc_us;

/*----------------------
 | close_frame
 | Description: Adds the timed frame to both sets of totals and logs every LOG_INTERVAL_FRAMES frames.
 | Author: suinevere
 | Dependencies: dc_log
 | Globals: s_frame_open, s_frame_us, s_stat_*, s_log_*
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void close_frame(void)
{
    if (!s_frame_open)
    {
        return;
    }
    s_frame_open = 0;
    s_stat_frames++;
    s_stat_us += s_frame_us;
    s_stat_max = s_frame_us > s_stat_max ? s_frame_us : s_stat_max;
    s_log_frames++;
    s_log_us += s_frame_us;
    s_log_max = s_frame_us > s_log_max ? s_frame_us : s_log_max;
    if (s_log_frames == LOG_INTERVAL_FRAMES)
    {
        dc_log("movie frames %u us_avg %u us_max %u vlc_avg %u", s_log_frames, s_log_us / s_log_frames, s_log_max, s_log_vlc_us / s_log_frames);
        s_log_vlc_us = 0;
        s_log_frames = 0;
        s_log_us = 0;
        s_log_max = 0;
    }
}

/*----------------------
 | libpress_dc_stats
 | Description: Reports decode timing since the previous call and starts a new period.
 | Author: suinevere
 | Dependencies: close_frame
 | Globals: s_stat_frames, s_stat_us, s_stat_max
 | Params: frames, us_avg, us_max -- outputs
 | Returns: N/A
 ----------------------*/
void libpress_dc_stats(unsigned *frames, unsigned *us_avg, unsigned *us_max)
{
    close_frame();
    *frames = s_stat_frames;
    *us_avg = s_stat_frames ? s_stat_us / s_stat_frames : 0;
    *us_max = s_stat_max;
    s_stat_frames = 0;
    s_stat_us = 0;
    s_stat_max = 0;
}

/*----------------------
 | DecDCTReset
 | Description: Builds the MDEC tables and drops the pending input; mode 0 also clears both callbacks.
 | Author: suinevere
 | Dependencies: mdec_init_tables
 | Globals: s_in, s_out_callback, s_in_callback_fn, s_callback_pending
 | Params: mode -- 0 full reset, 1 cancel the current decode
 | Returns: N/A
 ----------------------*/
void DecDCTReset(int mode)
{
    mdec_init_tables();
    s_in.src = 0;
    s_in.end = 0;
    s_callback_pending = 0;
    if (mode == 0)
    {
        s_out_callback = 0;
        s_in_callback_fn = 0;
    }
}

/*----------------------
 | DecDCTvlcBuild
 | Description: Builds the decoder's own lookup; the caller's table buffer is not used.
 | Author: suinevere
 | Dependencies: str_vlc_init
 | Globals: N/A
 | Params: table -- ignored
 | Returns: N/A
 ----------------------*/
void DecDCTvlcBuild(u_short *table)
{
    (void)table;
    str_vlc_init();
}

/*----------------------
 | DecDCTvlcSize2
 | Description: Records the per-call work limit, which a whole-frame decode never needs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_vlc_size
 | Params: size -- work limit
 | Returns: the previous limit
 ----------------------*/
int DecDCTvlcSize2(int size)
{
    int previous = s_vlc_size;

    s_vlc_size = size;
    return previous;
}

/*----------------------
 | DecDCTvlc2
 | Description: Decodes a whole frame into dst and opens its timing; a continuation call (bitstream NULL) has nothing left to do.
 | Author: suinevere
 | Dependencies: str_vlc_decode_frame, close_frame, dc_time_us
 | Globals: s_frame_open, s_frame_us
 | Params: bitstream -- frame data or NULL, dst -- MDEC command output, table -- ignored
 | Returns: 0, decoding is always complete
 ----------------------*/
int DecDCTvlc2(u_long *bitstream, u_long *dst, DECDCTTAB table)
{
    uint64_t t0;

    (void)table;
    if (bitstream == 0)
    {
        return 0;
    }
    close_frame();
    t0 = dc_time_us();
    str_vlc_decode_frame((const uint16_t *)bitstream, VLC_INPUT_HALFWORDS, (uint32_t *)dst, VLC_OUTPUT_WORDS);
    s_frame_open = 1;
    s_frame_us = (unsigned)(dc_time_us() - t0);
    s_log_vlc_us += s_frame_us;
    return 0;
}

/*----------------------
 | DecDCTin
 | Description: Starts an MDEC input command; mode bit 0 selects 24-bit output, bit 1 sets bit 15 of 15-bit pixels.
 | Author: suinevere
 | Dependencies: mdec_begin
 | Globals: s_in, s_in_callback_fn
 | Params: buf -- MDEC command and run-length codes, mode -- output mode
 | Returns: N/A
 ----------------------*/
void DecDCTin(u_long *buf, int mode)
{
    mdec_begin(&s_in, (const uint32_t *)buf, mode & 1, (mode >> 1) & 1);
    if (s_in_callback_fn != 0)
    {
        s_in_callback_fn();
    }
}

/*----------------------
 | DecDCTout
 | Description: Decodes size words of macroblocks into buf, then runs the output callback; a call from inside the callback only marks it pending.
 | Author: suinevere
 | Dependencies: mdec_decode_macroblock, dc_time_us
 | Globals: s_in, s_frame_us, s_callback_pending, s_in_callback, s_out_callback
 | Params: buf -- output, size -- words to fill
 | Returns: N/A
 ----------------------*/
void DecDCTout(u_long *buf, int size)
{
    uint64_t t0 = dc_time_us();
    unsigned mb_bytes = s_in.depth24 ? MDEC_MB_BYTES_24 : MDEC_MB_BYTES_15;
    unsigned count = (unsigned)size * 4u / mb_bytes;
    uint8_t *out = (uint8_t *)buf;
    unsigned i;

    for (i = 0; i < count; i++)
    {
        mdec_decode_macroblock(&s_in, out + i * mb_bytes);
    }
    s_frame_us += (unsigned)(dc_time_us() - t0);
    s_callback_pending = 1;
    if (s_in_callback)
    {
        return;
    }
    while (s_callback_pending && s_out_callback != 0)
    {
        s_callback_pending = 0;
        s_in_callback = 1;
        s_out_callback();
        s_in_callback = 0;
    }
}

/*----------------------
 | DecDCTinSync / DecDCToutSync
 | Description: Decoding finishes inside the calls that start it, so there is never anything to wait for.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mode -- ignored
 | Returns: 0
 ----------------------*/
int DecDCTinSync(int mode)
{
    (void)mode;
    return 0;
}

int DecDCToutSync(int mode)
{
    (void)mode;
    return 0;
}

/*----------------------
 | DecDCTinCallback
 | Description: Installs the callback DecDCTin runs once its input is taken.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_in_callback_fn
 | Params: func -- callback or NULL
 | Returns: the previous callback
 ----------------------*/
int DecDCTinCallback(void (*func)())
{
    int previous = (int)(intptr_t)s_in_callback_fn;

    s_in_callback_fn = (void (*)(void))func;
    return previous;
}

/*----------------------
 | DecDCToutCallback
 | Description: Installs the callback DecDCTout runs once its output is written.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_out_callback
 | Params: func -- callback or NULL
 | Returns: the previous callback
 ----------------------*/
int DecDCToutCallback(void (*func)())
{
    int previous = (int)(intptr_t)s_out_callback;

    s_out_callback = (void (*)(void))func;
    return previous;
}
