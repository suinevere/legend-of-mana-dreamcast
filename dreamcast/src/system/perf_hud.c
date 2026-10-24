/*----------------------
 | perf_hud.c
 | Description: Once-a-second speed readout on the first VMU's screen in a 4x6 font, four lines: V vblanks per frame and I idle ms; O ordering-table submit ms, P PowerVR wait ms and E expired flip waits; T texture ms, C CD stalls and R longest PowerVR render ms; M movie decode average and worst ms.
 |              A thread of its own draws it, since vmu_draw_lcd blocks until the next vblank's maple cycle and cost the game a frame each second.
 | Author: suinevere
 | Dependencies: perf_hud.h, libpress_dc.h, cd_stream.h, dc/vmu_fb.h, dc/maple.h, kos/sem.h, kos/thread.h, stdio.h, string.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include <dc/maple.h>
#include <dc/vmu_fb.h>
#include <kos/sem.h>
#include <kos/thread.h>

#include "cd_stream.h"
#include "libpress_dc.h"
#include "perf_hud.h"

/*----------------------
 | GLYPHS
 | Description: The readout's characters, each followed by its six 4-pixel rows (most significant bit leftmost).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const char GLYPHS[] =
    "0\xE\xA\xA\xA\xE\x0" "1\x4\xC\x4\x4\xE\x0" "2\xE\x2\xE\x8\xE\x0" "3\xE\x2\xE\x2\xE\x0" "4\xA\xA\xE\x2\x2\x0"
    "5\xE\x8\xE\x2\xE\x0" "6\xE\x8\xE\xA\xE\x0" "7\xE\x2\x2\x4\x4\x0" "8\xE\xA\xE\xA\xE\x0" "9\xE\xA\xE\x2\xE\x0"
    ".\x0\x0\x0\x0\x4\x0" "-\x0\x0\xE\x0\x0\x0" "A\x4\xA\xE\xA\xA\x0" "B\xC\xA\xC\xA\xC\x0" "C\xE\x8\x8\x8\xE\x0"
    "D\xC\xA\xA\xA\xC\x0" "E\xE\x8\xC\x8\xE\x0" "I\xE\x4\x4\x4\xE\x0" "P\xC\xA\xC\x8\x8\x0" "R\xC\xA\xC\xA\xA\x0""L\x8\x8\x8\x8\xE\x0" "M\xA\xE\xE\xA\xA\x0"
    "O\xE\xA\xA\xA\xE\x0" "T\xE\x4\x4\x4\x4\x0" "V\xA\xA\xA\xA\x4\x0" "X\xA\xA\x4\xA\xA\x0";

/*----------------------
 | GLYPH_ROWS / FONT_STRIDE
 | Description: Rows per glyph, and bytes per character in the vmufb font (six 4-bit rows).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GLYPH_ROWS 6
#define FONT_STRIDE 3

/*----------------------
 | s_font_data / s_font / s_font_ready
 | Description: The 128-character font built from GLYPHS on first use (unlisted characters stay blank), its vmufb description, and whether it is built.
 | Author: suinevere
 | Dependencies: FONT_STRIDE
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static char s_font_data[128 * FONT_STRIDE];
static const vmufb_font_t s_font = {4, GLYPH_ROWS, FONT_STRIDE, s_font_data};
static int s_font_ready;

/*----------------------
 | build_font
 | Description: Packs each GLYPHS entry's rows two to a byte at its character's slot.
 | Author: suinevere
 | Dependencies: GLYPHS
 | Globals: s_font_data, s_font_ready
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void build_font(void)
{
    const char *glyph;

    for (glyph = GLYPHS; *glyph != 0; glyph += 1 + GLYPH_ROWS)
    {
        char *slot = &s_font_data[(unsigned char)glyph[0] * FONT_STRIDE];
        int row;

        for (row = 0; row < GLYPH_ROWS; row += 2)
            slot[row / 2] = (char)((glyph[1 + row] << 4) | glyph[2 + row]);
    }
    s_font_ready = 1;
}

/*----------------------
 | HUD_TEXT_BYTES / HUD_STACK_BYTES
 | Description: Size of one readout's text, and the drawing thread's stack.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define HUD_TEXT_BYTES 80
#define HUD_STACK_BYTES 8192

/*----------------------
 | s_text / s_pending / s_wake / s_started
 | Description: The readout waiting to be drawn, whether the thread has yet to take it, the semaphore that wakes the thread, and whether it was started.
 | Author: suinevere
 | Dependencies: HUD_TEXT_BYTES
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static char s_text[HUD_TEXT_BYTES];
static volatile int s_pending;
static semaphore_t s_wake;
static int s_started;

/*----------------------
 | hud_thread
 | Description: Draws each readout the game hands over on the first VMU found, blocking here instead of on the game thread.
 | Author: suinevere
 | Dependencies: build_font, vmufb_print_string, vmufb_present, maple_enum_type, sem_wait
 | Globals: s_text, s_pending, s_wake, s_font, s_font_ready
 | Params: arg -- unused
 | Returns: never
 ----------------------*/
static void *hud_thread(void *arg)
{
    char text[HUD_TEXT_BYTES];
    vmufb_t fb;

    (void)arg;
    for (;;)
    {
        maple_device_t *vmu;

        sem_wait(&s_wake);
        memcpy(text, s_text, sizeof(text));
        s_pending = 0;
        vmu = maple_enum_type(0, MAPLE_FUNC_LCD);
        if (vmu == NULL)
            continue;
        if (!s_font_ready)
            build_font();
        memset(&fb, 0, sizeof(fb));
        vmufb_print_string(&fb, &s_font, text);
        vmufb_present(&fb, vmu);
    }
    return NULL;
}

void perf_hud_report(unsigned vbl_hundredths, unsigned idle_ms, unsigned texture_ms, unsigned otag_ms, unsigned pvr_ms, unsigned render_ms, unsigned flip_expired)
{
    unsigned frames = 0;
    unsigned us_avg = 0;
    unsigned us_max = 0;

#ifndef LOM_PAD_SCRIPT
    libpress_dc_stats(&frames, &us_avg, &us_max);
#endif
    if (!s_started)
    {
        kthread_attr_t attr = {0};

        s_started = 1;
        sem_init(&s_wake, 0);
        attr.create_detached = true;
        attr.stack_size = HUD_STACK_BYTES;
        attr.prio = PRIO_DEFAULT - 1;
        attr.label = "perf_hud";
        if (thd_create_ex(&attr, hud_thread, NULL) == NULL)
            return;
    }
    if (s_pending)
        return;
    snprintf(s_text, sizeof(s_text), "V%u.%02u I%u\nO%u P%u E%u\nT%u C%u R%u\n", vbl_hundredths / 100u, vbl_hundredths % 100u, idle_ms, otag_ms, pvr_ms, flip_expired,
             texture_ms, cd_stream_stalls(), render_ms);
    if (frames != 0)
        snprintf(s_text + strlen(s_text), sizeof(s_text) - strlen(s_text), "M%u %u", us_avg / 1000u, us_max / 1000u);
    else
        snprintf(s_text + strlen(s_text), sizeof(s_text) - strlen(s_text), "M-");
    s_pending = 1;
    sem_signal(&s_wake);
}
