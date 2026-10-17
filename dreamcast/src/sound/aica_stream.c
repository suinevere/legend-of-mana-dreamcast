/*----------------------
 | aica_stream.c
 | Description: The AICA stream channel pair: two looping 16-bit rings on channels 62 and 63 that a producer refills behind the play position.
 | Author: suinevere
 | Dependencies: aica_stream.h, aica.h, aica_convert.h, spu_backend.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "aica.h"
#include "aica_convert.h"
#include "aica_stream.h"

/*----------------------
 | STREAM_STAGING / STREAM_RATE_MIN / STREAM_RATE_MAX
 | Description: Frames per upload when de-interleaving, and the accepted sample rates.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STREAM_STAGING 512u
#define STREAM_RATE_MIN 4000u
#define STREAM_RATE_MAX 48000u

/*----------------------
 | STREAM_EG10 / STREAM_EG14 / STREAM_PAN_LEFT / STREAM_PAN_RIGHT
 | Description: Instant attack and release with key scaling off, and the hard-left and hard-right DIPAN values.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STREAM_EG10 0x001Fu
#define STREAM_EG14 ((0xFu << 10) | 0x1Fu)
#define STREAM_PAN_LEFT 0x1Fu
#define STREAM_PAN_RIGHT 0x0Fu

/*----------------------
 | s_running / s_channels / s_write / s_tl
 | Description: Whether the stream plays, its channel count, the ring write position, and each side's TL.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_running;
static int s_channels;
static unsigned s_write;
static unsigned char s_tl[2];

/*----------------------
 | ring_offset
 | Description: Sound RAM offset of a ring position on one side.
 | Author: suinevere
 | Dependencies: AICA_STREAM_RAM, AICA_STREAM_FRAMES
 | Globals: N/A
 | Params: side -- 0 left, 1 right, position -- frame
 | Returns: byte offset
 ----------------------*/
static unsigned ring_offset(int side, unsigned position)
{
    return AICA_STREAM_RAM + (unsigned)side * AICA_STREAM_FRAMES * 2u + position * 2u;
}

/*----------------------
 | key_side
 | Description: Keys one stream channel on looping over its ring at the stream's pitch, panned hard to its side.
 | Author: suinevere
 | Dependencies: aica_backend, aica_pitch_word, ring_offset
 | Globals: s_tl
 | Params: side -- 0 left, 1 right, rate_hz -- sample rate
 | Returns: N/A
 ----------------------*/
static void key_side(int side, unsigned rate_hz)
{
    SpuVoiceSetup setup;

    setup.span.pcm_offset = ring_offset(side, 0);
    setup.span.frames = AICA_STREAM_FRAMES;
    setup.span.loop_start = 0;
    setup.span.loops = 1;
    setup.span.streamed = 0;
    setup.span.start_block = 0;
    setup.span.loop_block = 0;
    setup.pitch = aica_pitch_word(0x1000, rate_hz);
    setup.tl = s_tl[side];
    setup.dipan = (unsigned char)(side == 0 ? STREAM_PAN_LEFT : STREAM_PAN_RIGHT);
    setup.eg10 = STREAM_EG10;
    setup.eg14 = STREAM_EG14;
    aica_backend.key_on(side == 0 ? AICA_STREAM_LEFT_CHANNEL : AICA_STREAM_RIGHT_CHANNEL, &setup);
}

/*----------------------
 | aica_stream_start
 | Description: Starts the stream at a sample rate with 1 or 2 channels: clears both rings and keys both channels looping over them, left hard left and right hard right.
 | Author: suinevere
 | Dependencies: aica_backend, ring_offset, key_side
 | Globals: s_running, s_channels, s_write
 | Params: rate_hz -- 4000..48000, channels -- 1 or 2
 | Returns: 0, or -1 on a bad argument or when already started
 ----------------------*/
int aica_stream_start(unsigned rate_hz, int channels)
{
    static const short zeros[STREAM_STAGING];
    unsigned position;
    int side;

    if (s_running || rate_hz < STREAM_RATE_MIN || rate_hz > STREAM_RATE_MAX || (channels != 1 && channels != 2))
        return -1;
    for (side = 0; side < 2; side++)
        for (position = 0; position < AICA_STREAM_FRAMES; position += STREAM_STAGING)
            aica_backend.upload(ring_offset(side, position), zeros, STREAM_STAGING * 2u);
    s_channels = channels;
    s_write = 0;
    s_running = 1;
    key_side(0, rate_hz);
    key_side(1, rate_hz);
    return 0;
}

/*----------------------
 | aica_stream_free
 | Description: Frames aica_stream_push can take now.
 | Author: suinevere
 | Dependencies: aica_ring_free, aica_backend
 | Globals: s_running, s_write
 | Params: N/A
 | Returns: frame count, 0 when stopped
 ----------------------*/
unsigned aica_stream_free(void)
{
    if (!s_running)
        return 0;
    return aica_ring_free(s_write, aica_backend.position(AICA_STREAM_LEFT_CHANNEL) % AICA_STREAM_FRAMES,
                          AICA_STREAM_FRAMES, AICA_STREAM_GUARD_FRAMES);
}

/*----------------------
 | push_run
 | Description: Writes a run of frames that does not cross the wrap into both rings, de-interleaving stereo through a staging buffer.
 | Author: suinevere
 | Dependencies: aica_backend, ring_offset
 | Globals: s_channels
 | Params: frames -- samples, count -- frames, position -- ring position
 | Returns: N/A
 ----------------------*/
static void push_run(const short *frames, unsigned count, unsigned position)
{
    static short staging[2][STREAM_STAGING];

    while (count != 0)
    {
        unsigned run = count < STREAM_STAGING ? count : STREAM_STAGING;
        unsigned i;

        for (i = 0; i < run; i++)
        {
            staging[0][i] = s_channels == 2 ? frames[2u * i] : frames[i];
            staging[1][i] = s_channels == 2 ? frames[2u * i + 1u] : frames[i];
        }
        aica_backend.upload(ring_offset(0, position), staging[0], run * 2u);
        aica_backend.upload(ring_offset(1, position), staging[1], run * 2u);
        frames += run * (unsigned)s_channels;
        position += run;
        count -= run;
    }
}

/*----------------------
 | aica_stream_push
 | Description: Copies frames after the write position, at most aica_stream_free() of them; stereo frames are interleaved left, right, and mono frames feed both rings.
 | Author: suinevere
 | Dependencies: aica_stream_free, aica_ring_split, push_run
 | Globals: s_write, s_channels
 | Params: frames -- samples, count -- frames offered
 | Returns: frames taken
 ----------------------*/
unsigned aica_stream_push(const short *frames, unsigned count)
{
    unsigned room = aica_stream_free();
    unsigned first;
    unsigned second;

    if (count > room)
        count = room;
    second = aica_ring_split(s_write, count, AICA_STREAM_FRAMES, &first);
    push_run(frames, first, s_write);
    push_run(frames + first * (unsigned)s_channels, second, 0);
    s_write = (s_write + count) % AICA_STREAM_FRAMES;
    return count;
}

/*----------------------
 | aica_stream_volume
 | Description: Stream volume per side on the PS1 CD-volume scale; SPU CD volume writes arrive here.
 | Author: suinevere
 | Dependencies: aica_level_pan, aica_backend
 | Globals: s_tl, s_running
 | Params: left / right -- 0..0x7FFF
 | Returns: N/A
 ----------------------*/
void aica_stream_volume(int left, int right)
{
    int level[2];
    int side;

    level[0] = left;
    level[1] = right;
    for (side = 0; side < 2; side++)
    {
        unsigned char dipan;
        int swept;
        int value = level[side] < 0 ? 0 : (level[side] > 0x7FFF ? 0x7FFF : level[side]);

        aica_level_pan((short)(value >> 1), 0, &s_tl[side], &dipan, &swept);
        if (s_running)
            aica_backend.set_level(side == 0 ? AICA_STREAM_LEFT_CHANNEL : AICA_STREAM_RIGHT_CHANNEL, s_tl[side],
                                   (unsigned char)(side == 0 ? STREAM_PAN_LEFT : STREAM_PAN_RIGHT));
    }
}

/*----------------------
 | aica_stream_stop
 | Description: Keys both stream channels off.
 | Author: suinevere
 | Dependencies: aica_backend
 | Globals: s_running
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void aica_stream_stop(void)
{
    if (!s_running)
        return;
    aica_backend.key_off(AICA_STREAM_LEFT_CHANNEL);
    aica_backend.key_off(AICA_STREAM_RIGHT_CHANNEL);
    s_running = 0;
}
