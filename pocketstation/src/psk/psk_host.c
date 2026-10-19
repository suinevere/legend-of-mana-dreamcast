/*----------------------
 | psk_host.c
 | Description: psk_hw.h and psk_swi.h with no emulator: RAM, the card's flash, LCD, buttons, timers, RTC and interrupts modelled from psx-spx and the reference core, kernel calls done in C.
 | Author: suinevere
 | Dependencies: psk_host.h, psk_hw.h, psk_swi.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "psk_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psk_swi.h"

/*----------------------
 | HOST_RETURN
 | Description: Return address given to interrupt callbacks the host calls; never code.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define HOST_RETURN 0xFFFFFFF0u

/*----------------------
 | INT_FIQ_BITS
 | Description: Interrupt bits delivered through FIQ (COM and Timer 2); the others through IRQ.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define INT_FIQ_BITS 0x2040u

/*----------------------
 | INT_LEVEL_BITS
 | Description: INT_INPUT bits that are signal levels INT_ACK leaves alone: buttons, COM, RTC wave, dock.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define INT_LEVEL_BITS (0x1Fu | (1u << 6) | (1u << 9) | (1u << 11))

/*----------------------
 | HostTimer
 | Description: One timer channel as the reference core keeps it: control, count, period and prescaler.
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
} HostTimer;

/*----------------------
 | Host
 | Description: The whole device state.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint8_t ram[0x800];
    uint8_t card[0x20000];
    uint8_t vram[128];
    uint8_t bank[16];
    uint32_t irq_mask, irq_flags, irq_latch, buttons;
    uint32_t lcd_mode, lcd_cal, lcd_reg20, dac_ctrl, dac_data, iop_ctrl, iop_data, clk_mode, rtc_mode;
    int clk_reads;
    HostTimer timers[3];
    uint32_t rtc[7];
    uint32_t rtc_countdown;
    uint64_t events;
    int ctx_irq, ctx_fiq;
    int gate;
    void (*frame)(void);
} Host;

static Host h;

/*----------------------
 | die
 | Description: Stops the host on something the model cannot do.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: what -- message; addr -- address involved
 | Returns: does not return
 ----------------------*/
static void die(const char *what, uint32_t addr) {
    fprintf(stderr, "psk_host: %s %08X\n", what, addr);
    exit(7);
}

/*----------------------
 | clock_hz
 | Description: System clock for CLK_MODE.FREQ, as the reference core maps it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: Hz
 ----------------------*/
static uint32_t clock_hz(void) {
    static const uint32_t hz[9] = {32768, 63488, 126976, 253952, 507904, 1015808, 1998848, 3997696, 7995392};
    uint32_t f = h.clk_mode & 0xFu;
    return hz[f > 8 ? 8 : f];
}

/*----------------------
 | timer_tick
 | Description: One system clock cycle of a timer, as the core's Timer::tick: prescale, count down, reload at zero.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: t -- timer
 | Returns: 1 when the timer expired
 ----------------------*/
static int timer_tick(HostTimer *t) {
    static const uint32_t div[4] = {2, 32, 512, 2};
    if (!(t->mode & 4u) || t->reload == 0) return 0;
    if (++t->prescale < div[t->mode & 3u]) return 0;
    t->prescale = 0;
    if (t->counter == 0) {
        t->counter = t->reload;
        return 1;
    }
    t->counter--;
    return 0;
}

/*----------------------
 | request
 | Description: Raises an interrupt input level and latches it if enabled, as the core's requestInterrupt does.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: bit -- interrupt number
 | Returns: N/A
 ----------------------*/
static void request(int bit) {
    uint32_t m = 1u << bit;
    if (!(h.irq_flags & m)) {
        h.irq_flags |= m;
        h.irq_latch |= m & h.irq_mask;
    }
}

/*----------------------
 | bcd_inc
 | Description: Adds one to a BCD byte.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- BCD value
 | Returns: v + 1 in BCD
 ----------------------*/
static uint32_t bcd_inc(uint32_t v) {
    v++;
    if ((v & 0xFu) == 0xAu) v += 6;
    return v & 0xFFu;
}

/*----------------------
 | rtc_second
 | Description: Advances the BCD calendar one second with carries, as the core's rtcAdvanceOneSecond does.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void rtc_second(void) {
    static const uint8_t dim[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint32_t *r = h.rtc;
    if ((r[0] = bcd_inc(r[0])) <= 0x59u) return;
    r[0] = 0;
    if ((r[1] = bcd_inc(r[1])) <= 0x59u) return;
    r[1] = 0;
    if ((r[2] = bcd_inc(r[2])) <= 0x23u) return;
    r[2] = 0;
    r[3] = r[3] % 7u + 1u;
    r[4] = bcd_inc(r[4]);
    unsigned day = (r[4] >> 4) * 10u + (r[4] & 0xFu), month = (r[5] >> 4) * 10u + (r[5] & 0xFu);
    unsigned year = (r[6] >> 4) * 10u + (r[6] & 0xFu);
    unsigned last = (month >= 1 && month <= 12) ? dim[month - 1] : 31u;
    if (month == 2 && year % 4u == 0) last = 29;
    if (day <= last) return;
    r[4] = 1;
    if ((r[5] = bcd_inc(r[5])) <= 0x12u) return;
    r[5] = 1;
    r[6] = bcd_inc(r[6]);
    if (r[6] > 0x99u) r[6] = 0;
}

/*----------------------
 | lookup
 | Description: The decompiled function at an address (bit 0 = Thumb).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: pc -- address
 | Returns: table entry or null
 ----------------------*/
static const PskHookEntry *lookup(uint32_t pc) {
    for (int i = 0; i < psk_hook_count; i++) {
        if (psk_hooks[i].addr == pc) return &psk_hooks[i];
    }
    return 0;
}

/*----------------------
 | psk_host_run
 | Description: Runs decompiled code from c->r[15] until it continues at ret with the stack pointer at sp.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: c -- register file; ret -- stop address; sp -- stack pointer at the stop
 | Returns: N/A (exits 7 where no decompiled code exists)
 ----------------------*/
void psk_host_run(PskCpu *c, uint32_t ret, uint32_t sp) {
    while (!(c->r[15] == ret && c->r[13] == sp)) {
        const PskHookEntry *e = lookup(c->r[15]);
        if (!e) die("no decompiled code at", c->r[15]);
        e->fn(c);
    }
}

/*----------------------
 | callback
 | Description: Calls an app interrupt callback the way OpenPSK's handler does: on the IRQ or FIQ stack, returning to the kernel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: addr -- callback (bit 0 = Thumb); stack_top -- the mode's stack top
 | Returns: N/A
 ----------------------*/
static void callback(uint32_t addr, uint32_t stack_top) {
    PskCpu c;
    memset(&c, 0, sizeof c);
    c.r[13] = stack_top - 24u;
    c.r[14] = HOST_RETURN;
    c.r[15] = addr;
    psk_host_run(&c, HOST_RETURN, c.r[13]);
}

/*----------------------
 | ram32
 | Description: A little-endian word of work RAM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: a -- RAM address
 | Returns: the word
 ----------------------*/
static uint32_t ram32(uint32_t a) {
    return h.ram[a] | (h.ram[a + 1] << 8) | (h.ram[a + 2] << 16) | ((uint32_t)h.ram[a + 3] << 24);
}

/*----------------------
 | ack
 | Description: An INT_ACK write: clears the latched requests and the non-level request flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: v -- bits
 | Returns: N/A
 ----------------------*/
static void ack(uint32_t v) {
    h.irq_flags &= ~v | INT_LEVEL_BITS;
    h.irq_latch &= ~v;
}

/*----------------------
 | take_one
 | Description: Takes the highest-priority deliverable interrupt as the launcher's FIQ service and OpenPSK's IRQ service do, calling the game's callback in that handler's context.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void take_one(void) {
    uint32_t pending = h.irq_latch & h.irq_mask;
    int irq = h.ctx_irq, fiq = h.ctx_fiq;
    if (!fiq && (pending & INT_FIQ_BITS)) {
        uint32_t r8 = pending & ~(1u << 6);
        if (pending & (1u << 6)) h.irq_latch &= ~(1u << 6);
        h.ctx_irq = h.ctx_fiq = 1;
        if (r8) {
            if (ram32(0x100)) {
                callback(ram32(0x100), 0x200);
            } else {
                ack(r8);
            }
        }
    } else if (!irq && (pending & 0x1F9Fu)) {
        h.ctx_irq = 1;
        ack((1u << 9) | (1u << 11));
        if (ram32(0xFC)) callback(ram32(0xFC), 0x180);
    }
    h.ctx_irq = irq;
    h.ctx_fiq = fiq;
}

/*----------------------
 | tick_rtc
 | Description: One system clock cycle of the RTC square wave: each edge is a device event, a rising edge requests IRQ 9 and in RUN mode advances a second.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void tick_rtc(void) {
    if (--h.rtc_countdown != 0) return;
    h.rtc_countdown = clock_hz() / (2u * ((h.rtc_mode & 1u) ? 4096u : 1u));
    h.events++;
    if (h.irq_flags & (1u << 9)) {
        h.irq_flags &= ~(1u << 9);
        return;
    }
    request(9);
    if ((h.rtc_mode & 1u) == 0) rtc_second();
}

/*----------------------
 | tick_timers
 | Description: One system clock cycle of the three timers; an expiry latches its line (7, 8, 13) and is a device event.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void tick_timers(void) {
    static const int line[3] = {7, 8, 13};
    for (int i = 0; i < 3; i++) {
        if (timer_tick(&h.timers[i])) {
            h.irq_latch |= 1u << line[i];
            h.events++;
        }
    }
}

/*----------------------
 | advance_to_event
 | Description: Advances the devices cycle by cycle to exactly the next timer expiry or RTC edge, as the core's advanceToEvent.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void advance_to_event(void) {
    uint64_t start = h.events;
    while (h.events == start) {
        tick_rtc();
        tick_timers();
    }
}

/*----------------------
 | deliverable
 | Description: Pending interrupts the current context would take: FIQ sources outside a FIQ, IRQ sources outside any handler.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: interrupt bits
 ----------------------*/
static uint32_t deliverable(void) {
    return h.irq_latch & h.irq_mask & ((h.ctx_fiq ? 0u : INT_FIQ_BITS) | (h.ctx_irq ? 0u : 0x1F9Fu));
}

/*----------------------
 | psk_host_advance
 | Description: Runs the devices for a span of emulated time, cycle by cycle, without delivering interrupts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: seconds -- emulated time
 | Returns: N/A
 ----------------------*/
void psk_host_advance(double seconds) {
    uint64_t n = (uint64_t)(seconds * (double)clock_hz());
    for (uint64_t i = 0; i < n; i++) {
        tick_rtc();
        tick_timers();
    }
}

/*----------------------
 | set_clock
 | Description: Writes CLK_MODE.FREQ; timers and the RTC count system cycles, so nothing rescales.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: mode -- new CLK_MODE
 | Returns: N/A
 ----------------------*/
static void set_clock(uint32_t mode) { h.clk_mode = mode & 0xFu; }

/*----------------------
 | write_timer_mode
 | Description: Writes a timer's control as the core's Timer::writeMode: restarts the prescaler and loads the count on enable.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: t -- timer; v -- control
 | Returns: N/A
 ----------------------*/
static void write_timer_mode(HostTimer *t, uint32_t v) {
    uint32_t was = t->mode & 4u;
    t->mode = v;
    t->prescale = 0;
    if (!was && (v & 4u)) t->counter = t->reload;
}

/*----------------------
 | flash_byte
 | Description: The card byte behind an address in the FLASH1 window or the physical flash.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: addr -- address
 | Returns: pointer into the card, or null outside both windows
 ----------------------*/
static uint8_t *flash_byte(uint32_t addr) {
    if (addr - 0x02000000u < 0x20000u) {
        uint32_t off = addr - 0x02000000u;
        uint8_t phys = h.bank[off >> 13];
        if (phys == 0xFF) return 0;
        return &h.card[(uint32_t)phys * 0x2000u + (off & 0x1FFFu)];
    }
    if (addr - 0x08000000u < 0x20000u) return &h.card[addr - 0x08000000u];
    return 0;
}

/*----------------------
 | io_read
 | Description: A 32-bit I/O register read, as the reference core answers it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: addr -- register
 | Returns: value
 ----------------------*/
static uint32_t io_read(uint32_t addr) {
    if (addr - 0x0D000100u < 0x80u) {
        uint32_t o = addr & 0x7Cu;
        return h.vram[o] | (h.vram[o + 1] << 8) | (h.vram[o + 2] << 16) | ((uint32_t)h.vram[o + 3] << 24);
    }
    if (addr >= 0x0A800000u && addr < 0x0A800030u) {
        const HostTimer *t = &h.timers[(addr >> 4) & 3u];
        switch (addr & 0xFu) {
            case 0x0: return t->reload;
            case 0x8: return t->mode;
            default: return t->counter;
        }
    }
    if (addr >= 0x0C000000u && addr < 0x0C000020u) return 0;
    if (addr >= 0x0C800000u && addr < 0x0C800010u) return 0;
    switch (addr) {
        case 0x0A000000u: return h.irq_latch;
        case 0x0A000004u: return h.irq_flags;
        case 0x0A000008u: return h.irq_mask;
        case 0x0B000000u:
            if (h.clk_reads++ == 4) {
                h.clk_mode |= 0x10u;
            } else if (h.clk_reads == 10) {
                h.clk_reads = 0;
                h.clk_mode &= ~0x10u;
            }
            return h.clk_mode;
        case 0x0B800000u: return h.rtc_mode;
        case 0x0B800008u: return h.rtc[0] | (h.rtc[1] << 8) | (h.rtc[2] << 16) | (h.rtc[3] << 24);
        case 0x0B80000Cu: return h.rtc[4] | (h.rtc[5] << 8) | (h.rtc[6] << 16);
        case 0x0D000000u: return h.lcd_mode;
        case 0x0D000020u: return h.lcd_reg20;
        case 0x0D800000u: return h.iop_ctrl;
        case 0x0D800004u: return 0;
        case 0x0D80000Cu: return h.iop_data;
        case 0x0D800010u: return h.dac_ctrl;
        case 0x0D800020u: return 0;
        default: die("read from unmodelled I/O", addr);
    }
    return 0;
}

/*----------------------
 | io_write
 | Description: A 32-bit I/O register write, as the reference core takes it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: addr -- register; v -- value
 | Returns: N/A
 ----------------------*/
static void io_write(uint32_t addr, uint32_t v) {
    if (addr - 0x0D000100u < 0x80u) {
        uint32_t o = addr & 0x7Cu;
        for (int i = 0; i < 4; i++) h.vram[o + i] = (uint8_t)(v >> (8 * i));
        return;
    }
    if (addr >= 0x0A800000u && addr < 0x0A800030u) {
        HostTimer *t = &h.timers[(addr >> 4) & 3u];
        if ((addr & 0xFu) == 0x0) t->reload = v & 0xFFFFu;
        if ((addr & 0xFu) == 0x8) write_timer_mode(t, v);
        return;
    }
    if (addr >= 0x0C000000u && addr < 0x0C000020u) return;
    if (addr >= 0x0C800000u && addr < 0x0C800010u) return;
    switch (addr) {
        case 0x0A000008u: h.irq_mask |= v; return;
        case 0x0A00000Cu: h.irq_mask &= ~v; return;
        case 0x0A000010u:
            h.irq_flags &= ~v | INT_LEVEL_BITS;
            h.irq_latch &= ~v;
            return;
        case 0x0B000000u: set_clock(v); return;
        case 0x0B000004u:
            if (v & 1u) die("CLK_STOP sleep is not modelled", v);
            return;
        case 0x0B800000u: h.rtc_mode = v; return;
        case 0x0D000000u: h.lcd_mode = v; return;
        case 0x0D000004u: h.lcd_cal = v & 0xFFFFu; return;
        case 0x0D000020u: h.lcd_reg20 = v; return;
        case 0x0D800000u: h.iop_ctrl = v & 0xFu; return;
        case 0x0D800004u: h.iop_data |= v; return;
        case 0x0D800008u: h.iop_data &= ~v; return;
        case 0x0D800010u: h.dac_ctrl = v; return;
        case 0x0D800014u: h.dac_data = v; return;
        case 0x0D800020u: return;
        default: die("write to unmodelled I/O", addr);
    }
}

/*----------------------
 | psk_r8 / psk_r16 / psk_r32
 | Description: Reads RAM, the FLASH1 window, physical flash or an I/O register.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: addr -- address
 | Returns: the value
 ----------------------*/
uint8_t psk_r8(uint32_t addr) {
    if (addr < 0x800u) return h.ram[addr];
    uint8_t *f = flash_byte(addr);
    if (f) return *f;
    return (uint8_t)(io_read(addr & ~3u) >> (8 * (addr & 3u)));
}

uint16_t psk_r16(uint32_t addr) { return (uint16_t)(psk_r8(addr) | (psk_r8(addr + 1) << 8)); }

uint32_t psk_r32(uint32_t addr) {
    if (addr < 0x800u || flash_byte(addr)) return psk_r16(addr) | ((uint32_t)psk_r16(addr + 2) << 16);
    return io_read(addr);
}

/*----------------------
 | psk_w8 / psk_w16 / psk_w32
 | Description: Writes RAM, VRAM or an I/O register; flash is written only through SWI 03h.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: addr -- address; v -- value
 | Returns: N/A
 ----------------------*/
void psk_w8(uint32_t addr, uint8_t v) {
    if (addr < 0x800u) {
        h.ram[addr] = v;
        return;
    }
    if (addr - 0x0D000100u < 0x80u) {
        h.vram[addr & 0x7Fu] = v;
        return;
    }
    die("byte write outside RAM and VRAM", addr);
}

void psk_w16(uint32_t addr, uint16_t v) {
    psk_w8(addr, (uint8_t)v);
    psk_w8(addr + 1, (uint8_t)(v >> 8));
}

void psk_w32(uint32_t addr, uint32_t v) {
    if (addr < 0x800u || addr - 0x0D000100u < 0x80u) {
        psk_w16(addr, (uint16_t)v);
        psk_w16(addr + 2, (uint16_t)(v >> 16));
        return;
    }
    io_write(addr, v);
}

/*----------------------
 | psk_frame
 | Description: Calls the driver's frame callback.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void psk_frame(void) {
    if (h.frame) h.frame();
}

/*----------------------
 | psk_idle
 | Description: The idle-time schedule: advances the devices to their next event, then takes interrupts until the context it idled from has none deliverable or a nested idle closed the gate.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void psk_idle(void) {
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
 | psk_call_arm
 | Description: There is no ARM on the host: stops with the callee's address.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: addr -- callee; r0-r3 -- arguments
 | Returns: does not return
 ----------------------*/
uint32_t psk_call_arm(uint32_t addr, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3) {
    (void)r0;
    (void)r1;
    (void)r2;
    (void)r3;
    die("no ARM on the host; psk_call_arm to", addr);
    return 0;
}

/*----------------------
 | psk_call
 | Description: A register-level call into decompiled code, run until it returns to ret at the same stack depth.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: c -- register file; target -- callee; ret -- return address
 | Returns: N/A
 ----------------------*/
void psk_call(PskCpu *c, uint32_t target, uint32_t ret) {
    uint32_t sp = c->r[13];
    c->r[14] = ret;
    c->r[15] = target;
    psk_host_run(c, ret, sp);
}

/*----------------------
 | flash_write_virtual
 | Description: SWI 03h as the launcher implements it: 128 bytes from src to virtual sector number sector of the app.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: sector -- virtual sector; src -- RAM source
 | Returns: 0 okay, 1 failed
 ----------------------*/
static uint32_t flash_write_virtual(uint32_t sector, uint32_t src) {
    if (sector > 0x3FFu || (src & 1u) || src - 0x08000000u < 0x20000u) return 1;
    uint8_t *dst = flash_byte(0x02000000u + sector * 0x80u);
    if (!dst) return 1;
    for (uint32_t i = 0; i < 0x80u; i++) dst[i] = psk_r8(src + i);
    return 0;
}

/*----------------------
 | psk_swi
 | Description: The census kernel calls, answering as the OpenPSK launcher does.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: number -- SWI number; r0-r2 -- arguments
 | Returns: the kernel's r0 (exits 4 on an SWI the app never makes)
 ----------------------*/
uint32_t psk_swi(uint32_t number, uint32_t r0, uint32_t r1, uint32_t r2) {
    switch (number) {
        case 0x01: {
            if (r0 > 3u) return 0;
            uint32_t slot = 0xF8u + r0 * 4u, prev = ram32(slot);
            psk_w32(slot, r1);
            return prev;
        }
        case 0x03: return flash_write_virtual(r0, r1);
        case 0x04: {
            uint32_t prev = h.clk_mode & 0xFu;
            set_clock(r0);
            return prev;
        }
        case 0x06: return 0xC0u;
        case 0x08:
            if (r0 == 1u) {
                psk_w16(0xD2u, (uint16_t)r1);
                psk_w32(0xD4u, r2);
            }
            return psk_r16(0xD2u);
        case 0x11:
            if (r0) {
                h.irq_mask |= 1u << 6;
                psk_w32(0xC0u, ram32(0xC0u) | (1u << 9));
            } else {
                h.irq_mask &= ~(1u << 6);
                h.irq_latch &= ~(1u << 6);
                psk_w32(0xC0u, ram32(0xC0u) & ~(1u << 9));
            }
            return 0;
        case 0x13: return 0xD8u;
        case 0x14: return 0xE0u;
        case 0x16: return psk_r16(0xD2u);
        default:
            fprintf(stderr, "psk_host: unimplemented swi %u\n", number);
            exit(4);
    }
}

/*----------------------
 | psk_host_init
 | Description: Loads the card and the kernel RAM page, maps the app's banks like launch.c, and sets the launcher's clock, interrupt mask and LCD mode.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: card -- 128 KB image; kernel_ram -- 0x200 bytes
 | Returns: N/A
 ----------------------*/
void psk_host_init(const uint8_t *card, const uint8_t *kernel_ram) {
    memset(&h, 0, sizeof h);
    memcpy(h.card, card, sizeof h.card);
    memcpy(h.ram, kernel_ram, 0x200);
    memset(h.bank, 0xFF, sizeof h.bank);
    unsigned next = 0;
    for (unsigned b = 1, n = 0; b >= 1 && b <= 15 && n < 15; n++) {
        h.bank[next++] = (uint8_t)b;
        unsigned link = h.card[b * 0x80u + 8u] | (h.card[b * 0x80u + 9u] << 8);
        if (link == 0xFFFFu) break;
        b = link + 1u;
    }
    h.clk_mode = 7;
    h.irq_mask = (1u << 9) | (1u << 11);
    h.lcd_mode = 0x58;
    h.rtc[3] = 1;
    h.rtc[4] = 1;
    h.rtc[5] = 1;
    h.rtc[6] = 0x99;
    h.rtc_countdown = clock_hz() / 2u;
}

/*----------------------
 | psk_host_load_entry
 | Description: Loads the work RAM and device state the reference runner dumped at app entry (pkrun --dump-entry-state).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: wram -- 2 KB; w -- device words in Bus::deviceWords order; n -- word count
 | Returns: N/A
 ----------------------*/
void psk_host_load_entry(const uint8_t *wram, const uint32_t *w, uint32_t n) {
    if (n < 28u) die("short device state", n);
    memcpy(h.ram, wram, sizeof h.ram);
    h.irq_mask = w[0];
    h.irq_flags = w[1];
    h.irq_latch = w[2];
    for (int i = 0; i < 3; i++) {
        h.timers[i].mode = w[3 + 4 * i];
        h.timers[i].counter = w[4 + 4 * i];
        h.timers[i].reload = w[5 + 4 * i];
        h.timers[i].prescale = w[6 + 4 * i];
    }
    h.clk_mode = w[15];
    h.clk_reads = (int)w[16];
    h.rtc_mode = w[17];
    h.rtc_countdown = w[18];
    psk_host_set_rtc(w[20], w[19]);
    h.iop_data = w[21];
    h.iop_ctrl = w[22];
    h.dac_ctrl = w[23];
    h.lcd_mode = w[24];
    h.lcd_cal = w[25];
    h.lcd_reg20 = w[26];
}

/*----------------------
 | psk_host_card
 | Description: The card image the host is running from, for carrying it across a reboot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: 128 KB card
 ----------------------*/
uint8_t *psk_host_card(void) { return h.card; }


/*----------------------
 | psk_host_set_buttons
 | Description: Holds the button mask; a press raises the level and latches its interrupt, a release drops the level.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: mask -- buttons
 | Returns: N/A
 ----------------------*/
void psk_host_set_buttons(uint32_t mask) {
    uint32_t changed = (mask ^ h.buttons) & 0x1Fu;
    for (int k = 0; k < 5; k++) {
        uint32_t bit = 1u << k;
        if (!(changed & bit)) continue;
        if (mask & bit) {
            request(k);
        } else {
            h.irq_flags &= ~bit;
        }
    }
    h.buttons = mask & 0x1Fu;
}

/*----------------------
 | psk_host_set_rtc
 | Description: Sets the BCD clock fields.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: date -- RTC_DATE layout; time -- RTC_TIME layout
 | Returns: N/A
 ----------------------*/
void psk_host_set_rtc(uint32_t date, uint32_t time) {
    for (int i = 0; i < 4; i++) h.rtc[i] = (time >> (8 * i)) & 0xFFu;
    for (int i = 0; i < 3; i++) h.rtc[4 + i] = (date >> (8 * i)) & 0xFFu;
}

/*----------------------
 | psk_host_set_frame
 | Description: Sets the callback psk_frame calls.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: frame -- callback
 | Returns: N/A
 ----------------------*/
void psk_host_set_frame(void (*frame)(void)) { h.frame = frame; }

/*----------------------
 | psk_host_vram / psk_host_ram
 | Description: The LCD frame buffer and the work RAM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: pointer
 ----------------------*/
const uint8_t *psk_host_vram(void) { return h.vram; }

const uint8_t *psk_host_ram(void) { return h.ram; }

/*----------------------
 | psk_host_card_crc
 | Description: CRC-32 of the card image.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: the CRC
 ----------------------*/
uint32_t psk_host_card_crc(void) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < sizeof h.card; i++) {
        c ^= h.card[i];
        for (int k = 0; k < 8; k++) c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
    }
    return c ^ 0xFFFFFFFFu;
}
