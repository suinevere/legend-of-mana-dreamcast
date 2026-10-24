/*----------------------
 | vmu_host_test.c
 | Description: Drives vmu_host.c's tick modes, sound end and one handler and, as their oracle, psk_host.c's timers, clock levels and two handlers through the same scenarios, printing each backend's delivery sequence; also checks buttons, the buzzer log, the split of tables and pet save and the frame buffer.
 | Author: suinevere
 | Dependencies: vmu_hw.h, vmu_host.h, psk_host.h, psk_hw.h, psk_swi.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psk_host.h"
#include "psk_hw.h"
#include "psk_swi.h"
#include "vmu_host.h"
#include "vmu_hw.h"

#define PSK_IRQ_FN 0x02000100u
#define PSK_FIQ_FN 0x02000200u

static void psk_irq_fn(PskCpu *c);
static void psk_fiq_fn(PskCpu *c);

const PskHookEntry psk_hooks[] = {
    {PSK_IRQ_FN, psk_irq_fn, "irq", 1, 0, 0u},
    {PSK_FIQ_FN, psk_fiq_fn, "fiq", 1, 0, 0u},
};
const int psk_hook_count = 2;

static unsigned char card[0x20000];
static unsigned char kram[0x200];
static char seq[2][1 << 20];
static size_t seq_len[2];
static int side, idles, narrow_mode, calls;
static unsigned sound_left;
static char letters[8];
static int letters_n;

static void emit(const char *fmt, int at, const char *what, unsigned lines) {
    size_t *n = &seq_len[side];
    *n += (size_t)snprintf(seq[side] + *n, sizeof seq[side] - *n, fmt, at, what, lines);
    if (*n >= sizeof seq[side] - 32) {
        fprintf(stderr, "vmu_host_test: sequence overflow\n");
        exit(3);
    }
}

static void note(const char *what, unsigned lines) { emit("%d%s%X ", idles, what, lines); }

static void letter(char c) {
    if (letters_n < (int)sizeof letters - 1) letters[letters_n++] = c;
}

static unsigned handle(unsigned latch, unsigned mask, const char *what) {
    unsigned lines = latch & mask & (*what == 'f' ? 0x2000u : 0x100u);
    if (!narrow_mode) {
        note(what, lines);
        return lines;
    }
    if (lines & 0x100u) letter('t');
    if ((lines & 0x2000u) && sound_left && --sound_left == 0) letter('s');
    return lines;
}

static void psk_irq_fn(PskCpu *c) {
    calls++;
    psk_w32(0x0A000010u, handle(psk_r32(0x0A000000u), psk_r32(0x0A000008u), "i"));
    c->r[15] = c->r[14];
}

static void psk_fiq_fn(PskCpu *c) {
    calls++;
    psk_w32(0x0A000010u, handle(psk_r32(0x0A000000u), psk_r32(0x0A000008u), "f"));
    if (narrow_mode && sound_left == 0) {
        psk_w32(0x0A800028u, 0);
        psk_w32(0x0A00000Cu, 0x2000u);
    }
    c->r[15] = c->r[14];
}

static void vmu_handler(void) {
    unsigned latch = vmu_irq_pending(), mask = vmu_irq_mask();
    calls++;
    if (latch & mask & VMU_LINE_SOUND) {
        vmu_irq_ack((uint16_t)handle(latch, mask, "f"));
        if (narrow_mode && sound_left == 0) {
            vmu_sound_stop();
            vmu_irq_disable(VMU_LINE_SOUND);
        }
    }
    vmu_irq_ack((uint16_t)handle(latch, mask, "i"));
}

static void psk_timer(unsigned t, unsigned reload, unsigned prescale, unsigned run) {
    psk_w32(0x0A800000u + t * 16u, reload);
    psk_w32(0x0A800008u + t * 16u, (run << 2) | prescale);
}

static void start(int s) {
    side = s;
    idles = 0;
    if (s == 0) {
        psk_host_init(card, kram);
        psk_swi(0x04u, 5u, 0, 0);
        psk_swi(0x01u, 1u, PSK_IRQ_FN, 0);
        psk_swi(0x01u, 2u, PSK_FIQ_FN, 0);
    } else {
        vmu_host_init(card, 0x1000u, card, 0x1000u);
        vmu_set_handler(vmu_handler);
    }
}

static void idle(void) {
    if (side == 0) {
        psk_idle();
    } else {
        vmu_idle();
    }
    idles++;
}

static void frame_ticks(void) {
    if (side == 0) {
        psk_timer(1, 0x3D09u, 0, 1);
    } else {
        vmu_tick_mode(VMU_TICK_FRAME);
    }
}

static void enable(unsigned lines) {
    if (side == 0) {
        psk_w32(0x0A000010u, lines);
        psk_w32(0x0A000008u, lines);
    } else {
        vmu_irq_ack((uint16_t)lines);
        vmu_irq_enable((uint16_t)lines);
    }
}

static void disable(unsigned lines) {
    if (side == 0) {
        psk_w32(0x0A00000Cu, lines);
    } else {
        vmu_irq_disable((uint16_t)lines);
    }
}

static unsigned pending(void) { return side == 0 ? psk_r32(0x0A000000u) : vmu_irq_pending(); }

static unsigned levels(void) { return side == 0 ? psk_r32(0x0A000004u) : vmu_irq_levels(); }

static void masked(void) {
    for (int s = 0; s < 2; s++) {
        start(s);
        frame_ticks();
        enable(0x100u);
        disable(0x100u);
        for (int i = 0; i < 4; i++) {
            idle();
            note("p", pending() & 0x100u);
        }
        if (side == 0) {
            psk_w32(0x0A000008u, 0x100u);
        } else {
            vmu_irq_enable(0x100u);
        }
        for (int i = 0; i < 3; i++) idle();
        note("p", pending() & 0x100u);
    }
}

static void narrow_sound(unsigned samples, int fast) {
    enable(0x2000u);
    if (side == 0) {
        if (fast) psk_swi(0x04u, 6u, 0, 0);
        sound_left = samples + 1u;
        psk_timer(2, fast ? 0x47u : 500u, 0, 1);
    } else {
        if (fast) vmu_tick_mode(VMU_TICK_FAST);
        sound_left = 1u;
        vmu_sound_play(samples, fast ? VMU_SOUND_FAST : VMU_SOUND_SLOW);
    }
}

static void ticks_for(const char *what) {
    if (!strcmp(what, "standby") || !strcmp(what, "rtcsame")) {
        if (side == 0) {
            psk_swi(0x04u, 2u, 0, 0);
            psk_timer(1, 0x7A1u, 0, 1);
        } else {
            vmu_tick_mode(VMU_TICK_STANDBY);
        }
    } else if (!strcmp(what, "fastsame")) {
        if (side == 0) {
            psk_swi(0x04u, 6u, 0, 0);
            psk_timer(1, 0x3D09u, 0, 1);
        } else {
            vmu_tick_mode(VMU_TICK_FAST);
        }
    } else {
        frame_ticks();
    }
}

static void narrow(const char *what) {
    int same = !strcmp(what, "same") || !strcmp(what, "fastsame"), rtcsame = !strcmp(what, "rtcsame");
    int total = rtcsame ? 16300 : same ? 600 : 1500;
    narrow_mode = 1;
    for (int s = 0; s < 2; s++) {
        int seen = 0, waiting = 0, waited = 0;
        unsigned rtc, now;
        start(s);
        ticks_for(what);
        enable(0x100u);
        if (!strcmp(what, "same")) narrow_sound(15625u, 0);
        if (!strcmp(what, "fastsame")) narrow_sound(7812u, 1);
        rtc = levels() & 0x200u;
        while (seen < total) {
            letters_n = 0;
            calls = 0;
            idle();
            now = levels() & 0x200u;
            if (now && !rtc) letter('r');
            if (rtcsame && calls) letter((char)('0' + calls));
            letters[letters_n] = 0;
            if (letters_n) emit("%d%s ", seen, letters, 0);
            if (letters_n || now != rtc) seen++;
            if (waiting && strchr(letters, 's')) {
                waiting = 0;
                if (s == 0) {
                    psk_swi(0x04u, 5u, 0, 0);
                } else {
                    vmu_tick_mode(VMU_TICK_FRAME);
                }
            }
            if (!strcmp(what, "slow") && seen == 3 && letters_n) narrow_sound(37u, 0);
            if (!strcmp(what, "waited") && now && !rtc && !waited) {
                waiting = waited = 1;
                narrow_sound(1000u, 1);
            }
            rtc = now;
        }
    }
    narrow_mode = 0;
}

static int buttons(unsigned mask) {
    vmu_host_init(card, 0x1000u, card, 0x1000u);
    vmu_irq_enable(0x05u);
    vmu_host_set_buttons((uint8_t)mask);
    printf("%02X %02X %02X\n", vmu_buttons(), vmu_irq_levels() & 0x1Fu, vmu_irq_pending() & 0x1Fu);
    vmu_host_set_buttons(0);
    printf("%02X %02X %02X\n", vmu_buttons(), vmu_irq_levels() & 0x1Fu, vmu_irq_pending() & 0x1Fu);
    return 0;
}

static int buzz(void) {
    uint32_t n;
    vmu_host_init(card, 0x1000u, card, 0x1000u);
    vmu_buzz(5u);
    vmu_tick_mode(VMU_TICK_FRAME);
    vmu_irq_enable(VMU_LINE_TICK);
    vmu_idle();
    vmu_idle();
    vmu_lcd(0);
    vmu_buzz(0);
    vmu_idle();
    vmu_lcd(7u);
    const VmuBuzz *log = vmu_host_buzz_log(&n);
    for (uint32_t i = 0; i < n; i++) printf("%u %c %u\n", (unsigned)log[i].idle, log[i].lcd ? 'l' : 'b', (unsigned)log[i].value);
    return 0;
}

static int save(void) {
    static unsigned char rom[0x1000], sav[0x1000];
    for (unsigned i = 0; i < sizeof rom; i++) {
        rom[i] = (unsigned char)(i * 7u);
        sav[i] = (unsigned char)(i * 13u + 1u);
    }
    vmu_host_init(rom, sizeof rom, sav, sizeof sav);
    for (unsigned i = 0; i < 0x80u; i++) vmu_w8((uint16_t)(0x100u + i), (uint8_t)(~(i * 7u) ^ 0x5Au));
    unsigned ok = vmu_save_write(3u, 0x100u), bad = vmu_save_write(0x20u, 0x100u);
    const uint8_t *f = vmu_host_save();
    unsigned changed = 0, first = 0xFFFFu, last = 0, romdiff = 0;
    for (unsigned i = 0; i < sizeof sav; i++) {
        if (vmu_rom8((uint16_t)i) != rom[i]) romdiff++;
        if (f[i] == sav[i]) continue;
        changed++;
        if (first == 0xFFFFu) first = i;
        last = i;
    }
    unsigned rd = vmu_save_read(3u, 0x200u), back = 1, rdbad = vmu_save_read(0x20u, 0x200u);
    for (unsigned i = 0; i < 0x80u; i++) back &= vmu_r8((uint16_t)(0x200u + i)) == vmu_r8((uint16_t)(0x100u + i));
    printf("ok %u bad %u changed %u first %X last %X rom %u read %u back %u readbad %u\n", ok, bad, changed, first, last, romdiff, rd,
           back, rdbad);
    return 0;
}

static int frames;

static void count_frame(void) { frames++; }

static int frame(void) {
    vmu_host_init(card, 0x1000u, card, 0x1000u);
    vmu_host_set_frame(count_frame);
    uint8_t *fb = vmu_fb();
    memset(fb, 0xA5, VMU_FB_BYTES);
    vmu_present();
    printf("bytes %d frames %d fb %02X\n", VMU_FB_BYTES, frames, vmu_fb()[VMU_FB_BYTES - 1]);
    return 0;
}

static int print_both(void) {
    printf("psk %s\nvmu %s\ncounts %llu\n", seq[0], seq[1], (unsigned long long)vmu_host_time());
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    if (!strcmp(argv[1], "narrow") && argc == 3) {
        narrow(argv[2]);
        return print_both();
    }
    if (!strcmp(argv[1], "masked")) {
        masked();
        return print_both();
    }
    if (!strcmp(argv[1], "buttons") && argc == 3) return buttons((unsigned)strtoul(argv[2], 0, 16));
    if (!strcmp(argv[1], "save")) return save();
    if (!strcmp(argv[1], "buzz")) return buzz();
    if (!strcmp(argv[1], "frame")) return frame();
    return 2;
}
