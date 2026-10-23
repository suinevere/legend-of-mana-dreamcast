/*----------------------
 | game_host.c
 | Description: Runs the structured Ring Ring Land with no R1 code and no emulator, on psk_host.c, under the same scripts as pkrun, writing the same trace format and optional per-frame state and RAM-read dumps.
 | Author: suinevere
 | Dependencies: psk_host.h, psk_hw.h, psk_cpu.h, game.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psk_host.h"
#include "psk_cpu.h"
#include "game.h"

/*----------------------
 | RAM_HIGH
 | Description: End of the work RAM a trace sample hashes (the stack above it is skipped), as pkrun's default --stack-low.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RAM_HIGH 0x700u

/*----------------------
 | SOUND_LO / SOUND_HI
 | Description: The sound driver's playback state, zeroed before hashing as pkrun's default --ram-skip.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SOUND_LO 0x5C4u
#define SOUND_HI 0x5D2u

/*----------------------
 | MAX_STEPS / MAX_BOOTS
 | Description: Limits for an expanded script and for the boots an entry-state file can hold.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MAX_STEPS 200000
#define MAX_BOOTS 8

/*----------------------
 | Step
 | Description: One expanded script step: a sample with a button mask, an RTC setting, or a reboot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    enum { SAMPLE, RTC, REBOOT } kind;
    uint32_t a, b;
} Step;

/*----------------------
 | Entry
 | Description: The work RAM and device words the reference runner dumped at one app entry.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint8_t wram[0x800];
    uint32_t words[64];
    uint32_t n;
} Entry;

/*----------------------
 | g_*
 | Description: The run: script steps, position, buttons, trace bytes, entry states, the state dump and the escape back to main.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static Step g_steps[MAX_STEPS];
static int g_count, g_next;
static uint32_t g_buttons, g_samples;
static uint8_t *g_trace;
static size_t g_len, g_cap;
static Entry g_entries[MAX_BOOTS];
static int g_boots;
static FILE *g_state;
static jmp_buf g_top;

/*----------------------
 | thunk_sound_fiq / thunk_timer_tick
 | Description: Register entries for the Timer 2 sample handler (no arguments) and the Timer 1 tick handler (its line in r0), returning to lr.
 | Author: suinevere
 | Dependencies: game_sound_fiq, game_timer_tick
 | Globals: N/A
 | Params: c -- register file
 | Returns: N/A
 ----------------------*/
static void thunk_sound_fiq(PskCpu *c) {
    game_sound_fiq();
    c->r[15] = c->r[14];
}

static void thunk_timer_tick(PskCpu *c) {
    game_timer_tick(c->r[0]);
    c->r[15] = c->r[14];
}

/*----------------------
 | thunk_dock_irq
 | Description: Register entry for the dock interrupt handler, returning to lr.
 | Author: suinevere
 | Dependencies: game_dock_irq
 | Globals: N/A
 | Params: c -- register file
 | Returns: N/A
 ----------------------*/
static void thunk_dock_irq(PskCpu *c) {
    game_dock_irq();
    c->r[15] = c->r[14];
}

void game_buttons_irq_regs(PskCpu *c);
void game_irq_callback_regs(PskCpu *c);
void game_fiq_callback_regs(PskCpu *c);

/*----------------------
 | psk_hooks / psk_hook_count
 | Description: The only code psk_host enters by address: the kernel's two callbacks and the interrupt slot handlers the game installs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
const PskHookEntry psk_hooks[] = {
    {0x02002B30u, game_fiq_callback_regs, "game_fiq_callback_regs", 1, 0, 0x2000u},
    {0x02002B5Cu, game_irq_callback_regs, "game_irq_callback_regs", 1, 0, 0x2000u},
    {0x02003B64u, thunk_sound_fiq, "game_sound_fiq", 0, 0, 0x1FFFFu},
    {0x02003E70u, thunk_timer_tick, "game_timer_tick", 0, 0, 0x1FFFFu},
    {0x02002F8Cu, game_buttons_irq_regs, "game_buttons_irq_regs", 1, 0, 0x200u},
    {0x020005A0u, thunk_dock_irq, "game_dock_irq", 0, 0, 0x1FFFFu},
};
const int psk_hook_count = sizeof psk_hooks / sizeof psk_hooks[0];

/*----------------------
 | fail
 | Description: Prints a message and exits with status 2.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: what -- message; arg -- detail
 | Returns: does not return
 ----------------------*/
static void fail(const char *what, const char *arg) {
    fprintf(stderr, "game_host: %s %s\n", what, arg);
    exit(2);
}

/*----------------------
 | put
 | Description: Appends bytes to the trace.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_trace, g_len, g_cap
 | Params: p -- bytes; n -- count
 | Returns: N/A
 ----------------------*/
static void put(const void *p, size_t n) {
    if (g_len + n > g_cap) {
        g_cap = (g_len + n) * 2;
        g_trace = realloc(g_trace, g_cap);
        if (!g_trace) fail("out of memory", "");
    }
    memcpy(g_trace + g_len, p, n);
    g_len += n;
}

/*----------------------
 | put32
 | Description: Appends a little-endian word to the trace.
 | Author: suinevere
 | Dependencies: put
 | Globals: N/A
 | Params: v -- word
 | Returns: N/A
 ----------------------*/
static void put32(uint32_t v) {
    uint8_t b[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)};
    put(b, 4);
}

/*----------------------
 | crc32
 | Description: The CRC-32 pkrun hashes RAM and the card with.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- bytes; n -- count
 | Returns: CRC
 ----------------------*/
static uint32_t crc32(const uint8_t *p, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int k = 0; k < 8; k++) c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
    }
    return c ^ 0xFFFFFFFFu;
}

/*----------------------
 | bcd / day_of_week
 | Description: BCD byte of 0..99, and the 1..7 weekday pkrun stores with a script's rtc line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value; y, m, d -- date
 | Returns: BCD byte; weekday
 ----------------------*/
static uint32_t bcd(int v) { return (uint32_t)(((v / 10) << 4) | (v % 10)); }
static int day_of_week(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7 + 1;
}

/*----------------------
 | push
 | Description: Adds one expanded step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_steps, g_count
 | Params: kind, a, b -- the step
 | Returns: N/A
 ----------------------*/
static void push(int kind, uint32_t a, uint32_t b) {
    if (g_count == MAX_STEPS) fail("script too long", "");
    g_steps[g_count].kind = kind;
    g_steps[g_count].a = a;
    g_steps[g_count].b = b;
    g_count++;
}

/*----------------------
 | load_script
 | Description: Parses and expands a .pks script exactly as pkrun does: wait, hold, press, rtc, reboot, end.
 | Author: suinevere
 | Dependencies: push, bcd, day_of_week
 | Globals: g_steps, g_count
 | Params: path -- script
 | Returns: N/A
 ----------------------*/
static void load_script(const char *path) {
    FILE *f = fopen(path, "r");
    char line[256], op[32];
    if (!f) fail("cannot read", path);
    while (fgets(line, sizeof line, f)) {
        char *hash = strchr(line, '#');
        unsigned a = 0, b = 0;
        if (hash) *hash = 0;
        if (sscanf(line, "%31s", op) != 1) continue;
        if (!strcmp(op, "end")) break;
        if (!strcmp(op, "wait") && sscanf(line, "%*s %u", &b) == 1) {
            for (unsigned i = 0; i < b; i++) push(SAMPLE, 0, 0);
        } else if (!strcmp(op, "hold") && sscanf(line, "%*s %x %u", &a, &b) == 2) {
            for (unsigned i = 0; i < b; i++) push(SAMPLE, a, 0);
        } else if (!strcmp(op, "press") && sscanf(line, "%*s %x", &a) == 1) {
            push(SAMPLE, a, 0);
            push(SAMPLE, 0, 0);
        } else if (!strcmp(op, "reboot")) {
            push(REBOOT, 0, 0);
        } else if (!strcmp(op, "rtc")) {
            int y, mo, d, hh, mi, ss;
            if (sscanf(line, "%*s %d-%d-%d %d:%d:%d", &y, &mo, &d, &hh, &mi, &ss) != 6) fail("bad rtc line in", path);
            push(RTC, bcd(d) | (bcd(mo) << 8) | (bcd(y % 100) << 16),
                 bcd(ss) | (bcd(mi) << 8) | (bcd(hh) << 16) | ((uint32_t)day_of_week(y, mo, d) << 24));
        } else {
            fail("bad line in", path);
        }
    }
    fclose(f);
}

/*----------------------
 | load_entries
 | Description: Reads every app-entry record (2 KB RAM, word count, words) from a pkrun --dump-entry-state file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_entries, g_boots
 | Params: path -- entry-state file
 | Returns: N/A
 ----------------------*/
static void load_entries(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) fail("cannot read", path);
    while (g_boots < MAX_BOOTS) {
        Entry *e = &g_entries[g_boots];
        if (fread(e->wram, 1, sizeof e->wram, f) != sizeof e->wram) break;
        if (fread(&e->n, 4, 1, f) != 1 || e->n > 64 || fread(e->words, 4, e->n, f) != e->n) fail("bad entry state", path);
        g_boots++;
    }
    fclose(f);
    if (!g_boots) fail("no entry state in", path);
}

/*----------------------
 | advance
 | Description: Applies the steps up to the next sample (RTC settings; a reboot or the end leave through g_top) and sets its buttons.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_steps, g_next, g_buttons, g_top
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void advance(void) {
    while (g_next < g_count && g_steps[g_next].kind != SAMPLE) {
        const Step *s = &g_steps[g_next++];
        if (s->kind == RTC) {
            psk_host_set_rtc(s->a, s->b);
        } else {
            longjmp(g_top, 2);
        }
    }
    if (g_next >= g_count) longjmp(g_top, 1);
    g_buttons = g_steps[g_next].a;
    psk_host_set_buttons(g_buttons);
}

/*----------------------
 | frame
 | Description: The frame point: one trace sample (index, buttons, LCD, RAM hash), the frame's state when dumping, then the next step.
 | Author: suinevere
 | Dependencies: put, put32, crc32, advance
 | Globals: g_samples, g_buttons, g_next, g_state
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void frame(void) {
    uint8_t ram[RAM_HIGH - 0x200u];
    memcpy(ram, psk_host_ram() + 0x200, sizeof ram);
    memset(ram + (SOUND_LO - 0x200u), 0, SOUND_HI - SOUND_LO);
    put32(g_samples);
    put32(g_buttons);
    put(psk_host_vram(), 128);
    put32(crc32(ram, sizeof ram));
    if (g_state && (fwrite(psk_host_ram() + 0x200, 1, 0x600, g_state) != 0x600 || fwrite(psk_host_vram(), 1, 128, g_state) != 128)) {
        fail("cannot write the state", "");
    }
    g_samples++;
    g_next++;
    advance();
}

/*----------------------
 | boot
 | Description: Starts boot k from its entry state, keeping the card, and runs the game's entry until a step leaves.
 | Author: suinevere
 | Dependencies: game_entry
 | Globals: g_entries, g_boots
 | Params: k -- boot index
 | Returns: does not return
 ----------------------*/
static void boot(int k) {
    static uint8_t card[0x20000];
    if (k >= g_boots) fail("no entry state for a reboot", "");
    memcpy(card, psk_host_card(), sizeof card);
    psk_host_init(card, g_entries[k].wram);
    psk_host_load_entry(g_entries[k].wram, g_entries[k].words, g_entries[k].n);
    psk_host_set_frame(frame);
    advance();
    game_entry();
    fail("the game returned", "");
}

/*----------------------
 | main
 | Description: game_host --card C --script S --entry E --trace OUT [--state OUT.state] [--reads OUT.bin].
 | Author: suinevere
 | Dependencies: load_script, load_entries, boot, psk_host_read_map (GAME_TRACE_READS)
 | Globals: g_trace, g_len, g_state
 | Params: argc, argv -- command line
 | Returns: 0, or 2 on a usage or input error
 ----------------------*/
int main(int argc, char **argv) {
    const char *card = 0, *script = 0, *entry = 0, *out = 0, *state = 0, *reads = 0;
    static uint8_t image[0x20000];
    volatile int k = 0;
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "--card")) card = argv[i + 1];
        else if (!strcmp(argv[i], "--script")) script = argv[i + 1];
        else if (!strcmp(argv[i], "--entry")) entry = argv[i + 1];
        else if (!strcmp(argv[i], "--trace")) out = argv[i + 1];
        else if (!strcmp(argv[i], "--state")) state = argv[i + 1];
        else if (!strcmp(argv[i], "--reads")) reads = argv[i + 1];
        else fail("unknown option", argv[i]);
    }
    if (!card || !script || !entry || !out) fail("usage:", "game_host --card C --script S --entry E --trace OUT [--state F] [--reads F]");
    FILE *f = fopen(card, "rb");
    if (!f || fread(image, 1, sizeof image, f) != sizeof image) fail("cannot read a 128 KB card from", card);
    fclose(f);
    if (state && !(g_state = fopen(state, "wb"))) fail("cannot write", state);
    load_script(script);
    load_entries(entry);
    while (g_next < g_count && g_steps[g_next].kind == RTC) g_next++;
    psk_host_init(image, g_entries[0].wram);
    put("PKTR1", 5);
    int how = setjmp(g_top);
    if (how == 2) k++;
    if (how != 1) boot(k);
    put32(psk_host_card_crc());
    f = fopen(out, "wb");
    if (!f || fwrite(g_trace, 1, g_len, f) != g_len) fail("cannot write", out);
    fclose(f);
    if (g_state) fclose(g_state);
#ifdef GAME_TRACE_READS
    if (reads) {
        f = fopen(reads, "wb");
        if (!f || fwrite(psk_host_read_map, 1, 0x800, f) != 0x800) fail("cannot write", reads);
        fclose(f);
    }
#else
    if (reads) fail("--reads needs a GAME_TRACE_READS build", "");
#endif
    printf("game_host: samples %u boots %d\n", g_samples, k + 1);
    return 0;
}
