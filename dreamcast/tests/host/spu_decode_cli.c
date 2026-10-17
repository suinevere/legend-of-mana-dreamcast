/*----------------------
 | spu_decode_cli.c
 | Description: Host tool for the disc test: uploads one ADPCM file through spu_sample_map_write and writes the AICA image, optionally printing the span of given start blocks.
 | Author: suinevere
 | Dependencies: spu_sample_map.c, spu_sample_map.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spu_sample_map.h"

static short s_image[SPU_IMAGE_BYTES / 2];
static unsigned char s_input[SPU_RAM_BYTES];

static void sink(unsigned aica_offset, const short *pcm, unsigned bytes, void *ctx)
{
    (void)ctx;
    memcpy((unsigned char *)s_image + aica_offset, pcm, bytes);
}

int main(int argc, char **argv)
{
    FILE *in;
    FILE *out;
    size_t size;
    unsigned spu_addr;
    int i;

    if (argc < 4)
    {
        fprintf(stderr, "usage: spu_decode_cli <adpcm> <spu_addr-hex> <out.raw> [start_block ...]\n");
        return 2;
    }
    in = fopen(argv[1], "rb");
    if (in == NULL)
        return 2;
    size = fread(s_input, 1, sizeof(s_input), in);
    fclose(in);
    spu_addr = (unsigned)strtoul(argv[2], NULL, 16);
    spu_sample_map_reset();
    spu_sample_map_write(spu_addr, s_input, (unsigned)size, sink, NULL);
    out = fopen(argv[3], "wb");
    if (out == NULL)
        return 2;
    fwrite(s_image, 1, sizeof(s_image), out);
    fclose(out);
    for (i = 4; i < argc; i++)
    {
        SpuSampleSpan span;
        unsigned block = (unsigned)strtoul(argv[i], NULL, 10);

        if (spu_sample_map_locate(block * 2u, 0, &span) == 0)
            printf("%u %u %d\n", block, span.frames, span.streamed);
        else
            printf("%u -1 -1\n", block);
    }
    return 0;
}
