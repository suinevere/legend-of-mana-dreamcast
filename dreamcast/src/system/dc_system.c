/*----------------------
 | dc_system.c
 | Description: Boot-time video setup, BIOS-font text, serial logging and the on-screen/serial halt for the Dreamcast port.
 | Author: suinevere
 | Dependencies: dc_system.h, dc_video_mode.h, log_ring.h, dc/video.h, dc/biosfont.h, dc/maple.h, dc/maple/controller.h, dc/flashrom.h, kos/dbglog.h, kos/thread.h, arch/irq.h, arch/timer.h, string.h, pad_script.h (LOM_PAD_SCRIPT)
 | Globals: s_video_hz
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <arch/irq.h>
#include <dc/biosfont.h>
#include <dc/video.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/flashrom.h>
#include <kos/dbglog.h>
#include <kos/dbgio.h>
#include <kos/thread.h>
#include <arch/timer.h>
#include "dc_system.h"
#include "dc_video_mode.h"
#include "log_ring.h"

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
 | SCIF_FTDR / SCIF_FSR / SCIF_FDR / SCIF_TX_FIFO
 | Description: The serial port's transmit data, status and FIFO-count registers, and the transmit FIFO's depth.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SCIF_FTDR (*(volatile uint8_t *)0xFFE8000C)
#define SCIF_FSR (*(volatile uint16_t *)0xFFE80010)
#define SCIF_FDR (*(volatile uint16_t *)0xFFE8001C)
#define SCIF_TX_FIFO 16u

/*----------------------
 | s_log_async
 | Description: Whether dbgio is the serial port so log lines go through the queued ring (-1 until the first log decides).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_log_async = -1;

/*----------------------
 | scif_tx_room
 | Description: Free slots in the serial transmit FIFO.
 | Author: suinevere
 | Dependencies: SCIF_FDR, SCIF_TX_FIFO
 | Globals: N/A
 | Params: N/A
 | Returns: free slots, 0 to SCIF_TX_FIFO
 ----------------------*/
static unsigned scif_tx_room(void)
{
    unsigned used = (SCIF_FDR >> 8) & 0x1Fu;

    return used < SCIF_TX_FIFO ? SCIF_TX_FIFO - used : 0u;
}

/*----------------------
 | scif_tx_put
 | Description: Writes one byte to the serial transmit FIFO and clears its empty flags.
 | Author: suinevere
 | Dependencies: SCIF_FTDR, SCIF_FSR
 | Globals: N/A
 | Params: c -- byte to send
 | Returns: N/A
 ----------------------*/
static void scif_tx_put(uint8_t c)
{
    SCIF_FTDR = c;
    SCIF_FSR &= 0xFF9Fu;
}

/*----------------------
 | s_scif_port
 | Description: The console's serial port as the log ring's port.
 | Author: suinevere
 | Dependencies: scif_tx_room, scif_tx_put, timer_us_gettime64, irq_disable, irq_restore
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const LogPort s_scif_port = { scif_tx_room, scif_tx_put, timer_us_gettime64, irq_disable, irq_restore };

/*----------------------
 | dc_log
 | Description: Queues a printf-style line, prefixed "LOM: ", for the serial port and feeds it what the FIFO takes now; a blocking serial write of a probe line had cost the world map a frame every second.
 |              Goes straight to dbglog when dbgio is not the serial port, and in the test build, whose Flycast screenshots are timed by when each capture line reaches the host.
 | Author: suinevere
 | Dependencies: dbgio_dev_get, dbglog, log_ring_bind, log_ring_push
 | Globals: s_log_async, s_scif_port
 | Params: format -- printf-style format string, ... -- format arguments
 | Returns: N/A
 ----------------------*/
/*----------------------
 | RECENT_LINES / RECENT_BYTES / s_recent / s_recent_next
 | Description: The last log lines kept in memory whatever the serial port does, oldest at s_recent_next, for the halt screen.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RECENT_LINES 2
#define RECENT_BYTES 89
static char s_recent[RECENT_LINES][RECENT_BYTES];
static unsigned s_recent_next;

void dc_log(const char *format, ...)
{
    char buf[256];
    va_list args;
    int len;

    va_start(args, format);
    len = snprintf(buf, sizeof buf, "LOM: ");
    vsnprintf(buf + len, sizeof buf - (size_t)len - 1u, format, args);
    va_end(args);
    strncpy(s_recent[s_recent_next], buf + len, RECENT_BYTES - 1);
    s_recent[s_recent_next][RECENT_BYTES - 1] = 0;
    s_recent_next = (s_recent_next + 1u) % RECENT_LINES;
    if (s_log_async < 0)
    {
        const char *dev = dbgio_dev_get();

        s_log_async = dev != NULL && strcmp(dev, "scif") == 0;
#ifdef LOM_PAD_SCRIPT
        s_log_async = 0;
#endif
        if (s_log_async)
        {
            log_ring_bind(&s_scif_port);
        }
    }
    if (!s_log_async)
    {
        dbglog(DBG_INFO, "%s\n", buf);
        return;
    }
    len = (int)strlen(buf);
    buf[len++] = '\n';
    log_ring_push(buf, (unsigned)len);
}

/*----------------------
 | HALT_COLUMN / HALT_ROW / HALT_COLUMNS / HALT_ROWS
 | Description: Where the halt text starts and how much of it shows, kept inside a TV's visible area.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define HALT_COLUMN 4
#define HALT_ROW 2
#define HALT_COLUMNS 44
#define HALT_ROWS 16

/*----------------------
 | dc_halt
 | Description: Logs "LOM HALT: <message>" followed by the last log lines and prints it on screen once, split at newlines and wrapped inside the visible area, then spins forever: on a plain empty loop with interrupts disabled if called from interrupt context, otherwise via thd_sleep(1000).
 | Author: suinevere
 | Dependencies: log_ring_flush, dbglog, dc_print, memcpy, thd_sleep, irq_inside_int, irq_disable
 | Globals: s_recent, s_recent_next
 | Params: message -- halt reason
 | Returns: N/A
 ----------------------*/
void dc_halt(const char *message)
{
    static char buf[512];
    char line[HALT_COLUMNS + 1];
    const char *at;
    int row = HALT_ROW;

    snprintf(buf, sizeof buf, "LOM HALT: %s", message);
    for (row = 0; row < RECENT_LINES; row++)
    {
        const char *recent = s_recent[(s_recent_next + (unsigned)row) % RECENT_LINES];

        if (recent[0] != 0)
            snprintf(buf + strlen(buf), sizeof buf - strlen(buf), "\nLOG %s", recent);
    }
    row = HALT_ROW;
    log_ring_flush();
    dbglog(DBG_INFO, "%s\n", buf);
    for (at = buf; *at != 0 && row < HALT_ROW + HALT_ROWS; row++)
    {
        int n = 0;

        while (at[n] != 0 && at[n] != '\n' && n < HALT_COLUMNS)
            n++;
        memcpy(line, at, (size_t)n);
        line[n] = 0;
        dc_print(HALT_COLUMN, row, line);
        at += n;
        if (*at == '\n')
            at++;
    }
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

void dc_yield(void)
{
    log_ring_drain();
    thd_pass();
}
