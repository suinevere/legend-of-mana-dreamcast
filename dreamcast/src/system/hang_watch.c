/*----------------------
 | hang_watch.c
 | Description: KallistiOS side of the hang check: a vblank handler halts with the interrupted and main threads' PC and PR once no drawn frame has closed for HANG_SECONDS, which also catches a black screen the game keeps calling VSync under, and KallistiOS assertions halt on screen instead of panicking to a black one.
 | Author: suinevere
 | Dependencies: hang_watch.h, dc_system.h, vsync_pacer.h, dc/vblank.h, arch/irq.h, kos/thread.h, assert.h, stdio.h, string.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <arch/irq.h>
#include <dc/vblank.h>
#include <kos/thread.h>

#include "dc_system.h"
#include "hang_watch.h"
#include "vsync_pacer.h"

/*----------------------
 | HANG_SECONDS
 | Description: Seconds without a frame that drew anything that count as a hang; well past any load, memory card write or black pause.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define HANG_SECONDS 20u

/*----------------------
 | s_main / s_limit / s_fed / s_armed / s_fired
 | Description: The main thread, the vblank limit, the count at the last VSync, whether VSync has run yet, and whether the halt already fired.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static kthread_t *s_main;
static unsigned s_limit;
static volatile unsigned s_fed;
static volatile int s_armed;
static int s_fired;

void hang_watch_feed(void)
{
    s_fed = vsync_count();
    s_armed = 1;
}

/*----------------------
 | hang_vblank
 | Description: Vblank handler: once overdue, formats where the interrupted thread and the main thread are and halts.
 | Author: suinevere
 | Dependencies: hang_watch_overdue, hang_watch_format, irq_get_context, thd_get_current, dc_halt
 | Globals: s_main, s_limit, s_fed, s_armed, s_fired
 | Params: code, data -- unused
 | Returns: N/A
 ----------------------*/
static void hang_vblank(uint32_t code, void *data)
{
    static char message[200];
    unsigned now = vsync_count();
    irq_context_t *context;
    kthread_t *current;
    unsigned long main_pc;
    unsigned long main_pr;

    (void)code;
    (void)data;
    if (s_fired || !hang_watch_overdue(now, s_fed, s_limit, s_armed))
    {
        return;
    }
    s_fired = 1;
    context = irq_get_context();
    current = thd_get_current();
    main_pc = context->pc;
    main_pr = context->pr;
    if (s_main != NULL && current != s_main)
    {
        main_pc = s_main->context.pc;
        main_pr = s_main->context.pr;
    }
    hang_watch_format(message, sizeof message, current != NULL ? current->label : "?", context->pc, context->pr, main_pc, main_pr, now - s_fed);
    snprintf(message + strlen(message), sizeof message - strlen(message), "\nBUILD " __DATE__ " " __TIME__);
    dc_halt(message);
}

/*----------------------
 | assert_halt
 | Description: KallistiOS assert handler: halts naming the failed expression and where.
 | Author: suinevere
 | Dependencies: dc_halt
 | Globals: N/A
 | Params: file, line, expr, msg, func -- the failed assertion
 | Returns: never
 ----------------------*/
static void assert_halt(const char *file, int line, const char *expr, const char *msg, const char *func)
{
    static char message[240];

    snprintf(message, sizeof message, "ASSERT %s:%d\n%s\n%s %s\nBUILD " __DATE__ " " __TIME__, file != NULL ? file : "?", line, func != NULL ? func : "?",
             expr != NULL ? expr : "", msg != NULL ? msg : "");
    dc_halt(message);
}

void hang_watch_install(unsigned video_hz)
{
    s_main = thd_get_current();
    s_limit = HANG_SECONDS * video_hz;
    assert_set_handler(assert_halt);
    vblank_handler_add(hang_vblank, NULL);
}
