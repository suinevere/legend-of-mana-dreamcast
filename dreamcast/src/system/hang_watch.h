/*----------------------
 | hang_watch.h
 | Description: Turns a silent freeze into a halt screen: a vblank check that names where the threads were once the game stops calling VSync, and an assert handler that shows KallistiOS assertion failures.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_HANG_WATCH_H
#define LOM_HANG_WATCH_H

/*----------------------
 | hang_watch_overdue
 | Description: Whether more than limit vblanks have passed since the last fed count, across counter wraparound, and only once fed at least once.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: now -- current vblank count, fed -- count at the last VSync, limit -- vblanks allowed, armed -- nonzero once VSync has run
 | Returns: 1 if overdue, else 0
 ----------------------*/
int hang_watch_overdue(unsigned now, unsigned fed, unsigned limit, int armed);

/*----------------------
 | hang_watch_format
 | Description: Writes the halt text: vblanks without a VSync, the interrupted thread's label, PC and PR, and the main thread's PC and PR.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: out, out_size -- buffer, label -- interrupted thread, pc, pr -- its registers, main_pc, main_pr -- the main thread's, vblanks -- time without a VSync
 | Returns: out
 ----------------------*/
char *hang_watch_format(char *out, int out_size, const char *label, unsigned long pc, unsigned long pr, unsigned long main_pc, unsigned long main_pr,
                        unsigned vblanks);

/*----------------------
 | hang_watch_install
 | Description: Records the calling thread as the main thread, sets the limit from the video rate, and hooks the vblank check and the assert handler.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: video_hz -- vblanks per second
 | Returns: N/A
 ----------------------*/
void hang_watch_install(unsigned video_hz);

/*----------------------
 | hang_watch_feed
 | Description: Marks the game alive; VSync's hook calls it whenever a frame that drew something has closed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void hang_watch_feed(void);

#endif
