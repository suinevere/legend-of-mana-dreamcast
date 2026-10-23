/*----------------------
 | movie_audio_dc.c
 | Description: Plays a movie's CD-fed audio ring through the AICA stream channel: each 4 KB ring block holds an AKAO program header and the left and right SPU ADPCM halves AKAO would upload, decoded here up to their loop-end flags with the SPU's running history.
 |              Keeps AKAO's ring position, the next block to play, so MOVIE releases audio sectors as on the PS1, and holds at most two blocks queued like the SPU's two ring halves.
 | Author: suinevere
 | Dependencies: movie_audio_dc.h, aica_stream.h, spu_sample_map.h, src/main/audio/akao_xa_stream.c
 | Globals: s_ops, s_ring, s_block_count, s_fill_block, s_filled, s_played, s_position, s_playing, s_done, s_frames, s_history, s_pcm
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <string.h>

#include "movie_audio_dc.h"
#include "aica_stream.h"
#include "spu_sample_map.h"
#if defined(LOM_PAD_SCRIPT) && !defined(LOM_HOST_TEST)
#include "dc_system.h"
#endif

/*----------------------
 | RING_BLOCK_BYTES / HEADER_AT / DATA_AT / RIGHT_AT / LEFT_BLOCKS / RIGHT_BLOCKS
 | Description: One ring block, where its AKAO program header and ADPCM start (XA_RING_HEADER_OFFSET, XA_RING_DATA_OFFSET), where the right half starts (XA_SPU_BLOCK_A_RIGHT less XA_SPU_BLOCK_A), and the ADPCM blocks each SPU half can hold of the 0xF30 bytes AKAO uploads.
 | Author: suinevere
 | Dependencies: src/main/audio/akao_xa_stream.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RING_BLOCK_BYTES 0x1000u
#define HEADER_AT 0x80u
#define DATA_AT 0xD0u
#define RIGHT_AT (DATA_AT + 0x800u)
#define LEFT_BLOCKS 128u
#define RIGHT_BLOCKS 115u

/*----------------------
 | AKAO_MAGIC / STEREO_FLAG / LOOP_END_FLAG / PITCH_UNITY / UNITY_RATE / QUEUE_BLOCKS / VOLUME_MAX / POSITION_NONE
 | Description: "AKAO" (XA_AKAO_MAGIC), XA_FLAG_STEREO, the ADPCM block flag that jumps a voice to its repeat address, the SPU pitch of 44.1 kHz, blocks kept queued, AKAO's 7-bit XA volume ceiling, and the position before playback.
 | Author: suinevere
 | Dependencies: src/main/audio/internal/akao_driver.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AKAO_MAGIC 0x4F414B41u
#define STEREO_FLAG 0x1u
#define LOOP_END_FLAG 0x1u
#define PITCH_UNITY 0x1000u
#define UNITY_RATE 44100u
#define QUEUE_BLOCKS 2u
#define VOLUME_MAX 0x7F
#define POSITION_NONE (-1)

/*----------------------
 | SILENCE_FRAMES
 | Description: Frames of silence pushed per call once the last block has played, so the stream ring never replays old audio.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SILENCE_FRAMES 512u

/*----------------------
 | FADE_HEADER_AT / AKAO_TICK_HZ
 | Description: AkaoXaProgramHeader.fade_in_ticks inside the ring header, and AKAO's tick rate those ticks count at.
 | Author: suinevere
 | Dependencies: src/main/audio/internal/akao_driver.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FADE_HEADER_AT 0x28u
#define AKAO_TICK_HZ 240u

/*----------------------
 | default_start / default_free / default_push / default_volume / default_stop
 | Description: The AICA stream channel behind MovieAudioOps.
 | Author: suinevere
 | Dependencies: aica_stream_start, aica_stream_free, aica_stream_push, aica_stream_volume, aica_stream_stop
 | Globals: N/A
 | Params: as MovieAudioOps
 | Returns: as MovieAudioOps
 ----------------------*/
#ifdef LOM_HOST_TEST
static const MovieAudioOps s_default_ops = { 0, 0, 0, 0, 0 };
#else
static const MovieAudioOps s_default_ops = { aica_stream_start, aica_stream_free, aica_stream_push, aica_stream_volume, aica_stream_stop };
#endif

/*----------------------
 | s_ops / s_ring / s_block_count / s_fill_block / s_filled / s_played / s_position / s_playing / s_done
 | Description: Stream output, the ring and its block count, AKAO's fill cursor, blocks filled and played, the next block to play, and whether playback has started or reached its last block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const MovieAudioOps *s_ops = &s_default_ops;

/*----------------------
 | s_poll
 | Description: Called before each position report to deliver work a worker finished, such as an MDEC decode, since the movie loop polls here between vblanks; null for none.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void (*s_poll)(void);
static const unsigned char *s_ring;
static unsigned s_block_count;
static unsigned s_fill_block;
static unsigned s_filled;
static unsigned s_played;
static int s_position = POSITION_NONE;
static int s_playing;
static int s_done;

/*----------------------
 | s_frames / s_history / s_pcm
 | Description: Frames per played block, the left and right ADPCM history carried across blocks, and the interleaved output of one block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_frames;
static int s_history[4];
static short s_pcm[LEFT_BLOCKS * SPU_BLOCK_FRAMES * 2];

/*----------------------
 | s_volume_target / s_fade_frames / s_fade_done
 | Description: The last level movie_audio_volume set, and the running fade-in's length and frames pushed so far (no fade while s_fade_frames is 0).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_volume_target = 0x7FFF;
static unsigned s_fade_frames;
static unsigned s_fade_done;

/*----------------------
 | read32 / read16
 | Description: Little-endian reads from a ring block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- bytes
 | Returns: the value
 ----------------------*/
static unsigned read32(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

static unsigned read16(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

/*----------------------
 | decode_half
 | Description: Decodes one SPU half into every other frame of s_pcm, up to and including its loop-end block, as a voice plays it before jumping to the other half.
 | Author: suinevere
 | Dependencies: spu_adpcm_decode_block
 | Globals: s_pcm, s_history
 | Params: adpcm -- half start, max_blocks -- blocks the half holds, channel -- 0 left, 1 right, stereo -- interleave stride 2 when set
 | Returns: frames decoded
 ----------------------*/
static unsigned decode_half(const unsigned char *adpcm, unsigned max_blocks, int channel, int stereo)
{
    short out[SPU_BLOCK_FRAMES];
    unsigned stride = stereo ? 2u : 1u;
    unsigned frames = 0;
    unsigned b;
    unsigned k;

    for (b = 0; b < max_blocks; b++)
    {
        spu_adpcm_decode_block(adpcm + b * 16u, &s_history[channel * 2], &s_history[channel * 2 + 1], out);
        for (k = 0; k < SPU_BLOCK_FRAMES; k++)
        {
            s_pcm[(frames + k) * stride + (unsigned)channel] = out[k];
        }
        frames += SPU_BLOCK_FRAMES;
        if (adpcm[b * 16u + 1] & LOOP_END_FLAG)
        {
            break;
        }
    }
    return frames;
}

/*----------------------
 | fade_step
 | Description: Counts frames pushed during a fade-in and raises the stream volume toward the target in proportion, ending the fade at the target.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_fade_frames, s_fade_done, s_volume_target, s_ops
 | Params: frames -- frames just pushed
 | Returns: N/A
 ----------------------*/
static void fade_step(unsigned frames)
{
    int level;

    if (s_fade_frames == 0)
    {
        return;
    }
    s_fade_done += frames;
    if (s_fade_done >= s_fade_frames)
    {
        s_fade_frames = 0;
        s_ops->volume(s_volume_target, s_volume_target);
        return;
    }
    level = (int)(((long long)s_volume_target * s_fade_done) / s_fade_frames);
    s_ops->volume(level, level);
}

/*----------------------
 | play_block
 | Description: Decodes the ring block at the play position and pushes it, then advances the position; a block without the AKAO magic or whose remaining size no longer exceeds its own ends playback, as AKAO's refill does.
 | Author: suinevere
 | Dependencies: read32, decode_half, fade_step
 | Globals: s_ring, s_position, s_block_count, s_played, s_done, s_frames, s_pcm, s_ops
 | Params: stereo -- the stream's channel count is 2
 | Returns: N/A
 ----------------------*/
static void play_block(int stereo)
{
    const unsigned char *block = s_ring + (unsigned)s_position * RING_BLOCK_BYTES;
    unsigned frames;

    if (read32(block + HEADER_AT) != AKAO_MAGIC)
    {
        s_done = 1;
        return;
    }
    frames = decode_half(block + DATA_AT, LEFT_BLOCKS, 0, stereo);
    if (stereo)
    {
        unsigned right = decode_half(block + RIGHT_AT, RIGHT_BLOCKS, 1, 1);

        frames = right < frames ? right : frames;
    }
    s_ops->push(s_pcm, frames);
    s_frames = frames;
    fade_step(frames);
    s_played++;
    s_position = (int)(((unsigned)s_position + 1u) % s_block_count);
    if (!(read32(block + HEADER_AT + 0x10u) > read32(block + HEADER_AT + 0x20u)))
    {
        s_done = 1;
    }
}

/*----------------------
 | pump
 | Description: Plays filled blocks while fewer than QUEUE_BLOCKS are queued, then after the last block keeps the stream ring filled with silence.
 | Author: suinevere
 | Dependencies: play_block
 | Globals: s_playing, s_done, s_played, s_filled, s_frames, s_ring, s_ops, s_pcm
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void pump(void)
{
    int stereo;

    if (!s_playing)
    {
        return;
    }
    stereo = (read32(s_ring + HEADER_AT + 0x18u) & STEREO_FLAG) != 0;
    while (!s_done && s_played < s_filled)
    {
        unsigned capacity = AICA_STREAM_FRAMES - AICA_STREAM_GUARD_FRAMES;
        unsigned queued = capacity - s_ops->free_frames();

        if (s_frames != 0 && queued >= QUEUE_BLOCKS * s_frames)
        {
            return;
        }
        play_block(stereo);
    }
    if (s_done)
    {
        unsigned free_frames = s_ops->free_frames();

        memset(s_pcm, 0, sizeof(s_pcm));
        while (free_frames != 0)
        {
            unsigned count = free_frames < SILENCE_FRAMES ? free_frames : SILENCE_FRAMES;

            s_ops->push(s_pcm, count);
            free_frames -= count;
        }
    }
}

void movie_audio_set_poll(void (*poll)(void))
{
    s_poll = poll;
}

/*----------------------
 | movie_audio_set_ops
 | Description: Replaces the stream output; NULL restores the AICA stream channel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_ops
 | Params: ops -- output, or NULL
 | Returns: N/A
 ----------------------*/
void movie_audio_set_ops(const MovieAudioOps *ops)
{
    s_ops = ops != 0 ? ops : &s_default_ops;
}

/*----------------------
 | movie_audio_start
 | Description: akao_start_xa_stream: takes the ring and resets the cursors; nothing plays until two blocks are filled.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_ring, s_block_count, s_fill_block, s_filled, s_played, s_position, s_playing, s_done, s_frames, s_history, s_fade_frames
 | Params: ring_base -- first ring block, byte_count -- ring size
 | Returns: 0, or -1 for an empty ring
 ----------------------*/
int movie_audio_start(void *ring_base, unsigned byte_count)
{
    if (byte_count == 0)
    {
        return -1;
    }
    s_ring = (const unsigned char *)ring_base;
    s_block_count = byte_count / RING_BLOCK_BYTES;
    s_fill_block = 0;
    s_filled = 0;
    s_played = 0;
    s_position = POSITION_NONE;
    s_playing = 0;
    s_done = 0;
    s_frames = 0;
    s_fade_frames = 0;
    memset(s_history, 0, sizeof(s_history));
    return 0;
}

/*----------------------
 | movie_audio_advance
 | Description: akao_xa_advance_frame: counts one more filled block and, once the fill cursor passes the second block, starts the stream at the first block's rate and channel count, beginning its fade-in when the header asks for one.
 | Author: suinevere
 | Dependencies: read32, read16, pump, dc_log (LOM_PAD_SCRIPT)
 | Globals: s_fill_block, s_filled, s_block_count, s_playing, s_position, s_ring, s_ops, s_fade_frames, s_fade_done
 | Params: N/A
 | Returns: the next ring block to play, or -1 before playback
 ----------------------*/
int movie_audio_advance(void)
{
    if (s_ring == 0 || s_block_count == 0)
    {
        return s_position;
    }
    s_filled++;
    s_fill_block = (s_fill_block + 1u) % s_block_count;
    if (!s_playing && !s_done && s_fill_block >= 2u && read32(s_ring + HEADER_AT) == AKAO_MAGIC)
    {
        unsigned pitch = read16(s_ring + HEADER_AT + 0x1Cu);
        int stereo = (read32(s_ring + HEADER_AT + 0x18u) & STEREO_FLAG) != 0;

        unsigned rate = pitch * UNITY_RATE / PITCH_UNITY;
        int ticks = (int)read32(s_ring + HEADER_AT + FADE_HEADER_AT);

        s_ops->start(rate, stereo ? 2 : 1);
        if (ticks > 0)
        {
            s_fade_frames = (unsigned)ticks * rate / AKAO_TICK_HZ;
            s_fade_done = 0;
            s_ops->volume(0, 0);
#if defined(LOM_PAD_SCRIPT) && !defined(LOM_HOST_TEST)
            dc_log("movie audio fade %d", ticks);
#endif
        }
        s_playing = 1;
        s_position = 0;
    }
    pump();
    return s_position;
}

/*----------------------
 | movie_audio_position
 | Description: akao_xa_get_position: feeds the stream, then reports the next ring block to play.
 | Author: suinevere
 | Dependencies: pump
 | Globals: s_position
 | Params: N/A
 | Returns: the next ring block to play, or -1 before playback
 ----------------------*/
int movie_audio_position(void)
{
    if (s_poll != 0)
    {
        s_poll();
    }
    pump();
    return s_position;
}

/*----------------------
 | movie_audio_volume
 | Description: akao_set_xa_volume: scales AKAO's 7-bit XA volume to the stream's PS1 CD-volume range on both sides; during a fade-in it only moves the fade's target.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_ops, s_volume_target, s_fade_frames
 | Params: volume -- low 7 bits used
 | Returns: 0
 ----------------------*/
int movie_audio_volume(int volume)
{
    s_volume_target = ((volume & VOLUME_MAX) * 0x7FFF) / VOLUME_MAX;
    if (s_fade_frames == 0)
    {
        s_ops->volume(s_volume_target, s_volume_target);
    }
    return 0;
}

/*----------------------
 | movie_audio_stop
 | Description: Ends playback and keys the stream off, as akao_stop_xa does for AKAO's voices.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_playing, s_done, s_position, s_ring, s_ops
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void movie_audio_stop(void)
{
    if (s_playing)
    {
        s_ops->stop();
    }
    s_playing = 0;
    s_done = 1;
    s_position = POSITION_NONE;
    s_ring = 0;
}
