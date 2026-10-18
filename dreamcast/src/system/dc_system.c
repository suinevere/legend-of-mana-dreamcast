/*----------------------
 | dc_system.c
 | Description: Boot-time video setup, BIOS-font text, serial logging and the on-screen/serial halt for the Dreamcast port.
 | Author: suinevere
 | Dependencies: dc_system.h, dc/video.h, dc/biosfont.h, kos/dbglog.h, kos/thread.h, arch/irq.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdarg.h>
#include <stdio.h>
#include <arch/irq.h>
#include <dc/biosfont.h>
#include <dc/video.h>
#include <kos/dbglog.h>
#include <kos/thread.h>
#include <arch/timer.h>
#include "dc_system.h"

/*----------------------
 | dc_system_init
 | Description: Sets 640x480 RGB565 video and clears the framebuffer, ready for boot output.
 | Author: suinevere
 | Dependencies: vid_set_mode, vid_clear
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void dc_system_init(void)
{
    vid_set_mode(DM_640x480, PM_RGB565);
    vid_clear(0, 0, 0);
}

/*----------------------
 | dc_print
 | Description: Draws BIOS-font text at character cell (x, y) on the framebuffer.
 | Author: suinevere
 | Dependencies: bfont_draw_str
 | Globals: vram_s
 | Params: x -- character column, y -- character row, text -- NUL-terminated string
 | Returns: N/A
 ----------------------*/
void dc_print(int x, int y, const char *text)
{
    bfont_draw_str(vram_s + y * 24 * 640 + x * 12, 640, 1, text);
}

/*----------------------
 | dc_log
 | Description: Writes a printf-style line to KallistiOS dbgio, prefixed "LOM: ".
 | Author: suinevere
 | Dependencies: dbglog
 | Globals: N/A
 | Params: format -- printf-style format string, ... -- format arguments
 | Returns: N/A
 ----------------------*/
void dc_log(const char *format, ...)
{
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof buf, format, args);
    va_end(args);
    dbglog(DBG_INFO, "LOM: %s\n", buf);
}

/*----------------------
 | dc_halt
 | Description: Logs "LOM HALT: <message>" and prints it on screen once, then spins forever: on a plain empty loop with interrupts disabled if called from interrupt context, otherwise via thd_sleep(1000).
 | Author: suinevere
 | Dependencies: dbglog, dc_print, thd_sleep, irq_inside_int, irq_disable
 | Globals: N/A
 | Params: message -- halt reason
 | Returns: N/A
 ----------------------*/
void dc_halt(const char *message)
{
    char buf[256];
    snprintf(buf, sizeof buf, "LOM HALT: %s", message);
    dbglog(DBG_INFO, "%s\n", buf);
    dc_print(1, 1, buf);
    if (irq_inside_int()) {
        irq_disable();
        for (;;) {
        }
    }
    for (;;) {
        thd_sleep(1000);
    }
}

/*----------------------
 | dc_time_us
 | Description: Microseconds since boot from the KallistiOS timer, for decode timing.
 | Author: suinevere
 | Dependencies: timer_us_gettime64
 | Globals: N/A
 | Params: N/A
 | Returns: microseconds
 ----------------------*/
unsigned long long dc_time_us(void)
{
    return timer_us_gettime64();
}
