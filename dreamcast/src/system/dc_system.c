/*----------------------
 | dc_system.c
 | Description: Boot-time video setup, BIOS-font text, serial logging and the on-screen/serial halt for the Dreamcast port.
 | Author: suinevere
 | Dependencies: dc_system.h, dc_video_mode.h, dc/video.h, dc/biosfont.h, dc/maple.h, dc/maple/controller.h, dc/flashrom.h, kos/dbglog.h, kos/thread.h, arch/irq.h, arch/timer.h, pad_script.h (LOM_PAD_SCRIPT)
 | Globals: s_video_hz
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdarg.h>
#include <stdio.h>
#include <arch/irq.h>
#include <dc/biosfont.h>
#include <dc/video.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/flashrom.h>
#include <kos/dbglog.h>
#include <kos/thread.h>
#include <arch/timer.h>
#include "dc_system.h"
#include "dc_video_mode.h"

#ifdef LOM_PAD_SCRIPT
#include "pad_script.h"
#endif

/*----------------------
 | s_video_hz
 | Description: The refresh rate dc_system_init chose, returned by dc_system_video_hz.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_video_hz = 60;

/*----------------------
 | dc_system_boot_held
 | Description: Whether the boot-time 50 Hz request is active: controller 0's A button under real input, or pad_script_boot_hold() under LOM_PAD_SCRIPT.
 | Author: suinevere
 | Dependencies: maple_enum_type, maple_dev_status, pad_script_boot_hold (LOM_PAD_SCRIPT)
 | Globals: N/A
 | Params: N/A
 | Returns: 1 to request 50 Hz, else 0
 ----------------------*/
static int dc_system_boot_held(void)
{
#ifdef LOM_PAD_SCRIPT
    return pad_script_boot_hold();
#else
    maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    cont_state_t *state;

    if (dev == NULL)
    {
        return 0;
    }
    state = (cont_state_t *)maple_dev_status(dev);
    return state != NULL && (state->buttons & CONT_A) != 0;
#endif
}

/*----------------------
 | dc_system_init
 | Description: Chooses VGA/NTSC/PAL 640x480 RGB565 video from the cable, region and a held A button (or pad_script_boot_hold under LOM_PAD_SCRIPT), sets it, clears the framebuffer and logs the refresh, ready for boot output.
 | Author: suinevere
 | Dependencies: vid_waitvbl, dc_system_boot_held, flashrom_get_region, vid_check_cable, dc_video_choose, vid_set_mode, vid_clear, dc_log
 | Globals: s_video_hz
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void dc_system_init(void)
{
    DcVideoChoice choice;
    int held;
    int i;

    for (i = 0; i < 2; i++)
    {
        vid_waitvbl();
    }
    held = dc_system_boot_held();
    choice = dc_video_choose(vid_check_cable(), flashrom_get_region(), held);
    switch (choice.mode)
    {
    case DC_VIDEO_VGA:
        vid_set_mode(DM_640x480_VGA, PM_RGB565);
        break;
    case DC_VIDEO_PAL:
        vid_set_mode(DM_640x480_PAL_IL, PM_RGB565);
        break;
    default:
        vid_set_mode(DM_640x480_NTSC_IL, PM_RGB565);
        break;
    }
    vid_clear(0, 0, 0);
    s_video_hz = choice.hz;
    dc_log("video %d Hz", choice.hz);
}

/*----------------------
 | dc_system_video_hz
 | Description: The refresh rate dc_system_init chose (60 for VGA/NTSC, 50 for PAL).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_video_hz
 | Params: N/A
 | Returns: Hz
 ----------------------*/
int dc_system_video_hz(void)
{
    return s_video_hz;
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
