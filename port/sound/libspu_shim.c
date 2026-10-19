/*----------------------
 | libspu_shim.c
 | Description: The Psy-Q libspu calls AKAO and boot make, on the shared SPU shim: uploads decode through the sample map, envelope reads come from the backend, and unsupported features log once.
 | Author: suinevere
 | Dependencies: libspu.h, spu_shim.h, spu_shadow.h, spu_sample_map.h, spu_long_voice.h, stdio.h, string.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include <libspu.h>

#include "spu_long_voice.h"
#include "spu_sample_map.h"
#include "spu_shadow.h"
#include "spu_shim.h"

/*----------------------
 | s_started / s_transfer_addr / s_transfer_callback / s_irq_callback / s_reverb_mode
 | Description: Whether the shim has reset its state, the next upload address, the stored callbacks and the reverb mode AKAO set.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_started;
static unsigned long s_transfer_addr;
static SpuTransferCallbackProc s_transfer_callback;
static SpuIRQCallbackProc s_irq_callback;
static long s_reverb_mode;

/*----------------------
 | s_logged_read / s_logged_reverb_mode / s_logged_upload_limit
 | Description: Unsupported calls, and conditions worth one log line, already reported.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_logged_read;
static int s_logged_reverb_mode;
static int s_logged_upload_limit;

/*----------------------
 | log_once
 | Description: Logs a message the first time its flag is clear.
 | Author: suinevere
 | Dependencies: spu_shim_log
 | Globals: N/A
 | Params: flag -- log-once flag, message -- text
 | Returns: N/A
 ----------------------*/
static void log_once(int *flag, const char *message)
{
    if (*flag)
        return;
    *flag = 1;
    spu_shim_log(message);
}

/*----------------------
 | upload_sink
 | Description: Passes decoded PCM to the backend, clipping or dropping anything at or past the reverb ring and logging that once.
 | Author: suinevere
 | Dependencies: spu_shim_backend, log_once, SPU_PCM_LIMIT
 | Globals: s_logged_upload_limit
 | Params: aica_offset -- AICA byte offset, pcm -- frames, bytes -- byte count, ctx -- unused
 | Returns: N/A
 ----------------------*/
static void upload_sink(unsigned aica_offset, const short *pcm, unsigned bytes, void *ctx)
{
    const SpuBackend *backend = spu_shim_backend();

    (void)ctx;
    if (aica_offset >= SPU_PCM_LIMIT)
    {
        log_once(&s_logged_upload_limit, "spu: upload past 0x1A8000 dropped");
        return;
    }
    if (aica_offset + bytes > SPU_PCM_LIMIT)
    {
        bytes = SPU_PCM_LIMIT - aica_offset;
        log_once(&s_logged_upload_limit, "spu: upload past 0x1A8000 dropped");
    }
    if (backend != NULL)
        backend->upload(aica_offset, pcm, bytes);
}

/*----------------------
 | finish_transfer
 | Description: Runs the transfer callback, as the PS1 does when an upload completes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_transfer_callback
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void finish_transfer(void)
{
    if (s_transfer_callback != NULL)
        s_transfer_callback();
}

/*----------------------
 | SpuInit / SpuStart
 | Description: Clear the register shadow, the sample map and the voice flush's caches the first time either is called.
 | Author: suinevere
 | Dependencies: spu_shadow_reset, spu_sample_map_reset, spu_voice_flush_reset
 | Globals: s_started
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void SpuInit(void)
{
    if (s_started)
        return;
    s_started = 1;
    spu_shadow_reset();
    spu_sample_map_reset();
    spu_voice_flush_reset();
}

void SpuStart(void)
{
    SpuInit();
}

/*----------------------
 | SpuQuit / SpuInitMalloc / SpuSetTransferMode / SpuSetIRQ / SpuSetIRQAddr
 | Description: Calls with no effect on the shim; each returns its argument.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: as libspu.h
 | Returns: as libspu.h
 ----------------------*/
void SpuQuit(void)
{
}

long SpuInitMalloc(long count, char *top)
{
    (void)top;
    return count;
}

long SpuSetTransferMode(long mode)
{
    return mode;
}

long SpuSetIRQ(long on_off)
{
    return on_off;
}

unsigned long SpuSetIRQAddr(unsigned long addr)
{
    return addr;
}

/*----------------------
 | SpuSetTransferStartAddr
 | Description: Remembers where the next upload goes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_transfer_addr
 | Params: addr -- SPU byte address
 | Returns: addr
 ----------------------*/
unsigned long SpuSetTransferStartAddr(unsigned long addr)
{
    s_transfer_addr = addr;
    return addr;
}

/*----------------------
 | SpuWrite
 | Description: Uploads ADPCM at the transfer address: the sample map decodes it to the backend; the address advances and the transfer callback runs.
 | Author: suinevere
 | Dependencies: spu_sample_map_write, upload_sink, finish_transfer
 | Globals: s_transfer_addr
 | Params: addr -- data, size -- bytes
 | Returns: size
 ----------------------*/
unsigned long SpuWrite(unsigned char *addr, unsigned long size)
{
    spu_sample_map_write((unsigned)s_transfer_addr, addr, (unsigned)size, upload_sink, NULL);
    s_transfer_addr += size;
    finish_transfer();
    return size;
}

/*----------------------
 | SpuRead
 | Description: SPU RAM reads are not supported: zero-fills, logs once and completes.
 | Author: suinevere
 | Dependencies: log_once, finish_transfer
 | Globals: s_logged_read
 | Params: addr -- buffer, size -- bytes
 | Returns: size
 ----------------------*/
unsigned long SpuRead(unsigned char *addr, unsigned long size)
{
    memset(addr, 0, size);
    log_once(&s_logged_read, "spu: SpuRead unsupported");
    finish_transfer();
    return size;
}

/*----------------------
 | SpuSetTransferCallback / SpuSetIRQCallback
 | Description: Store a callback.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_transfer_callback, s_irq_callback
 | Params: func -- callback
 | Returns: the previous callback
 ----------------------*/
SpuTransferCallbackProc SpuSetTransferCallback(SpuTransferCallbackProc func)
{
    SpuTransferCallbackProc previous = s_transfer_callback;

    s_transfer_callback = func;
    return previous;
}

SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc func)
{
    SpuIRQCallbackProc previous = s_irq_callback;

    s_irq_callback = func;
    return previous;
}

/*----------------------
 | SpuSetReverb / SpuSetNoiseClock
 | Description: Enables or disables the backend's reverb; stores the SPU noise clock's low 6 bits for the voice flush to pitch noise voices with.
 | Author: suinevere
 | Dependencies: spu_shim_backend, spu_shim_set_noise_clock
 | Globals: N/A
 | Params: as libspu.h
 | Returns: the argument
 ----------------------*/
long SpuSetReverb(long on_off)
{
    const SpuBackend *backend = spu_shim_backend();

    if (backend != NULL && backend->reverb_enable != NULL)
        backend->reverb_enable(on_off != 0);
    return on_off;
}

long SpuSetNoiseClock(long n_clock)
{
    spu_shim_set_noise_clock((unsigned)n_clock & 0x3Fu);
    return n_clock;
}

/*----------------------
 | SpuSetReverbModeType / SpuGetReverbModeType / SpuSetReverbModeDepth / SpuClearReverbWorkArea
 | Description: Keep the reverb mode AKAO sets so it reads back, clearing the backend's work area on request and warning once if the mode is not Studio Large; depth and a bare clear pass straight to the backend.
 | Author: suinevere
 | Dependencies: spu_shim_backend, spu_shim_log
 | Globals: s_reverb_mode, s_logged_reverb_mode
 | Params: as libspu.h
 | Returns: as libspu.h
 ----------------------*/
long SpuSetReverbModeType(long mode)
{
    const SpuBackend *backend = spu_shim_backend();

    s_reverb_mode = mode & ~SPU_REV_MODE_CLEAR_WA;
    if ((mode & SPU_REV_MODE_CLEAR_WA) && backend != NULL && backend->reverb_clear != NULL)
        backend->reverb_clear();
    if (s_reverb_mode != 0 && s_reverb_mode != 4 && !s_logged_reverb_mode)
    {
        char line[64];

        s_logged_reverb_mode = 1;
        snprintf(line, sizeof(line), "spu: reverb mode %ld kept as studio large", s_reverb_mode);
        spu_shim_log(line);
    }
    return 0;
}

void SpuGetReverbModeType(long *mode)
{
    *mode = s_reverb_mode;
}

void SpuSetReverbModeDepth(short depth_left, short depth_right)
{
    const SpuBackend *backend = spu_shim_backend();

    if (backend != NULL && backend->reverb_depth != NULL)
        backend->reverb_depth(depth_left, depth_right);
}

long SpuClearReverbWorkArea(long mode)
{
    const SpuBackend *backend = spu_shim_backend();

    (void)mode;
    if (backend != NULL && backend->reverb_clear != NULL)
        backend->reverb_clear();
    return 0;
}

/*----------------------
 | SpuGetVoiceEnvelope
 | Description: A voice's envelope level for AKAO's voice allocation: 0 with no backend or once a streamed sample ended, else the backend's reading.
 | Author: suinevere
 | Dependencies: spu_shim_backend, spu_long_voice_finished
 | Globals: N/A
 | Params: vNum -- voice, envx -- out level
 | Returns: N/A
 ----------------------*/
void SpuGetVoiceEnvelope(int vNum, short *envx)
{
    const SpuBackend *backend = spu_shim_backend();

    if (backend == NULL || spu_long_voice_finished(vNum))
    {
        *envx = 0;
        return;
    }
    *envx = (short)backend->envelope(vNum);
}
