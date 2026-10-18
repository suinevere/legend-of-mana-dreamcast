/*----------------------
 | dc_system.h
 | Description: Declares boot-time platform setup, BIOS-font text and the on-screen/serial halt for the Dreamcast port.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef DC_SYSTEM_H
#define DC_SYSTEM_H

/*----------------------
 | dc_system_init
 | Description: Sets 640x480 RGB565 video and clears the framebuffer, ready for boot output.
 | Author: suinevere
 | Dependencies: vid_set_mode, vid_clear
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void dc_system_init(void);

/*----------------------
 | dc_print
 | Description: Draws BIOS-font text at character cell (x, y) on the framebuffer.
 | Author: suinevere
 | Dependencies: bfont_draw_str
 | Globals: N/A
 | Params: x -- character column, y -- character row, text -- NUL-terminated string
 | Returns: N/A
 ----------------------*/
void dc_print(int x, int y, const char *text);

/*----------------------
 | dc_log
 | Description: Writes a printf-style line to KallistiOS dbgio, prefixed "LOM: ".
 | Author: suinevere
 | Dependencies: dbglog
 | Globals: N/A
 | Params: format -- printf-style format string, ... -- format arguments
 | Returns: N/A
 ----------------------*/
void dc_log(const char *format, ...);

/*----------------------
 | dc_halt
 | Description: Logs "LOM HALT: <message>" and prints it on screen once, then spins forever: on a plain empty loop with interrupts disabled if called from interrupt context, otherwise via thd_sleep(1000).
 | Author: suinevere
 | Dependencies: dbglog, dc_print, thd_sleep, irq_inside_int, irq_disable
 | Globals: N/A
 | Params: message -- halt reason
 | Returns: N/A
 ----------------------*/
void dc_halt(const char *message) __attribute__((noreturn));

/*----------------------
 | dc_time_us
 | Description: Microseconds since boot from the KallistiOS timer, for decode timing.
 | Author: suinevere
 | Dependencies: timer_us_gettime64
 | Globals: N/A
 | Params: N/A
 | Returns: microseconds
 ----------------------*/
unsigned long long dc_time_us(void);

#endif
