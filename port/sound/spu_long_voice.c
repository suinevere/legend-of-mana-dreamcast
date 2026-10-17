/*----------------------
 | spu_long_voice.c
 | Description: Ring-streamed voices for samples longer than an AICA channel reaches, decoded block by block with the PS1's loop-start, repeat and end flags.
 | Author: suinevere
 | Dependencies: spu_long_voice.h, spu_sample_map.h, string.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <string.h>

#include "spu_long_voice.h"

/*----------------------
 | LONG_STAGING_FRAMES
 | Description: Frames decoded per upload call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LONG_STAGING_FRAMES 512u

/*----------------------
 | LongVoice
 | Description: One streaming voice: decoder position and history, repeat block, ring write and last play positions, frames played and decoded, and where its sample ended.
 | Author: suinevere
 | Dependencies: SPU_BLOCK_FRAMES
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct LongVoice
{
    int active;
    int finished;
    int ended;
    unsigned block;
    unsigned frame;
    unsigned repeat_block;
    int old;
    int older;
    short current[SPU_BLOCK_FRAMES];
    unsigned write;
    unsigned last_position;
    unsigned long played;
    unsigned long decoded;
} LongVoice;

/*----------------------
 | s_voices
 | Description: Stream state per SPU voice.
 | Author: suinevere
 | Dependencies: LongVoice
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static LongVoice s_voices[SPU_VOICE_COUNT];

/*----------------------
 | decode_current
 | Description: Decodes the voice's current block, taking its loop-start flag as the new repeat block like the SPU.
 | Author: suinevere
 | Dependencies: spu_sample_map_ram, spu_adpcm_decode_block
 | Globals: N/A
 | Params: lv -- voice
 | Returns: N/A
 ----------------------*/
static void decode_current(LongVoice *lv)
{
    const unsigned char *block = spu_sample_map_ram() + lv->block * SPU_BLOCK_BYTES;

    if (block[1] & 0x04u)
        lv->repeat_block = lv->block;
    spu_adpcm_decode_block(block, &lv->old, &lv->older, lv->current);
    lv->frame = 0;
}

/*----------------------
 | produce
 | Description: Next frames of a voice's sample: after the last frame of a block, an end block with repeat jumps to the repeat block keeping history, an end block without it ends the sample, and every later frame is silence.
 | Author: suinevere
 | Dependencies: decode_current, spu_sample_map_ram
 | Globals: N/A
 | Params: lv -- voice, out -- frames, count -- how many
 | Returns: N/A
 ----------------------*/
static void produce(LongVoice *lv, short *out, unsigned count)
{
    unsigned i;

    for (i = 0; i < count; i++)
    {
        if (!lv->ended && lv->frame == SPU_BLOCK_FRAMES)
        {
            unsigned char flags = spu_sample_map_ram()[lv->block * SPU_BLOCK_BYTES + 1];

            if (!(flags & 0x01u))
            {
                lv->block++;
                decode_current(lv);
            }
            else if (flags & 0x02u)
            {
                lv->block = lv->repeat_block;
                decode_current(lv);
            }
            else
            {
                lv->ended = 1;
            }
        }
        if (lv->ended)
        {
            out[i] = 0;
            continue;
        }
        out[i] = lv->current[lv->frame++];
        lv->decoded++;
    }
}

/*----------------------
 | fill
 | Description: Decodes frames into a voice's ring from its write position, wrapping, in uploads of at most LONG_STAGING_FRAMES.
 | Author: suinevere
 | Dependencies: produce
 | Globals: N/A
 | Params: backend -- sound chip, voice -- 0..23, count -- frames
 | Returns: N/A
 ----------------------*/
static void fill(const SpuBackend *backend, int voice, unsigned count)
{
    static short staging[LONG_STAGING_FRAMES];
    LongVoice *lv = &s_voices[voice];
    unsigned ring = SPU_LONG_RING_RAM + (unsigned)voice * SPU_LONG_RING_STRIDE;

    while (count != 0)
    {
        unsigned run = SPU_LONG_RING_FRAMES - lv->write;

        if (run > count)
            run = count;
        if (run > LONG_STAGING_FRAMES)
            run = LONG_STAGING_FRAMES;
        produce(lv, staging, run);
        backend->upload(ring + lv->write * 2u, staging, run * 2u);
        lv->write = (lv->write + run) % SPU_LONG_RING_FRAMES;
        count -= run;
    }
}

/*----------------------
 | spu_long_voice_start
 | Description: Starts streaming a voice: fills its ring from the span's start block and keys the channel on looping over the ring.
 | Author: suinevere
 | Dependencies: decode_current, fill
 | Globals: s_voices
 | Params: backend -- sound chip, voice -- 0..23, setup -- the key-on setup with the streamed span
 | Returns: N/A
 ----------------------*/
void spu_long_voice_start(const SpuBackend *backend, int voice, const SpuVoiceSetup *setup)
{
    LongVoice *lv = &s_voices[voice];
    SpuVoiceSetup ring = *setup;

    memset(lv, 0, sizeof(*lv));
    lv->active = 1;
    lv->block = setup->span.start_block;
    lv->repeat_block = setup->span.loop_block;
    decode_current(lv);
    fill(backend, voice, SPU_LONG_RING_FRAMES - SPU_LONG_GUARD_FRAMES);
    ring.span.pcm_offset = SPU_LONG_RING_RAM + (unsigned)voice * SPU_LONG_RING_STRIDE;
    ring.span.frames = SPU_LONG_RING_FRAMES;
    ring.span.loop_start = 0;
    ring.span.loops = 1;
    ring.span.streamed = 0;
    backend->key_on(voice, &ring);
}

/*----------------------
 | spu_long_voice_stop
 | Description: Drops a voice's stream state, before a new key-on of that voice.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_voices
 | Params: voice -- 0..23
 | Returns: N/A
 ----------------------*/
void spu_long_voice_stop(int voice)
{
    memset(&s_voices[voice], 0, sizeof(s_voices[voice]));
}

/*----------------------
 | spu_long_voice_service
 | Description: Once per tick: adds each streaming voice's frames played since the last call, refills its ring up to just behind the play position, and keys off a voice once it has played past its sample's end.
 | Author: suinevere
 | Dependencies: fill
 | Globals: s_voices
 | Params: backend -- sound chip
 | Returns: N/A
 ----------------------*/
void spu_long_voice_service(const SpuBackend *backend)
{
    int v;

    for (v = 0; v < SPU_VOICE_COUNT; v++)
    {
        LongVoice *lv = &s_voices[v];
        unsigned position;
        unsigned target;

        if (!lv->active)
            continue;
        position = backend->position(v) % SPU_LONG_RING_FRAMES;
        lv->played += (position + SPU_LONG_RING_FRAMES - lv->last_position) % SPU_LONG_RING_FRAMES;
        lv->last_position = position;
        if (lv->ended && lv->played >= lv->decoded)
        {
            backend->key_off(v);
            lv->active = 0;
            lv->finished = 1;
            continue;
        }
        target = (position + SPU_LONG_RING_FRAMES - SPU_LONG_GUARD_FRAMES) % SPU_LONG_RING_FRAMES;
        fill(backend, v, (target + SPU_LONG_RING_FRAMES - lv->write) % SPU_LONG_RING_FRAMES);
    }
}

/*----------------------
 | spu_long_voice_active
 | Description: Whether a voice is streaming.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_voices
 | Params: voice -- 0..23
 | Returns: 1 or 0
 ----------------------*/
int spu_long_voice_active(int voice)
{
    return s_voices[voice].active;
}

/*----------------------
 | spu_long_voice_finished
 | Description: Whether a voice's stream played to an unrepeated end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_voices
 | Params: voice -- 0..23
 | Returns: 1 or 0
 ----------------------*/
int spu_long_voice_finished(int voice)
{
    return s_voices[voice].finished;
}
