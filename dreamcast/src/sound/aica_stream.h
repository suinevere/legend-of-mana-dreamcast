/*----------------------
 | aica_stream.h
 | Description: A ring-buffered 16-bit PCM stream on AICA channels 62 (left) and 63 (right), the path D7's XA audio plays through, plus the pure ring arithmetic it rests on.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_AICA_STREAM_H
#define LOM_AICA_STREAM_H

/*----------------------
 | AICA_STREAM_LEFT_CHANNEL / AICA_STREAM_RIGHT_CHANNEL
 | Description: AICA channels the stream plays on, above the 24 SPU voices.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AICA_STREAM_LEFT_CHANNEL 62
#define AICA_STREAM_RIGHT_CHANNEL 63

/*----------------------
 | AICA_STREAM_RAM / AICA_STREAM_FRAMES / AICA_STREAM_GUARD_FRAMES
 | Description: Sound RAM offset of the left ring (the right ring follows it), frames per ring, and frames a push stays behind the play position.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AICA_STREAM_RAM 0x1C0000u
#define AICA_STREAM_FRAMES 16384u
#define AICA_STREAM_GUARD_FRAMES 256u

/*----------------------
 | aica_stream_start
 | Description: Starts the stream at a sample rate with 1 or 2 channels: clears both rings and keys both channels looping over them, left hard left and right hard right.
 | Author: suinevere
 | Dependencies: aica_backend
 | Globals: N/A
 | Params: rate_hz -- 4000..48000, channels -- 1 or 2
 | Returns: 0, or -1 on a bad argument or when already started
 ----------------------*/
int aica_stream_start(unsigned rate_hz, int channels);

/*----------------------
 | aica_stream_free
 | Description: Frames aica_stream_push can take now.
 | Author: suinevere
 | Dependencies: aica_ring_free
 | Globals: N/A
 | Params: N/A
 | Returns: frame count, 0 when stopped
 ----------------------*/
unsigned aica_stream_free(void);

/*----------------------
 | aica_stream_push
 | Description: Copies frames after the write position, at most aica_stream_free() of them; stereo frames are interleaved left, right, and mono frames feed both rings.
 | Author: suinevere
 | Dependencies: aica_ring_split, aica_backend
 | Globals: N/A
 | Params: frames -- samples, count -- frames offered
 | Returns: frames taken
 ----------------------*/
unsigned aica_stream_push(const short *frames, unsigned count);

/*----------------------
 | aica_stream_volume
 | Description: Stream volume per side on the PS1 CD-volume scale; SPU CD volume writes arrive here.
 | Author: suinevere
 | Dependencies: aica_level_pan, aica_backend
 | Globals: N/A
 | Params: left / right -- 0..0x7FFF
 | Returns: N/A
 ----------------------*/
void aica_stream_volume(int left, int right);

/*----------------------
 | aica_stream_stop
 | Description: Keys both stream channels off.
 | Author: suinevere
 | Dependencies: aica_backend
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void aica_stream_stop(void);

/*----------------------
 | aica_ring_free
 | Description: Frames writable from the write position before coming within guard frames of the play position; equal positions mean an empty ring.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: write / play -- ring positions, frames -- ring size, guard -- frames kept clear
 | Returns: frame count
 ----------------------*/
unsigned aica_ring_free(unsigned write, unsigned play, unsigned frames, unsigned guard);

/*----------------------
 | aica_ring_split
 | Description: Splits a write of count frames at the ring's wrap.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: write -- ring position, count -- frames, frames -- ring size, first -- out frames before the wrap
 | Returns: frames after the wrap
 ----------------------*/
unsigned aica_ring_split(unsigned write, unsigned count, unsigned frames, unsigned *first);

#endif
