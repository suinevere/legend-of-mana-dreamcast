/*----------------------
 | vmu_host.c
 | Description: vmu_hw.h on the host: RAM, the frame buffer, the game's file, and interrupts, timers, the clock and the RTC edge scheduled exactly as psk_host.c schedules the PocketStation's.
 | Author: suinevere
 | Dependencies: vmu_host.h, vmu_hw.h
 | Globals: vmu_ram
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "vmu_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------
 | FIQ_LINES
 | Description: Lines delivered to the FIQ handler (COM and Timer 2); the others go to the IRQ handler.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FIQ_LINES (VMU_LINE_COM | VMU_LINE_TIMER2)

/*----------------------
 | IRQ_LINES
 | Description: Lines the IRQ handler takes: buttons, timers 0 and 1, RTC, dock, link and line 12.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define IRQ_LINES 0x1F9Fu

/*----------------------
 | LEVEL_LINES
 | Description: Lines whose request level follows a signal, which an ack leaves alone: buttons, COM, RTC and link.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LEVEL_LINES (VMU_LINE_BUTTONS | VMU_LINE_COM | VMU_LINE_RTC | VMU_LINE_LINK)

/*----------------------
 | LOG_CAP
 | Description: Entries the peripheral write log keeps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOG_CAP 65536u

/*----------------------
 | IO_REGS
 | Description: Number of VMU_IO_* registers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define IO_REGS 9u

/*----------------------
 | VmuTimer
 | Description: One timer channel: control (bit 2 runs, bits 0-1 divider code), count, reload and prescaler.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint32_t mode;
    uint32_t counter;
    uint32_t reload;
    uint32_t prescale;
} VmuTimer;

/*----------------------
 | Host
 | Description: The whole device state but RAM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint8_t fb[VMU_FB_BYTES];
    uint8_t file[0x10000];
    uint32_t file_len;
    uint32_t irq_mask, irq_flags, irq_latch;
    uint8_t buttons, sys;
    uint32_t clk, rtc_mode, rtc_countdown;
    VmuTimer timers[3];
    uint16_t io[IO_REGS];
    uint64_t events, cycles;
    int ctx_irq, ctx_fiq, gate;
    void (*irq)(void);
    void (*fiq)(void);
    void (*frame)(void);
    uint32_t log_n;
    VmuIoWrite log[LOG_CAP];
} Host;

static Host h;

uint8_t vmu_ram[0x400];

/*----------------------
 | die
 | Description: Stops the host on something the model cannot do.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: what -- message; v -- value involved
 | Returns: does not return
 ----------------------*/
static void die(const char *what, uint32_t v) {
    fprintf(stderr, "vmu_host: %s %08X\n", what, (unsigned)v);
    exit(7);
}

/*----------------------
 | clock_hz
 | Description: CPU clock for a speed, as psk_host maps the PocketStation's.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: Hz
 ----------------------*/
static uint32_t clock_hz(void) {
    static const uint32_t hz[9] = {32768, 63488, 126976, 253952, 507904, 1015808, 1998848, 3997696, 7995392};
    return hz[h.clk > 8u ? 8u : h.clk];
}

/*----------------------
 | write_mode
 | Description: A timer control write: restarts the prescaler and loads the count when the timer starts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: t -- timer; v -- control
 | Returns: N/A
 ----------------------*/
static void write_mode(VmuTimer *t, uint32_t v) {
    uint32_t was = t->mode & 4u;
    t->mode = v;
    t->prescale = 0;
    if (!was && (v & 4u)) t->counter = t->reload;
}

/*----------------------
 | request
 | Description: Raises a line's request level and latches it if enabled.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: m -- line bit
 | Returns: N/A
 ----------------------*/
static void request(uint32_t m) {
    if (!(h.irq_flags & m)) {
        h.irq_flags |= m;
        h.irq_latch |= m & h.irq_mask;
    }
}

/*----------------------
 | TIMER_DIV
 | Description: Clock cycles per timer step for each divider code.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const uint32_t TIMER_DIV[4] = {2, 32, 512, 2};

/*----------------------
 | timer_first
 | Description: Cycles until a running timer's next step: the prescaler counts one a cycle and steps the timer when it reaches the divider.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: t -- running timer
 | Returns: cycles, at least 1
 ----------------------*/
static uint32_t timer_first(const VmuTimer *t) {
    uint32_t div = TIMER_DIV[t->mode & 3u];
    return t->prescale + 1u >= div ? 1u : div - t->prescale;
}

/*----------------------
 | timer_due
 | Description: Cycles until a timer expires: its next step, then one step per count left, expiring on the step that finds the count at zero.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: t -- timer
 | Returns: cycles, or 0 for a stopped timer, which never expires
 ----------------------*/
static uint64_t timer_due(const VmuTimer *t) {
    if (!(t->mode & 4u) || t->reload == 0) return 0;
    return timer_first(t) + (uint64_t)t->counter * TIMER_DIV[t->mode & 3u];
}

/*----------------------
 | timer_skip
 | Description: Runs a timer for k cycles, k no more than its timer_due: prescaler and count follow, and an expiry on the last cycle reloads it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: t -- timer; k -- cycles
 | Returns: 1 when it expired
 ----------------------*/
static int timer_skip(VmuTimer *t, uint64_t k) {
    uint32_t div, first, steps;
    if (!(t->mode & 4u) || t->reload == 0) return 0;
    div = TIMER_DIV[t->mode & 3u];
    first = timer_first(t);
    if (k < first) {
        t->prescale += (uint32_t)k;
        return 0;
    }
    steps = 1u + (uint32_t)((k - first) / div);
    t->prescale = (uint32_t)((k - first) % div);
    if (steps == t->counter + 1u) {
        t->counter = t->reload;
        return 1;
    }
    t->counter -= steps;
    return 0;
}

/*----------------------
 | rtc_edge
 | Description: An RTC square-wave edge: the next edge's period is taken from the clock speed, each edge is an event, and a rising edge requests the RTC line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void rtc_edge(void) {
    h.rtc_countdown = clock_hz() / (2u * ((h.rtc_mode & 1u) ? 4096u : 1u));
    h.events++;
    if (h.irq_flags & VMU_LINE_RTC) {
        h.irq_flags &= ~VMU_LINE_RTC;
        return;
    }
    request(VMU_LINE_RTC);
}

/*----------------------
 | advance_to_event
 | Description: Runs the devices straight to exactly the next timer expiry or RTC edge, as psk_host's cycle-by-cycle loop reaches it, latching each timer that expires then.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void advance_to_event(void) {
    static const uint32_t line[3] = {VMU_LINE_TIMER0, VMU_LINE_TIMER1, VMU_LINE_TIMER2};
    uint64_t k = h.rtc_countdown ? h.rtc_countdown : 0x100000000ull;
    for (int i = 0; i < 3; i++) {
        uint64_t due = timer_due(&h.timers[i]);
        if (due && due < k) k = due;
    }
    h.rtc_countdown = (uint32_t)(h.rtc_countdown - k);
    if (h.rtc_countdown == 0) rtc_edge();
    for (int i = 0; i < 3; i++) {
        if (timer_skip(&h.timers[i], k)) {
            h.irq_latch |= line[i];
            h.events++;
        }
    }
    h.cycles += k;
}

/*----------------------
 | deliverable
 | Description: Latched, enabled lines the current context would take: FIQ lines outside the FIQ handler, IRQ lines outside both.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: line bits
 ----------------------*/
static uint32_t deliverable(void) {
    return h.irq_latch & h.irq_mask & ((h.ctx_fiq ? 0u : FIQ_LINES) | (h.ctx_irq ? 0u : IRQ_LINES));
}

/*----------------------
 | ack
 | Description: Clears latched lines and their non-level requests.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: v -- line bits
 | Returns: N/A
 ----------------------*/
static void ack(uint32_t v) {
    h.irq_flags &= ~v | LEVEL_LINES;
    h.irq_latch &= ~v;
}

/*----------------------
 | take_one
 | Description: Takes the highest-priority deliverable line as the PocketStation launcher does: COM is dropped and Timer 2 goes to the FIQ handler, or with no FIQ handler is acknowledged; otherwise RTC and link are acknowledged and the IRQ handler runs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void take_one(void) {
    uint32_t pending = h.irq_latch & h.irq_mask;
    int irq = h.ctx_irq, fiq = h.ctx_fiq;
    if (!fiq && (pending & FIQ_LINES)) {
        uint32_t rest = pending & ~VMU_LINE_COM;
        if (pending & VMU_LINE_COM) h.irq_latch &= ~VMU_LINE_COM;
        h.ctx_irq = h.ctx_fiq = 1;
        if (rest) {
            if (h.fiq) {
                h.fiq();
            } else {
                ack(rest);
            }
        }
    } else if (!irq && (pending & IRQ_LINES)) {
        h.ctx_irq = 1;
        ack(VMU_LINE_RTC | VMU_LINE_LINK);
        if (h.irq) h.irq();
    }
    h.ctx_irq = irq;
    h.ctx_fiq = fiq;
}

/*----------------------
 | timer_at
 | Description: A timer by number.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: which -- 0-2
 | Returns: the timer
 ----------------------*/
static VmuTimer *timer_at(uint8_t which) {
    if (which > 2u) die("no timer", which);
    return &h.timers[which];
}

#ifdef VMU_COUNT_ACCESS
static uint32_t g_counts[0x400];

/*----------------------
 | vmu_host_counts
 | Description: Reads and writes of each vmu_ram offset since start; only in VMU_COUNT_ACCESS builds.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_counts
 | Params: N/A
 | Returns: 0x400 counts
 ----------------------*/
const uint32_t *vmu_host_counts(void) { return g_counts; }
#endif

/*----------------------
 | vmu_r8 / vmu_w8
 | Description: Reads or writes a byte of vmu_ram, stopping on an offset past its end, and counting the access in VMU_COUNT_ACCESS builds.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: vmu_ram, g_counts
 | Params: off -- RAM offset; v -- value
 | Returns: the byte (vmu_r8)
 ----------------------*/
uint8_t vmu_r8(uint16_t off) {
    if (off >= sizeof vmu_ram) die("RAM read past the end", off);
#ifdef VMU_COUNT_ACCESS
    g_counts[off]++;
#endif
    return vmu_ram[off];
}

void vmu_w8(uint16_t off, uint8_t v) {
    if (off >= sizeof vmu_ram) die("RAM write past the end", off);
#ifdef VMU_COUNT_ACCESS
    g_counts[off]++;
#endif
    vmu_ram[off] = v;
}

/*----------------------
 | vmu_buttons / vmu_fb / vmu_present
 | Description: The buttons held, the frame buffer, and the end of a frame, which calls the host's frame callback.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: button bits; the frame buffer
 ----------------------*/
uint8_t vmu_buttons(void) { return h.buttons; }

uint8_t *vmu_fb(void) { return h.fb; }

void vmu_present(void) {
    if (h.frame) h.frame();
}

/*----------------------
 | vmu_idle
 | Description: The idle-time schedule: advances the devices to their next event, then takes lines until the context it idled from has none deliverable or a nested idle closed the gate.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void vmu_idle(void) {
    advance_to_event();
    if (!deliverable()) return;
    h.gate = 1;
    for (int guard = 0; h.gate; guard++) {
        if (!deliverable()) {
            h.gate = 0;
            break;
        }
        if (guard == 64) die("interrupt storm, latch", h.irq_latch);
        take_one();
    }
}

/*----------------------
 | vmu_irq_enable / vmu_irq_disable / vmu_irq_ack / vmu_irq_pending / vmu_irq_levels / vmu_irq_mask
 | Description: The interrupt controller: mask set and clear, acknowledge, and the latched, level and enabled lines.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: lines -- VMU_LINE_* bits
 | Returns: line bits (the reads)
 ----------------------*/
void vmu_irq_enable(uint16_t lines) { h.irq_mask |= lines; }

void vmu_irq_disable(uint16_t lines) { h.irq_mask &= ~(uint32_t)lines; }

void vmu_irq_ack(uint16_t lines) { ack(lines); }

uint16_t vmu_irq_pending(void) { return (uint16_t)h.irq_latch; }

uint16_t vmu_irq_levels(void) { return (uint16_t)h.irq_flags; }

uint16_t vmu_irq_mask(void) { return (uint16_t)h.irq_mask; }

/*----------------------
 | vmu_set_handlers
 | Description: Installs the IRQ and FIQ handlers take_one calls.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: irq -- IRQ handler; fiq -- FIQ handler
 | Returns: N/A
 ----------------------*/
void vmu_set_handlers(void (*irq)(void), void (*fiq)(void)) {
    h.irq = irq;
    h.fiq = fiq;
}

/*----------------------
 | vmu_set_timer / vmu_timer_run
 | Description: Sets a timer's reload, divider code and run bit, or only its run bit, through a control write.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: which -- timer 0-2; reload -- count; prescale -- divider code; run -- nonzero to run
 | Returns: N/A
 ----------------------*/
void vmu_set_timer(uint8_t which, uint16_t reload, uint8_t prescale, uint8_t run) {
    VmuTimer *t = timer_at(which);
    t->reload = reload;
    write_mode(t, (run ? 4u : 0u) | (prescale & 3u));
}

void vmu_timer_run(uint8_t which, uint8_t run) {
    VmuTimer *t = timer_at(which);
    write_mode(t, run ? t->mode | 4u : t->mode & ~4u);
}

/*----------------------
 | vmu_set_clock / vmu_clock
 | Description: Sets or reads the clock speed; the RTC's next edge period is taken from the speed when an edge falls.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: level -- speed 0-8
 | Returns: the previous or current speed
 ----------------------*/
uint8_t vmu_set_clock(uint8_t level) {
    uint8_t prev = (uint8_t)h.clk;
    h.clk = level & 0xFu;
    return prev;
}

uint8_t vmu_clock(void) { return (uint8_t)h.clk; }

/*----------------------
 | vmu_sys_flags / vmu_power
 | Description: The flags vmu_host_set_sys holds, and power requests, which stop the host: exit 5 for the exits, exit 7 for standby.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: mode -- VMU_POWER_*
 | Returns: flags (vmu_sys_flags)
 ----------------------*/
uint8_t vmu_sys_flags(void) { return h.sys; }

void vmu_power(uint8_t mode) {
    if (mode == VMU_POWER_STANDBY) die("standby is not modelled", mode);
    fprintf(stderr, "vmu_host: power %u\n", mode);
    exit(5);
}

/*----------------------
 | vmu_file8 / vmu_file_write
 | Description: Reads a byte of the game's file, stopping past its end, or writes 128 bytes of RAM to one of its sectors.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h, vmu_ram
 | Params: off -- file offset; sector -- sector number; src -- RAM offset
 | Returns: the byte; 0 on success, 1 when the sector or source lies outside
 ----------------------*/
uint8_t vmu_file8(uint16_t off) {
    if (off >= h.file_len) die("file read past the end", off);
    return h.file[off];
}

uint8_t vmu_file_write(uint16_t sector, uint16_t src) {
    uint32_t dst = (uint32_t)sector * 0x80u;
    if (dst + 0x80u > h.file_len || (uint32_t)src + 0x80u > sizeof vmu_ram) return 1;
    memcpy(&h.file[dst], &vmu_ram[src], 0x80u);
    return 0;
}

/*----------------------
 | vmu_io_write / vmu_io_read
 | Description: Writes a peripheral register, logging the write with its cycle, or reads one back.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: reg -- VMU_IO_*; value -- value
 | Returns: the value (vmu_io_read)
 ----------------------*/
void vmu_io_write(uint8_t reg, uint16_t value) {
    if (reg >= IO_REGS) die("no peripheral register", reg);
    h.io[reg] = value;
    if (h.log_n < LOG_CAP) {
        VmuIoWrite *e = &h.log[h.log_n++];
        e->cycle = h.cycles;
        e->reg = reg;
        e->value = value;
    }
}

uint16_t vmu_io_read(uint8_t reg) {
    if (reg >= IO_REGS) die("no peripheral register", reg);
    return h.io[reg];
}

/*----------------------
 | vmu_host_init
 | Description: Copies the file, clears RAM and the device, and sets the launcher's clock speed 7, lines RTC and link enabled, LCD mode 0x58 and the first RTC edge.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h, vmu_ram
 | Params: file -- the game's file; len -- its length
 | Returns: N/A
 ----------------------*/
void vmu_host_init(const uint8_t *file, uint32_t len) {
    if (len > sizeof h.file) die("file too long", len);
    memset(&h, 0, sizeof h);
    memset(vmu_ram, 0, sizeof vmu_ram);
    memcpy(h.file, file, len);
    h.file_len = len;
    h.clk = 7;
    h.irq_mask = VMU_LINE_RTC | VMU_LINE_LINK;
    h.io[VMU_IO_LCD_MODE] = 0x58;
    h.rtc_countdown = clock_hz() / 2u;
}

/*----------------------
 | vmu_host_set_entry
 | Description: Loads the device words psk_host_load_entry takes, but RAM, the RTC calendar and the clock-read counter.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: w -- device words; n -- word count
 | Returns: N/A
 ----------------------*/
void vmu_host_set_entry(const uint32_t *w, uint32_t n) {
    if (n < 28u) die("short device state", n);
    h.irq_mask = w[0];
    h.irq_flags = w[1];
    h.irq_latch = w[2];
    for (int i = 0; i < 3; i++) {
        h.timers[i].mode = w[3 + 4 * i];
        h.timers[i].counter = w[4 + 4 * i];
        h.timers[i].reload = w[5 + 4 * i];
        h.timers[i].prescale = w[6 + 4 * i];
    }
    h.clk = w[15] & 0xFu;
    h.rtc_mode = w[17];
    h.rtc_countdown = w[18];
    h.io[VMU_IO_PORT] = (uint16_t)w[22];
    h.io[VMU_IO_DAC_CTRL] = (uint16_t)w[23];
    h.io[VMU_IO_LCD_MODE] = (uint16_t)w[24];
    h.io[VMU_IO_LCD_REG20] = (uint16_t)w[26];
}

/*----------------------
 | vmu_host_set_sys / vmu_host_set_buttons / vmu_host_set_frame
 | Description: Holds the system flags, the buttons (a press requests its line, a release drops its level) and the frame callback.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: flags -- VMU_SYS_*; mask -- VMU_BTN_*; frame -- callback
 | Returns: N/A
 ----------------------*/
void vmu_host_set_sys(uint8_t flags) { h.sys = flags; }

void vmu_host_set_buttons(uint8_t mask) {
    uint32_t changed = (uint32_t)(mask ^ h.buttons) & VMU_LINE_BUTTONS;
    for (int k = 0; k < 5; k++) {
        uint32_t bit = 1u << k;
        if (!(changed & bit)) continue;
        if (mask & bit) {
            request(bit);
        } else {
            h.irq_flags &= ~bit;
        }
    }
    h.buttons = (uint8_t)(mask & VMU_LINE_BUTTONS);
}

void vmu_host_set_frame(void (*frame)(void)) { h.frame = frame; }

/*----------------------
 | vmu_host_cycles / vmu_host_file / vmu_host_io_log
 | Description: Clock cycles run, the file as it stands, and the peripheral write log.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: count -- set to the log's length
 | Returns: the value
 ----------------------*/
uint64_t vmu_host_cycles(void) { return h.cycles; }

const uint8_t *vmu_host_file(void) { return h.file; }

const VmuIoWrite *vmu_host_io_log(uint32_t *count) {
    *count = h.log_n;
    return h.log;
}
