/*----------------------
 | vmu_host.c
 | Description: vmu_hw.h on the host: RAM, the frame buffer, the game's tables and pet save, the buzzer log, and five lines, one handler, the tick, the sound end and the RTC edge counted in timer counts (two clock cycles each), matching psk_host.c's schedule of the original's timers.
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
 | LINES
 | Description: The five interrupt lines the host has; nothing else latches, enables or is delivered.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LINES (VMU_LINE_BUTTONS | VMU_LINE_TICK | VMU_LINE_RTC | VMU_LINE_DOCK | VMU_LINE_SOUND)

/*----------------------
 | LEVEL_LINES
 | Description: Lines whose request level follows a signal, which an ack leaves alone: buttons, RTC and dock.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LEVEL_LINES (VMU_LINE_BUTTONS | VMU_LINE_RTC | VMU_LINE_DOCK)

/*----------------------
 | LOG_CAP
 | Description: Entries the buzzer log keeps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOG_CAP 65536u

/*----------------------
 | TICK_RELOAD
 | Description: Tick reload in timer counts by VMU_TICK_* mode; a period is one count more, as the original's timer counts through zero.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const uint32_t TICK_RELOAD[4] = {0, 15625, 15625, 1953};

/*----------------------
 | TICK_CLOCK
 | Description: The original's clock speed by VMU_TICK_* mode, which sets the RTC's half-period in counts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const uint8_t TICK_CLOCK[4] = {0, 5, 6, 2};

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
    uint8_t rom[0x10000];
    uint32_t rom_len;
    uint8_t save[0x10000];
    uint32_t save_len;
    uint32_t irq_mask, irq_flags, irq_latch;
    uint8_t buttons, sys, held, ignore, asleep, quit;
    uint8_t tick_mode;
    uint32_t tick_left, sound_left, rtc_left, rtc_half;
    uint64_t time;
    uint32_t idles;
    int in_handler;
    void (*handler)(void);
    void (*frame)(void);
    uint32_t log_n;
    VmuBuzz log[LOG_CAP];
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
 | rtc_half
 | Description: The RTC's half-period in timer counts at a clock speed: half a second of the original's clock cycles, two to a count.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: level -- clock speed 0-8
 | Returns: counts
 ----------------------*/
static uint32_t rtc_half(uint32_t level) {
    static const uint32_t hz[9] = {32768, 63488, 126976, 253952, 507904, 1015808, 1998848, 3997696, 7995392};
    return hz[level > 8u ? 8u : level] / 4u;
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
 | rtc_edge
 | Description: An RTC square-wave edge: the next half-period is the one the tick mode last set, and a rising edge requests the RTC line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void rtc_edge(void) {
    h.rtc_left = h.rtc_half;
    if (h.irq_flags & VMU_LINE_RTC) {
        h.irq_flags &= ~VMU_LINE_RTC;
        return;
    }
    request(VMU_LINE_RTC);
}

/*----------------------
 | advance_to_event
 | Description: Runs time to the nearest of the tick, the sound end and the RTC edge, then fires every one due in that count: the tick latches its line and reloads, the sound end latches its line once.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void advance_to_event(void) {
    uint32_t k = h.rtc_left;
    if (h.tick_mode && h.tick_left < k) k = h.tick_left;
    if (h.sound_left && h.sound_left < k) k = h.sound_left;
    h.rtc_left -= k;
    if (h.rtc_left == 0) rtc_edge();
    if (h.tick_mode) {
        h.tick_left -= k;
        if (h.tick_left == 0) {
            h.tick_left = TICK_RELOAD[h.tick_mode] + 1u;
            h.irq_latch |= VMU_LINE_TICK;
        }
    }
    if (h.sound_left) {
        h.sound_left -= k;
        if (h.sound_left == 0) h.irq_latch |= VMU_LINE_SOUND;
    }
    h.time += k;
}

/*----------------------
 | deliverable
 | Description: Latched, enabled lines, none while the handler runs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: line bits
 ----------------------*/
static uint32_t deliverable(void) {
    return h.in_handler ? 0u : h.irq_latch & h.irq_mask & LINES;
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
 | take
 | Description: One delivery as the PocketStation launcher's: acknowledges the RTC, then runs the handler, or with none acknowledges every line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void take(void) {
    h.in_handler = 1;
    ack(VMU_LINE_RTC);
    if (h.handler) {
        h.handler();
    } else {
        ack(LINES);
    }
    h.in_handler = 0;
}

/*----------------------
 | log_call
 | Description: Logs a vmu_buzz or vmu_lcd call with the idles begun before it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: lcd -- 1 for vmu_lcd; value -- tone, or 0/1
 | Returns: N/A
 ----------------------*/
static void log_call(uint8_t lcd, uint8_t value) {
    if (h.log_n < LOG_CAP) {
        VmuBuzz *e = &h.log[h.log_n++];
        e->idle = h.idles;
        e->lcd = lcd;
        e->value = value;
    }
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
 | Description: Advances to the next count with an event, then calls the handler until no enabled line is latched; everything due in one count goes in one call, the RTC acknowledged first. An idle inside the handler only advances time.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void vmu_idle(void) {
    advance_to_event();
    h.idles++;
    for (int guard = 0; deliverable(); guard++) {
        if (guard == 64) die("interrupt storm, latch", h.irq_latch);
        take();
    }
}

/*----------------------
 | vmu_irq_enable / vmu_irq_disable / vmu_irq_ack / vmu_irq_pending / vmu_irq_levels / vmu_irq_mask
 | Description: The interrupt controller over the five lines: mask set and clear, acknowledge, and the latched, level and enabled lines.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: lines -- VMU_LINE_* bits
 | Returns: line bits (the reads)
 ----------------------*/
void vmu_irq_enable(uint16_t lines) { h.irq_mask |= lines & LINES; }

void vmu_irq_disable(uint16_t lines) { h.irq_mask &= ~(uint32_t)lines; }

void vmu_irq_ack(uint16_t lines) { ack(lines); }

uint16_t vmu_irq_pending(void) { return (uint16_t)h.irq_latch; }

uint16_t vmu_irq_levels(void) { return (uint16_t)h.irq_flags; }

uint16_t vmu_irq_mask(void) { return (uint16_t)h.irq_mask; }

/*----------------------
 | vmu_set_handler
 | Description: Installs the handler take calls.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: irq -- handler, or null
 | Returns: N/A
 ----------------------*/
void vmu_set_handler(void (*irq)(void)) { h.handler = irq; }

/*----------------------
 | vmu_tick_mode
 | Description: Sets the tick mode: a running mode from OFF loads a full period, one from another running mode keeps the period's remaining counts; a running mode also sets the RTC's next half-period.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: mode -- VMU_TICK_*
 | Returns: N/A
 ----------------------*/
void vmu_tick_mode(uint8_t mode) {
    if (mode > VMU_TICK_STANDBY) die("no tick mode", mode);
    if (mode != VMU_TICK_OFF) {
        if (h.tick_mode == VMU_TICK_OFF) h.tick_left = TICK_RELOAD[mode] + 1u;
        h.rtc_half = rtc_half(TICK_CLOCK[mode]);
    }
    h.tick_mode = mode;
}

/*----------------------
 | vmu_sound_play / vmu_sound_stop
 | Description: Times the sound end (samples + 1) * (counts_per_sample + 1) counts from now, or cancels it and acknowledges its line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: samples -- samples to play; counts_per_sample -- sample reload
 | Returns: N/A
 ----------------------*/
void vmu_sound_play(uint32_t samples, uint16_t counts_per_sample) {
    h.sound_left = (samples + 1u) * (counts_per_sample + 1u);
}

void vmu_sound_stop(void) {
    h.sound_left = 0;
    ack(VMU_LINE_SOUND);
}

/*----------------------
 | vmu_buzz / vmu_lcd
 | Description: Log the buzzer tone or the LCD switch, the LCD's as 0 or 1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: tone -- 0 off, 1-255 a pitch step; on -- nonzero for on
 | Returns: N/A
 ----------------------*/
void vmu_buzz(uint8_t tone) { log_call(0, tone); }

void vmu_lcd(uint8_t on) { log_call(1, on != 0); }

/*----------------------
 | vmu_sys_flags / vmu_power
 | Description: The flags vmu_host_set_sys holds, with VMU_SYS_QUIT while a Mode press is unread, and power requests: standby takes events as vmu_idle does, with no handler and no idle counted, until an enabled button or RTC line is latched, stopping (exit 7) if none comes in 2^20 events; the exits stop the host with exit 5.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: mode -- VMU_POWER_*
 | Returns: flags (vmu_sys_flags)
 ----------------------*/
uint8_t vmu_sys_flags(void) {
    uint8_t f = (uint8_t)(h.sys | (h.quit ? VMU_SYS_QUIT : 0u));
    h.quit = 0;
    return f;
}

void vmu_power(uint8_t mode) {
    if (mode == VMU_POWER_STANDBY) {
        for (uint32_t n = 0; n < (1u << 20); n++) {
            advance_to_event();
            if (h.irq_latch & h.irq_mask & (VMU_LINE_BUTTONS | VMU_LINE_RTC)) return;
        }
        die("standby never woke, mask", h.irq_mask);
    }
    fprintf(stderr, "vmu_host: power %u\n", mode);
    exit(5);
}

/*----------------------
 | SAVE_RAM_LO / SAVE_RAM_HI
 | Description: The RAM offsets a save sector may use: 128 bytes of work RAM ending before the VMU runtime's rt_save at 0x2E8, the range its page buffer trade works on.
 | Author: suinevere
 | Dependencies: pocketstation/src/game/ram_map.txt, pocketstation/src/vmu_rt/rt_file.s
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SAVE_RAM_LO 0x100u
#define SAVE_RAM_HI 0x268u

/*----------------------
 | vmu_rom8 / vmu_save_read / vmu_save_write
 | Description: Reads a byte of the tables, stopping past their end, or copies 128 bytes between RAM and one sector of the save.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h, vmu_ram
 | Params: off -- table offset; sector -- sector number; dst, src -- RAM offset
 | Returns: the byte; 0 on success, 1 when the sector lies outside the save or the RAM range outside work RAM before rt_save (0x100-0x2E7), the runtime's contract
 ----------------------*/
uint8_t vmu_rom8(uint16_t off) {
    if (off >= h.rom_len) die("table read past the end", off);
    return h.rom[off];
}

uint8_t vmu_save_read(uint16_t sector, uint16_t dst) {
    uint32_t at = (uint32_t)sector * 0x80u;
    if (at + 0x80u > h.save_len || dst < SAVE_RAM_LO || dst > SAVE_RAM_HI) return 1;
    memcpy(&vmu_ram[dst], &h.save[at], 0x80u);
    return 0;
}

uint8_t vmu_save_write(uint16_t sector, uint16_t src) {
    uint32_t at = (uint32_t)sector * 0x80u;
    if (at + 0x80u > h.save_len || src < SAVE_RAM_LO || src > SAVE_RAM_HI) return 1;
    memcpy(&h.save[at], &vmu_ram[src], 0x80u);
    return 0;
}

/*----------------------
 | vmu_host_init
 | Description: Copies the tables and the save, clears RAM and the device, and sets the tick off, only the RTC line enabled and the first RTC edge a half-period of the launcher's clock speed 7 away.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h, vmu_ram
 | Params: rom -- the tables; rom_len -- their length; save -- the pet save; save_len -- its length
 | Returns: N/A
 ----------------------*/
void vmu_host_init(const uint8_t *rom, uint32_t rom_len, const uint8_t *save, uint32_t save_len) {
    if (rom_len > sizeof h.rom) die("tables too long", rom_len);
    if (save_len > sizeof h.save) die("save too long", save_len);
    memset(&h, 0, sizeof h);
    memset(vmu_ram, 0, sizeof vmu_ram);
    memcpy(h.rom, rom, rom_len);
    h.rom_len = rom_len;
    memcpy(h.save, save, save_len);
    h.save_len = save_len;
    h.irq_mask = VMU_LINE_RTC;
    h.rtc_half = rtc_half(7u);
    h.rtc_left = h.rtc_half;
}

/*----------------------
 | vmu_host_set_entry
 | Description: Loads psk_host_load_entry's interrupt words for the five lines and the RTC's next edge, rounded up to a count, stopping on the /4096 RTC mode. The rounding can merge an edge psk_host takes a cycle before a tick into that tick's handler call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: w -- device words; n -- word count
 | Returns: N/A
 ----------------------*/
void vmu_host_set_entry(const uint32_t *w, uint32_t n) {
    if (n < 28u) die("short device state", n);
    if (w[17] & 1u) die("RTC /4096 mode is not modelled", w[17]);
    h.irq_mask = w[0] & LINES;
    h.irq_flags = w[1] & LINES;
    h.irq_latch = w[2] & LINES;
    h.rtc_half = rtc_half(w[15] & 0xFu);
    h.rtc_left = (w[18] + 1u) / 2u;
    if (h.rtc_left == 0) die("no RTC edge", w[18]);
}

/*----------------------
 | vmu_host_set_sys / vmu_host_set_frame
 | Description: Holds the system flags and the frame callback.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: flags -- VMU_SYS_*; frame -- callback
 | Returns: N/A
 ----------------------*/
void vmu_host_set_sys(uint8_t flags) { h.sys = flags; }

/*----------------------
 | vmu_host_set_buttons
 | Description: Holds the buttons as the runtime reads port 3: Mode's press raises the quit request, Sleep's toggles an unseen sleep that ignores other buttons, and buttons held at the wake read as released until let go.
 | Author: suinevere
 | Dependencies: request
 | Globals: h
 | Params: mask -- VMU_BTN_* bits with VMU_HOST_MODE and VMU_HOST_SLEEP
 | Returns: N/A
 ----------------------*/
void vmu_host_set_buttons(uint8_t mask) {
    uint8_t held = (uint8_t)(mask & (VMU_LINE_BUTTONS | VMU_HOST_MODE | VMU_HOST_SLEEP));
    uint8_t press = (uint8_t)(held & ~h.held), sleep = 0;
    h.held = held;
    h.ignore &= held;
    if (h.asleep) {
        if (!(press & VMU_HOST_SLEEP)) return;
        h.asleep = 0;
        h.ignore = held;
    } else {
        press &= (uint8_t)~h.ignore;
        if (press & VMU_HOST_MODE) h.quit = 1;
        sleep = press & VMU_HOST_SLEEP;
    }
    uint8_t eff = (uint8_t)(held & ~h.ignore & VMU_LINE_BUTTONS);
    uint8_t changed = (uint8_t)(eff ^ h.buttons);
    for (int k = 0; k < 5; k++) {
        uint32_t bit = 1u << k;
        if (!(changed & bit)) continue;
        if (eff & bit) {
            request(bit);
        } else {
            h.irq_flags &= ~bit;
        }
    }
    h.buttons = eff;
    if (sleep) h.asleep = 1;
}

/*----------------------
 | vmu_host_set_dock
 | Description: The dock line follows port 7: docking requests it, undocking drops its level.
 | Author: suinevere
 | Dependencies: request
 | Globals: h
 | Params: on -- nonzero when docked
 | Returns: N/A
 ----------------------*/
void vmu_host_set_dock(uint8_t on) {
    if (on) {
        request(VMU_LINE_DOCK);
    } else {
        h.irq_flags &= ~VMU_LINE_DOCK;
    }
}

void vmu_host_set_frame(void (*frame)(void)) { h.frame = frame; }

/*----------------------
 | vmu_host_time / vmu_host_save / vmu_host_buzz_log
 | Description: Timer counts run, the save as it stands, and the buzzer log.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: h
 | Params: count -- set to the log's length
 | Returns: the value
 ----------------------*/
uint64_t vmu_host_time(void) { return h.time; }

const uint8_t *vmu_host_save(void) { return h.save; }

const VmuBuzz *vmu_host_buzz_log(uint32_t *count) {
    *count = h.log_n;
    return h.log;
}
