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
# | LoopBody
# | Description: A loop region decoded with carried history: its PCM, whether it settled, passes decoded, and how many passes one cycle spans.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LoopBody = collections.namedtuple("LoopBody", "pcm converged passes period")

#----------------------
# | LOOP_PASS_LIMIT
# | Description: Most loop passes steady_loop decodes before reporting non-convergence.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LOOP_PASS_LIMIT = 64


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


#----------------------
# | steady_loop
# | Description: Re-decodes a loop region with the history the SPU carries back to the loop start until a pass's start history repeats.
# | Author: suinevere
# | Dependencies: decode_block, LoopBody, LOOP_PASS_LIMIT
# | Globals: N/A
# | Params: data -- ADPCM bytes, offset -- sample byte offset, sample -- its first-pass Sample, loop_index -- block-aligned loop start, max_passes -- pass cap
# | Returns: LoopBody whose pcm unrolls the cycle of period passes, rotated so its first pass lines up with PS1 pass 1; pass 1 is the first-pass decode
#----------------------
def steady_loop(data, offset, sample, loop_index, max_passes=LOOP_PASS_LIMIT):
    start = offset + loop_index // SAMPLES_PER_BLOCK * BLOCK_SIZE
    end = offset + sample.blocks * BLOCK_SIZE
    outputs = []
    seen = {}
    hist = sample.hist
    for passes in range(2, max_passes + 1):
        seen[hist] = len(outputs)
        pcm = []
        for pos in range(start, end, BLOCK_SIZE):
            out, hist = decode_block(data[pos:pos + BLOCK_SIZE], hist)
            pcm.extend(out)
        outputs.append(pcm)
        if hist in seen:
            cycle = outputs[seen[hist]:]
            shift = -passes % len(cycle)
            cycle = cycle[shift:] + cycle[:shift]
            return LoopBody([value for body in cycle for value in body], True, passes, len(cycle))
    return LoopBody(outputs[-1] if outputs else sample.pcm[loop_index:], False, max_passes, 1)


#----------------------
# | to_pcm16_be
# | Description: Packs samples as signed 16-bit big-endian PCM for the SCSP.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: pcm -- list of samples
# | Returns: bytes
#----------------------
def to_pcm16_be(pcm):
    return struct.pack(">%dh" % len(pcm), *pcm)


#----------------------
# | to_pcm8
# | Description: Packs samples as signed 8-bit PCM for the SCSP, rounding to the nearest step and saturating at -128 and 127.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: pcm -- list of samples
# | Returns: bytes
#----------------------
def to_pcm8(pcm):
    return bytes(max(-128, min(127, (value + 128) >> 8)) & 0xFF for value in pcm)
