
#----------------------
# | spu_adpcm.py
# | Description: Decodes PS1 SPU ADPCM sample data to 16-bit PCM and reports the loop points its block flags describe.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import collections
import struct

#----------------------
# | BLOCK_SIZE
# | Description: Bytes in one ADPCM block: a shift/filter byte, a flag byte and fourteen bytes of 4-bit samples.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BLOCK_SIZE = 16

#----------------------
# | SAMPLES_PER_BLOCK
# | Description: PCM samples one ADPCM block decodes to.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SAMPLES_PER_BLOCK = 28

#----------------------
# | FLAG_LOOP_END
# | Description: Flag bit marking the last block of a sample; the voice jumps to the loop address or releases.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FLAG_LOOP_END = 0x01

#----------------------
# | FLAG_REPEAT
# | Description: Flag bit that, on the end block, makes the voice loop instead of releasing.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FLAG_REPEAT = 0x02

#----------------------
# | FLAG_LOOP_START
# | Description: Flag bit that latches the block's address as the voice loop address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FLAG_LOOP_START = 0x04

#----------------------
# | FILTERS
# | Description: Prediction coefficient pairs for the previous and second-previous sample, in 64ths, indexed by filter.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FILTERS = ((0, 0), (60, 0), (115, -52), (98, -55), (122, -60))

#----------------------
# | Sample
# | Description: A decoded sample: PCM, flag loop start or None, end index, repeat flag, blocks read, end history, and whether data ran out first.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Sample = collections.namedtuple("Sample", "pcm loop_start loop_end loops blocks hist truncated")

#----------------------
# | clamp16
# | Description: Saturates a value to the signed 16-bit range.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- integer
# | Returns: value limited to -32768..32767
#----------------------
def clamp16(value):
    return max(-32768, min(32767, value))


#----------------------
# | decode_block
# | Description: Decodes one 16-byte block; shifts 13 to 15 act as 9 and filters above 4 are rejected.
# | Author: suinevere
# | Dependencies: FILTERS, clamp16
# | Globals: N/A
# | Params: data -- 16 block bytes, hist -- (previous, second-previous) sample
# | Returns: (list of 28 samples, new history)
#----------------------
def decode_block(data, hist):
    shift = data[0] & 0x0F
    filt = (data[0] >> 4) & 0x07
    if filt >= len(FILTERS):
        raise ValueError("invalid ADPCM filter %d" % filt)
    if shift > 12:
        shift = 9
    f0, f1 = FILTERS[filt]
    old, older = hist
    out = []
    for i in range(SAMPLES_PER_BLOCK):
        nibble = (data[2 + i // 2] >> ((i & 1) * 4)) & 0x0F
        if nibble >= 8:
            nibble -= 16
        value = clamp16(((nibble << 12) >> shift) + ((old * f0 + older * f1 + 32) >> 6))
        out.append(value)
        older, old = old, value
    return out, (old, older)


#----------------------
# | decode
# | Description: Decodes blocks from offset until an end-flag block or the data runs out, tracking the flag loop points.
# | Author: suinevere
# | Dependencies: decode_block, Sample
# | Globals: N/A
# | Params: data -- ADPCM bytes, offset -- byte offset of the first block
# | Returns: Sample
#----------------------
def decode(data, offset=0):
    pcm = []
    hist = (0, 0)
    loop_start = None
    loops = False
    truncated = True
    blocks = 0
    while offset + BLOCK_SIZE <= len(data):
        chunk = data[offset:offset + BLOCK_SIZE]
        if chunk[1] & FLAG_LOOP_START:
            loop_start = len(pcm)
        samples, hist = decode_block(chunk, hist)
        pcm.extend(samples)
        blocks += 1
        offset += BLOCK_SIZE
        if chunk[1] & FLAG_LOOP_END:
            loops = bool(chunk[1] & FLAG_REPEAT)
            truncated = False
            break
    return Sample(pcm, loop_start, len(pcm), loops, blocks, hist, truncated)

