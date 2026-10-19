/*----------------------
 | test_movie_audio.c
 | Description: Host tests for movie audio: AKAO's ring positions, playback starting after two filled blocks, at most two blocks queued, each block decoded left and right up to its loop-end flag with running history, the last block, silence after it, volume and stop.
 | Author: suinevere
 | Dependencies: movie_audio_dc.c, spu_sample_map.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "movie_audio_dc.h"
#include "spu_sample_map.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

#define BLOCKS 6
#define BLOCK_BYTES 0x1000
#define HEADER_AT 0x80
#define DATA_AT 0xD0
#define RIGHT_AT (DATA_AT + 0x800)
#define ADPCM_BLOCKS 105
#define FRAMES (ADPCM_BLOCKS * 28)
#define CAPACITY 16128u
#define PUSHED_MAX (FRAMES * 2 * 12)

static unsigned char s_ring[BLOCKS * BLOCK_BYTES];
static short s_pushed[PUSHED_MAX];
static unsigned s_pushed_frames;
static unsigned s_queued;
static int s_started_rate;
static int s_started_channels;
static int s_volume_left;
static int s_volume_right;
static int s_stops;

static int fake_start(unsigned rate_hz, int channels)
{
    s_started_rate = (int)rate_hz;
    s_started_channels = channels;
    return 0;
}

static unsigned fake_free(void)
{
    return CAPACITY - s_queued;
}

static unsigned fake_push(const short *frames, unsigned count)
{
    unsigned room = CAPACITY - s_queued;

    count = count > room ? room : count;
    if ((s_pushed_frames + count) * 2 <= PUSHED_MAX)
    {
        memcpy(&s_pushed[s_pushed_frames * 2], frames, count * 2 * sizeof(short));
    }
    s_pushed_frames += count;
    s_queued += count;
    return count;
}

static int s_volume_calls[16];
static int s_volume_count;

static void fake_volume(int left, int right)
{
    s_volume_left = left;
    s_volume_right = right;
    if (s_volume_count < 16)
    {
        s_volume_calls[s_volume_count] = left == right ? left : -1;
    }
    s_volume_count++;
}

static void fake_stop(void)
{
    s_stops++;
}

static const MovieAudioOps s_ops = { fake_start, fake_free, fake_push, fake_volume, fake_stop };

static void put32(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static void make_block(int index, unsigned remaining)
{
    unsigned char *b = &s_ring[index * BLOCK_BYTES];
    int i;

    for (i = 0; i < BLOCK_BYTES; i++)
    {
        b[i] = (unsigned char)rand();
    }
    put32(b + HEADER_AT, 0x4F414B41u);
    put32(b + HEADER_AT + 0x10, remaining);
    put32(b + HEADER_AT + 0x18, 0x11u);
    b[HEADER_AT + 0x1C] = 0x00;
    b[HEADER_AT + 0x1D] = 0x10;
    put32(b + HEADER_AT + 0x20, ADPCM_BLOCKS * 16u);
    put32(b + HEADER_AT + 0x28, 0);
    for (i = 0; i < 128; i++)
    {
        unsigned char *left = b + DATA_AT + i * 16;

        left[0] = (unsigned char)(left[0] % 0x4D);
        left[1] = (unsigned char)(i == ADPCM_BLOCKS - 1 ? 1 : 0);
    }
    for (i = 0; i < 115; i++)
    {
        unsigned char *right = b + RIGHT_AT + i * 16;

        right[0] = (unsigned char)(right[0] % 0x4D);
        right[1] = (unsigned char)(i == ADPCM_BLOCKS - 1 ? 1 : 0);
    }
}

static int block_matches(int index, unsigned first_frame, int *hist)
{
    const unsigned char *b = &s_ring[index * BLOCK_BYTES];
    short left[28];
    short right[28];
    int i;
    int k;

    for (i = 0; i < ADPCM_BLOCKS; i++)
    {
        spu_adpcm_decode_block(b + DATA_AT + i * 16, &hist[0], &hist[1], left);
        spu_adpcm_decode_block(b + RIGHT_AT + i * 16, &hist[2], &hist[3], right);
        for (k = 0; k < 28; k++)
        {
            unsigned at = (first_frame + (unsigned)(i * 28 + k)) * 2;

            if (s_pushed[at] != left[k] || s_pushed[at + 1] != right[k])
            {
                return 0;
            }
        }
    }
    return 1;
}

static void reset(void)
{
    int i;

    srand(7);
    for (i = 0; i < BLOCKS; i++)
    {
        make_block(i, (unsigned)(BLOCKS - i) * ADPCM_BLOCKS * 16u);
    }
    s_pushed_frames = 0;
    s_queued = 0;
    s_started_rate = 0;
    s_started_channels = 0;
    s_stops = 0;
    movie_audio_set_ops(&s_ops);
}

static void test_empty_ring_is_refused(void)
{
    reset();
    CHECK(movie_audio_start(s_ring, 0) == -1);
}

static void test_playback_starts_after_two_blocks(void)
{
    reset();
    CHECK(movie_audio_start(s_ring, sizeof(s_ring)) == 0);
    CHECK(movie_audio_position() == -1);
    CHECK(movie_audio_advance() == -1);
    CHECK(s_started_rate == 0);
    CHECK(movie_audio_advance() == 2);
    CHECK(s_started_rate == 44100);
    CHECK(s_started_channels == 2);
    CHECK(s_pushed_frames == 2 * FRAMES);
}

static void test_queue_holds_two_blocks_and_decodes_in_order(void)
{
    int hist[4] = { 0, 0, 0, 0 };

    reset();
    movie_audio_start(s_ring, sizeof(s_ring));
    movie_audio_advance();
    movie_audio_advance();
    movie_audio_advance();
    movie_audio_advance();
    CHECK(movie_audio_position() == 2);
    CHECK(s_pushed_frames == 2 * FRAMES);
    s_queued -= FRAMES;
    CHECK(movie_audio_position() == 3);
    CHECK(s_pushed_frames == 3 * FRAMES);
    CHECK(block_matches(0, 0, hist));
    CHECK(block_matches(1, FRAMES, hist));
    CHECK(block_matches(2, 2 * FRAMES, hist));
}

static void test_unfilled_blocks_are_not_played(void)
{
    reset();
    movie_audio_start(s_ring, sizeof(s_ring));
    movie_audio_advance();
    movie_audio_advance();
    s_queued = 0;
    CHECK(movie_audio_position() == 2);
    CHECK(s_pushed_frames == 2 * FRAMES);
}

static void test_last_block_ends_with_silence(void)
{
    int i;

    reset();
    make_block(2, ADPCM_BLOCKS * 16u);
    movie_audio_start(s_ring, sizeof(s_ring));
    for (i = 0; i < 5; i++)
    {
        movie_audio_advance();
    }
    s_queued = 0;
    movie_audio_position();
    CHECK(s_pushed_frames == 3 * FRAMES + (CAPACITY - FRAMES));
    CHECK(s_pushed[(3 * FRAMES) * 2] == 0 && s_pushed[(3 * FRAMES) * 2 + 1] == 0);
    CHECK(movie_audio_position() == 3);
}

static void test_volume_and_stop(void)
{
    reset();
    movie_audio_volume(127);
    CHECK(s_volume_left == 0x7FFF && s_volume_right == 0x7FFF);
    movie_audio_volume(0);
    CHECK(s_volume_left == 0 && s_volume_right == 0);
    movie_audio_start(s_ring, sizeof(s_ring));
    movie_audio_advance();
    movie_audio_advance();
    movie_audio_stop();
    CHECK(s_stops == 1);
    CHECK(movie_audio_position() == -1);
}

static void test_fade_in_ramps_to_the_set_volume(void)
{
    reset();
    movie_audio_volume(127);
    put32(&s_ring[HEADER_AT + 0x28], 24);
    s_volume_count = 0;
    movie_audio_start(s_ring, sizeof(s_ring));
    movie_audio_advance();
    movie_audio_advance();
    CHECK(s_volume_count == 3);
    CHECK(s_volume_calls[0] == 0);
    CHECK(s_volume_calls[1] == 0x7FFF * FRAMES / 4410);
    CHECK(s_volume_calls[2] == 0x7FFF);
    s_queued -= FRAMES;
    movie_audio_position();
    CHECK(s_volume_count == 3);
}

static void test_volume_mid_fade_moves_the_target(void)
{
    reset();
    movie_audio_volume(127);
    put32(&s_ring[HEADER_AT + 0x28], 48);
    s_volume_count = 0;
    movie_audio_start(s_ring, sizeof(s_ring));
    movie_audio_advance();
    movie_audio_advance();
    CHECK(s_volume_count == 3);
    CHECK(s_volume_calls[2] == 0x7FFF * 2 * FRAMES / 8820);
    movie_audio_volume(0x40);
    CHECK(s_volume_count == 3);
    movie_audio_advance();
    s_queued -= FRAMES;
    movie_audio_position();
    CHECK(s_volume_count == 4);
    CHECK(s_volume_calls[3] == (0x40 * 0x7FFF) / 0x7F);
}

static void test_no_fade_without_ticks(void)
{
    reset();
    movie_audio_volume(127);
    s_volume_count = 0;
    movie_audio_start(s_ring, sizeof(s_ring));
    movie_audio_advance();
    movie_audio_advance();
    CHECK(s_volume_count == 0);
}

int main(void)
{
    test_empty_ring_is_refused();
    test_playback_starts_after_two_blocks();
    test_queue_holds_two_blocks_and_decodes_in_order();
    test_unfilled_blocks_are_not_played();
    test_last_block_ends_with_silence();
    test_volume_and_stop();
    test_fade_in_ramps_to_the_set_volume();
    test_volume_mid_fade_moves_the_target();
    test_no_fade_without_ticks();
    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
