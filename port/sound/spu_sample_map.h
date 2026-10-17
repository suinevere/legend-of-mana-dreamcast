/*----------------------
 | spu_sample_map.h
 | Description: PS1 SPU RAM as the AICA sees it: ADPCM uploads decode to 16-bit PCM at SPU address x 3.5, and each block's sample bounds answer key-on lookups.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_SPU_SAMPLE_MAP_H
#define LOM_SPU_SAMPLE_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------
 | SPU_RAM_BYTES
 | Description: Size of PS1 SPU RAM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_RAM_BYTES 0x80000u

/*----------------------
 | SPU_BLOCK_BYTES / SPU_BLOCK_FRAMES / SPU_BLOCK_COUNT
 | Description: One ADPCM block is 16 bytes holding 28 frames; SPU RAM holds this many blocks.
 | Author: suinevere
 | Dependencies: SPU_RAM_BYTES
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_BLOCK_BYTES 16u
#define SPU_BLOCK_FRAMES 28u
#define SPU_BLOCK_COUNT (SPU_RAM_BYTES / SPU_BLOCK_BYTES)

/*----------------------
 | SPU_IMAGE_BYTES
 | Description: AICA bytes the decoded image of all SPU RAM takes (x 3.5).
 | Author: suinevere
 | Dependencies: SPU_RAM_BYTES
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_IMAGE_BYTES (SPU_RAM_BYTES / 2u * 7u)

/*----------------------
 | AICA_FRAME_LIMIT
 | Description: Most frames one AICA channel can reach from its start address; longer samples are streamed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AICA_FRAME_LIMIT 65535u

/*----------------------
 | SPU_PCM_OFFSET
 | Description: AICA byte offset of the PCM decoded from a 16-byte-aligned SPU address.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: spu_addr -- SPU byte address
 | Returns: AICA byte offset
 ----------------------*/
#define SPU_PCM_OFFSET(spu_addr) ((unsigned)(spu_addr) / 2u * 7u)

/*----------------------
 | SpuSampleSpan
 | Description: What a voice keyed at a start address plays: AICA offset, frames to the sample end, loop frame and block, whether it loops and whether it is too long for one channel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct SpuSampleSpan
{
    unsigned pcm_offset;
    unsigned frames;
    unsigned loop_start;
    int loops;
    int streamed;
    unsigned start_block;
    unsigned loop_block;
} SpuSampleSpan;

/*----------------------
 | SpuPcmSink
 | Description: Receives decoded PCM for an AICA offset.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: aica_offset -- AICA byte offset, pcm -- frames, bytes -- byte count, ctx -- caller data
 | Returns: N/A
 ----------------------*/
typedef void (*SpuPcmSink)(unsigned aica_offset, const short *pcm, unsigned bytes, void *ctx);

/*----------------------
 | spu_adpcm_decode_block
 | Description: Decodes one PS1 ADPCM block into 28 frames, updating the two-frame history like the SPU.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: block -- 16 bytes, old / older -- history in and out, out -- 28 frames
 | Returns: N/A
 ----------------------*/
void spu_adpcm_decode_block(const unsigned char *block, int *old, int *older, short out[SPU_BLOCK_FRAMES]);

/*----------------------
 | spu_sample_map_reset
 | Description: Forgets every upload, so every block reads as never written.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void spu_sample_map_reset(void);

/*----------------------
 | spu_sample_map_write
 | Description: Copies ADPCM into the SPU RAM copy, records its blocks' flags and sample bounds, and sends the decoded PCM to the sink.
 | Author: suinevere
 | Dependencies: spu_adpcm_decode_block
 | Globals: N/A
 | Params: spu_addr -- 16-byte-aligned SPU address, adpcm -- data, size -- bytes, sink -- PCM receiver, ctx -- passed to sink
 | Returns: Bytes consumed: whole blocks that fit in SPU RAM
 ----------------------*/
unsigned spu_sample_map_write(unsigned spu_addr, const unsigned char *adpcm, unsigned size, SpuPcmSink sink, void *ctx);

/*----------------------
 | spu_sample_map_locate
 | Description: The span a voice plays when keyed with these start and repeat register values.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: start_addr8 / repeat_addr8 -- voice registers in 8-byte units, out -- span
 | Returns: 0, or -1 when the start block was never written
 ----------------------*/
int spu_sample_map_locate(unsigned start_addr8, unsigned repeat_addr8, SpuSampleSpan *out);

/*----------------------
 | spu_sample_map_ram
 | Description: The main-RAM copy of SPU RAM that uploads fill, read by streamed voices.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: SPU_RAM_BYTES bytes
 ----------------------*/
const unsigned char *spu_sample_map_ram(void);

#ifdef __cplusplus
}
#endif

#endif
