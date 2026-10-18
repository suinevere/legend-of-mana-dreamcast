/*----------------------
 | movie_audio_dc.h
 | Description: Movie audio on the Dreamcast: the MOVIE overlay's AKAO ring-stream calls, renamed at compile time, decode each ring block's SPU ADPCM and push it through D6's AICA stream channel.
 | Author: suinevere
 | Dependencies: aica_stream.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_MOVIE_AUDIO_DC_H
#define LOM_MOVIE_AUDIO_DC_H

/*----------------------
 | MovieAudioOps
 | Description: The stream output movie audio drives: start at a rate and channel count, frames free, push interleaved frames, PS1-scale volume, stop; host tests fake it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    int (*start)(unsigned rate_hz, int channels);
    unsigned (*free_frames)(void);
    unsigned (*push)(const short *frames, unsigned count);
    void (*volume)(int left, int right);
    void (*stop)(void);
} MovieAudioOps;

/*----------------------
 | movie_audio_set_ops
 | Description: Replaces the stream output; NULL restores the AICA stream channel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ops -- output, or NULL
 | Returns: N/A
 ----------------------*/
void movie_audio_set_ops(const MovieAudioOps *ops);

/*----------------------
 | movie_audio_start / movie_audio_advance / movie_audio_position / movie_audio_volume / movie_audio_stop
 | Description: akao_start_xa_stream, akao_xa_advance_frame, akao_xa_get_position and akao_set_xa_volume for the MOVIE overlay, plus the stop cdrom_reset calls.
 | Author: suinevere
 | Dependencies: movie_audio_dc.c
 | Globals: N/A
 | Params: ring_base -- first ring block, byte_count -- ring size, volume -- 0..127
 | Returns: start: 0, or -1 for an empty ring; advance and position: the next ring block to play, or -1 before playback; volume: 0
 ----------------------*/
int movie_audio_start(void *ring_base, unsigned byte_count);
int movie_audio_advance(void);
int movie_audio_position(void);
int movie_audio_volume(int volume);
void movie_audio_stop(void);

#endif
