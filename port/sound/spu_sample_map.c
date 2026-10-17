/*----------------------
 | spu_sample_map.c
 | Description: Decodes PS1 SPU uploads into the AICA image and keeps per-block sample bounds for key-on lookups.
 | Author: suinevere
 | Dependencies: spu_sample_map.h, string.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <string.h>

#include "spu_sample_map.h"

/*----------------------
 | SPU_FLAG_END / SPU_FLAG_REPEAT / SPU_FLAG_LOOP_START / SPU_BLOCK_WRITTEN
 | Description: ADPCM block flag bits, and the s_flags bit marking a block an upload filled.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_FLAG_END 0x01u
#define SPU_FLAG_REPEAT 0x02u
#define SPU_FLAG_LOOP_START 0x04u
#define SPU_BLOCK_WRITTEN 0x80u

/*----------------------
 | SPU_NO_BLOCK
 | Description: s_loop value for a block with no loop-start block after it in its sample.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_NO_BLOCK 0xFFFFu

/*----------------------
 | STAGING_FRAMES
 | Description: Frames decoded before each sink call (4 KB).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STAGING_FRAMES 2048u

/*----------------------
 | k_filter
 | Description: The SPU's ADPCM prediction filter coefficients (/64).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const int k_filter[5][2] = {{0, 0}, {60, 0}, {115, -52}, {98, -55}, {122, -60}};

/*----------------------
 | s_ram / s_flags / s_end / s_loop
 | Description: The SPU RAM copy, and per block its flags, its sample's end block, and the first loop-start block from it to the end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_ram[SPU_RAM_BYTES];
static unsigned char s_flags[SPU_BLOCK_COUNT];
static unsigned short s_end[SPU_BLOCK_COUNT];
static unsigned short s_loop[SPU_BLOCK_COUNT];

/*----------------------
 | s_cont_next / s_cont_first / s_cont_old / s_cont_older
 | Description: A sample the last upload left open: the block that would continue it, its first block and its decoder history.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static long s_cont_next = -1;
static unsigned s_cont_first;
static int s_cont_old;
static int s_cont_older;

/*----------------------
 | spu_adpcm_decode_block
 | Description: Decodes one PS1 ADPCM block into 28 frames, updating the two-frame history like the SPU.
 | Author: suinevere
 | Dependencies: k_filter
 | Globals: N/A
 | Params: block -- 16 bytes, old / older -- history in and out, out -- 28 frames
 | Returns: N/A
 ----------------------*/
void spu_adpcm_decode_block(const unsigned char *block, int *old, int *older, short out[SPU_BLOCK_FRAMES])
{
    int shift = block[0] & 0x0F;
    int filter = (block[0] >> 4) & 0x07;
    int i;

    if (shift > 12)
        shift = 9;
    if (filter > 4)
        filter = 0;
    for (i = 0; i < (int)SPU_BLOCK_FRAMES; i++)
    {
        int nibble = (block[2 + i / 2] >> ((i & 1) * 4)) & 0x0F;
        int value;

        if (nibble >= 8)
            nibble -= 16;
        value = ((nibble << 12) >> shift) + ((*old * k_filter[filter][0] + *older * k_filter[filter][1] + 32) >> 6);
        if (value > 32767)
            value = 32767;
        if (value < -32768)
            value = -32768;
        out[i] = (short)value;
        *older = *old;
        *old = value;
    }
}

/*----------------------
 | mark_sample
 | Description: Records a sample's end block on each of its blocks, and each block's next loop-start block.
 | Author: suinevere
 | Dependencies: s_flags, s_end, s_loop
 | Globals: s_end, s_loop
 | Params: first / end -- the sample's first and last block
 | Returns: N/A
 ----------------------*/
static void mark_sample(unsigned first, unsigned end)
{
    unsigned loop = SPU_NO_BLOCK;
    unsigned b = end + 1;

    while (b-- > first)
    {
        if (s_flags[b] & SPU_FLAG_LOOP_START)
            loop = b;
        s_end[b] = (unsigned short)end;
        s_loop[b] = (unsigned short)loop;
    }
}

/*----------------------
 | spu_sample_map_reset
 | Description: Forgets every upload, so every block reads as never written.
 | Author: suinevere
 | Dependencies: s_flags
 | Globals: s_flags, s_cont_next
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void spu_sample_map_reset(void)
{
    memset(s_flags, 0, sizeof(s_flags));
    s_cont_next = -1;
}

/*----------------------
 | spu_sample_map_write
 | Description: Copies ADPCM into the SPU RAM copy, records its blocks' flags and sample bounds, and sends the decoded PCM to the sink; a sample left open by the previous upload continues when this one starts at its next block.
 | Author: suinevere
 | Dependencies: spu_adpcm_decode_block, mark_sample
 | Globals: s_ram, s_flags, s_cont_next, s_cont_first, s_cont_old, s_cont_older
 | Params: spu_addr -- 16-byte-aligned SPU address, adpcm -- data, size -- bytes, sink -- PCM receiver, ctx -- passed to sink
 | Returns: Bytes consumed: whole blocks that fit in SPU RAM
 ----------------------*/
unsigned spu_sample_map_write(unsigned spu_addr, const unsigned char *adpcm, unsigned size, SpuPcmSink sink, void *ctx)
{
    static short staging[STAGING_FRAMES];
    unsigned base = spu_addr / SPU_BLOCK_BYTES;
    unsigned count = size / SPU_BLOCK_BYTES;
    unsigned i = 0;

    if (base >= SPU_BLOCK_COUNT)
        return 0;
    if (count > SPU_BLOCK_COUNT - base)
        count = SPU_BLOCK_COUNT - base;
    memcpy(s_ram + base * SPU_BLOCK_BYTES, adpcm, count * SPU_BLOCK_BYTES);
    for (i = 0; i < count; i++)
        s_flags[base + i] = (unsigned char)((adpcm[i * SPU_BLOCK_BYTES + 1] & 0x07u) | SPU_BLOCK_WRITTEN);

    i = 0;
    while (i < count)
    {
        unsigned first = base + i;
        unsigned end;
        unsigned b;
        unsigned staged = 0;
        unsigned staged_from = base + i;
        int old = 0;
        int older = 0;

        if (s_cont_next == (long)(base + i))
        {
            first = s_cont_first;
            old = s_cont_old;
            older = s_cont_older;
        }
        end = base + i;
        while (end + 1 < base + count && !(s_flags[end] & SPU_FLAG_END))
            end++;
        for (b = base + i; b <= end; b++)
        {
            if (staged + SPU_BLOCK_FRAMES > STAGING_FRAMES)
            {
                sink(SPU_PCM_OFFSET(staged_from * SPU_BLOCK_BYTES), staging, staged * 2u, ctx);
                staged_from = b;
                staged = 0;
            }
            spu_adpcm_decode_block(s_ram + b * SPU_BLOCK_BYTES, &old, &older, staging + staged);
            staged += SPU_BLOCK_FRAMES;
        }
        if (staged != 0)
            sink(SPU_PCM_OFFSET(staged_from * SPU_BLOCK_BYTES), staging, staged * 2u, ctx);
        mark_sample(first, end);
        if (s_flags[end] & SPU_FLAG_END)
        {
            s_cont_next = -1;
        }
        else
        {
            s_cont_next = (long)end + 1;
            s_cont_first = first;
            s_cont_old = old;
            s_cont_older = older;
        }
        i = end - base + 1;
    }
    return count * SPU_BLOCK_BYTES;
}

/*----------------------
 | spu_sample_map_locate
 | Description: The span a voice plays when keyed with these start and repeat register values; the loop is the repeat block when inside the sample, else the first loop-start block, else the start.
 | Author: suinevere
 | Dependencies: s_flags, s_end, s_loop
 | Globals: N/A
 | Params: start_addr8 / repeat_addr8 -- voice registers in 8-byte units, out -- span
 | Returns: 0, or -1 when the start block was never written
 ----------------------*/
int spu_sample_map_locate(unsigned start_addr8, unsigned repeat_addr8, SpuSampleSpan *out)
{
    unsigned start = start_addr8 / 2u;
    unsigned repeat = repeat_addr8 / 2u;
    unsigned end;
    unsigned loop;

    if (start >= SPU_BLOCK_COUNT || !(s_flags[start] & SPU_BLOCK_WRITTEN))
        return -1;
    end = s_end[start];
    if (repeat >= start && repeat <= end)
        loop = repeat;
    else if (s_loop[start] != SPU_NO_BLOCK)
        loop = s_loop[start];
    else
        loop = start;
    out->pcm_offset = SPU_PCM_OFFSET(start * SPU_BLOCK_BYTES);
    out->frames = (end - start + 1u) * SPU_BLOCK_FRAMES;
    out->loop_start = (loop - start) * SPU_BLOCK_FRAMES;
    out->loops = (s_flags[end] & SPU_FLAG_REPEAT) != 0;
    out->streamed = out->frames > AICA_FRAME_LIMIT;
    out->start_block = start;
    out->loop_block = loop;
    return 0;
}

/*----------------------
 | spu_sample_map_ram
 | Description: The main-RAM copy of SPU RAM that uploads fill, read by streamed voices.
 | Author: suinevere
 | Dependencies: s_ram
 | Globals: N/A
 | Params: N/A
 | Returns: SPU_RAM_BYTES bytes
 ----------------------*/
const unsigned char *spu_sample_map_ram(void)
{
    return s_ram;
}
