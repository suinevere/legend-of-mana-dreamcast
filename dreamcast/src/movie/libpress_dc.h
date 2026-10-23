/*----------------------
 | libpress_dc.h
 | Description: Dreamcast-only additions to libpress: decode timing for the probe and the movie log.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_LIBPRESS_DC_H
#define LOM_LIBPRESS_DC_H

/*----------------------
 | libpress_dc_stats
 | Description: Reports decode timing since the previous call and starts a new period.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: frames, us_avg, us_max -- outputs
 | Returns: N/A
 ----------------------*/
void libpress_dc_stats(unsigned *frames, unsigned *us_avg, unsigned *us_max);

/*----------------------
 | libpress_dc_set_runner
 | Description: Installs the worker DecDCTout hands decodes to (start runs a job asynchronously, wait lets it make progress while the caller waits), or null hooks for decoding in the call; waits out a decode in flight first.
 | Author: suinevere
 | Dependencies: wait_job
 | Globals: s_start, s_wait
 | Params: start -- runs a job on the worker, wait -- yields to the worker
 | Returns: N/A
 ----------------------*/
void libpress_dc_set_runner(void (*start)(void (*job)(void)), void (*wait)(void));

/*----------------------
 | libpress_dc_poll
 | Description: Delivers the output callback of a decode the worker finished; called from the platform's VSync, DrawSync and CD polls, which must not stall the game behind the worker.
 | Author: suinevere
 | Dependencies: finish_job, drain
 | Globals: s_job_done
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void libpress_dc_poll(void);

/*----------------------
 | libpress_dc_poll_wait
 | Description: Yields to the worker while a decode is running, then polls; for the movie loop's audio-position poll, where the game only spins waiting for that decode.
 | Author: suinevere
 | Dependencies: libpress_dc_poll
 | Globals: s_job_busy, s_job_done, s_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void libpress_dc_poll_wait(void);

#endif
