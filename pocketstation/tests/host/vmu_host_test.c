/*----------------------
 | vmu_host_test.c
 | Description: Drives vmu_host.c and, as its oracle, psk_host.c through the same interrupt scenarios, printing each backend's delivery sequence; also checks buttons, the file write and the frame buffer.
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
#define TIMER_LINES 0x2180u

static void psk_irq_fn(PskCpu *c);
static void psk_fiq_fn(PskCpu *c);

const PskHookEntry psk_hooks[] = {
    {PSK_IRQ_FN, psk_irq_fn, "irq", 1, 0, 0u},
    {PSK_FIQ_FN, psk_fiq_fn, "fiq", 1, 0, 0u},
};
const int psk_hook_count = 2;

static unsigned char card[0x20000];
static unsigned char kram[0x200];
static char seq[2][1 << 16];
static size_t seq_len[2];
static int side, nest, nested, idles;

static void note(const char *what, unsigned lines) {
    size_t *n = &seq_len[side];
    *n += (size_t)snprintf(seq[side] + *n, sizeof seq[side] - *n, "%d%s%X ", idles, what, lines);
    if (*n >= sizeof seq[side] - 32) {
        fprintf(stderr, "vmu_host_test: sequence overflow\n");
        exit(3);
    }
}

static void psk_handle(PskCpu *c, const char *what) {
    unsigned lines = psk_r32(0x0A000000u) & psk_r32(0x0A000008u) & TIMER_LINES;
    note(what, lines);
    psk_w32(0x0A000010u, lines);
    if (nest && !nested) {
        nested = 1;
        psk_idle();
        note("-", 0);
    }
    c->r[15] = c->r[14];
}

static void psk_irq_fn(PskCpu *c) { psk_handle(c, "i"); }

static void psk_fiq_fn(PskCpu *c) { psk_handle(c, "f"); }

static void vmu_handle(const char *what) {
    unsigned lines = vmu_irq_pending() & vmu_irq_mask() & TIMER_LINES;
    note(what, lines);
    vmu_irq_ack((uint16_t)lines);
    if (nest && !nested) {
        nested = 1;
        vmu_idle();
        note("-", 0);
    }
}

static void vmu_irq_fn(void) { vmu_handle("i"); }

static void vmu_fiq_fn(void) { vmu_handle("f"); }

static unsigned line_of(unsigned t) { return t == 0 ? 0x80u : t == 1 ? 0x100u : 0x2000u; }

static void psk_timer(unsigned t, unsigned reload, unsigned prescale, unsigned run) {
    psk_w32(0x0A800000u + t * 16u, reload);
    psk_w32(0x0A800008u + t * 16u, (run << 2) | prescale);
}

static void start(int s) {
    side = s;
    nested = 0;
    idles = 0;
    if (s == 0) {
        psk_host_init(card, kram);
        psk_swi(0x04u, 5u, 0, 0);
        psk_swi(0x01u, 1u, PSK_IRQ_FN, 0);
        psk_swi(0x01u, 2u, PSK_FIQ_FN, 0);
    } else {
        vmu_host_init(card, 0x1000u);
        vmu_set_clock(5u);
        vmu_set_handlers(vmu_irq_fn, vmu_fiq_fn);
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

static void timer(unsigned t, unsigned reload, unsigned prescale, unsigned run) {
    if (side == 0) {
        psk_timer(t, reload, prescale, run);
    } else {
        vmu_set_timer((uint8_t)t, (uint16_t)reload, (uint8_t)prescale, (uint8_t)run);
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

static void set_clock(unsigned level) {
    if (side == 0) {
        psk_swi(0x04u, level, 0, 0);
    } else {
        vmu_set_clock((uint8_t)level);
    }
}

static void schedule(unsigned t, unsigned reload, unsigned prescale, int clock_at, unsigned clock) {
    for (int s = 0; s < 2; s++) {
        start(s);
        timer(t, reload, prescale, 1);
        enable(line_of(t));
        for (int i = 0; i < 3000; i++) {
            if (i == clock_at) set_clock(clock);
            idle();
            if (i % 50 == 0) note(".", 0);
        }
    }
}

static void pair(void) {
    for (int s = 0; s < 2; s++) {
        start(s);
        timer(1, 0x7Bu, 2u, 1);
        timer(0, 0x1F3u, 1u, 1);
        enable(0x180u);
        for (int i = 0; i < 4000; i++) {
            if (i == 1700) set_clock(6);
            idle();
        }
    }
}

static void masked(void) {
    for (int s = 0; s < 2; s++) {
        start(s);
        timer(1, 40u, 1u, 1);
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

static void nesting(void) {
    nest = 1;
    for (int s = 0; s < 2; s++) {
        for (int first = 0; first < 2; first++) {
            start(s);
            timer(1, 30u, 1u, 1);
            timer(2, 7u, 1u, 1);
            enable(first ? 0x2100u : 0x100u);
            if (!first) enable(0x2000u);
            for (int i = 0; i < 60; i++) idle();
            note("|", 0);
        }
    }
    nest = 0;
}

static int buttons(unsigned mask) {
    vmu_host_init(card, 0x1000u);
    vmu_irq_enable(0x05u);
    vmu_host_set_buttons((uint8_t)mask);
    printf("%02X %02X %02X\n", vmu_buttons(), vmu_irq_levels() & 0x1Fu, vmu_irq_pending() & 0x1Fu);
    vmu_host_set_buttons(0);
    printf("%02X %02X %02X\n", vmu_buttons(), vmu_irq_levels() & 0x1Fu, vmu_irq_pending() & 0x1Fu);
    return 0;
}

static int file(void) {
    static unsigned char img[0x1000];
    for (unsigned i = 0; i < sizeof img; i++) img[i] = (unsigned char)(i * 7u);
    vmu_host_init(img, sizeof img);
    for (unsigned i = 0; i < 0x80u; i++) vmu_w8((uint16_t)(0x100u + i), (uint8_t)(~(i * 7u) ^ 0x5Au));
    unsigned ok = vmu_file_write(3u, 0x100u), bad = vmu_file_write(0x20u, 0x100u);
    const uint8_t *f = vmu_host_file();
    unsigned changed = 0, first = 0xFFFFu, last = 0;
    for (unsigned i = 0; i < sizeof img; i++) {
        if (f[i] == img[i]) continue;
        changed++;
        if (first == 0xFFFFu) first = i;
        last = i;
    }
    printf("ok %u bad %u changed %u first %X last %X read %02X\n", ok, bad, changed, first, last, vmu_file8(0x180u));
    return 0;
}

static int frames;

static void count_frame(void) { frames++; }

static int frame(void) {
    vmu_host_init(card, 0x1000u);
    vmu_host_set_frame(count_frame);
    uint8_t *fb = vmu_fb();
    memset(fb, 0xA5, VMU_FB_BYTES);
    vmu_present();
    printf("bytes %d frames %d fb %02X\n", VMU_FB_BYTES, frames, vmu_fb()[VMU_FB_BYTES - 1]);
    return 0;
}

static int print_both(void) {
    printf("psk %s\nvmu %s\ncycles %llu\n", seq[0], seq[1], (unsigned long long)vmu_host_cycles());
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    if (!strcmp(argv[1], "schedule") && argc == 7) {
        schedule((unsigned)atoi(argv[2]), (unsigned)strtoul(argv[3], 0, 0), (unsigned)atoi(argv[4]), atoi(argv[5]),
                 (unsigned)atoi(argv[6]));
        return print_both();
    }
    if (!strcmp(argv[1], "pair")) {
        pair();
        return print_both();
    }
    if (!strcmp(argv[1], "masked")) {
        masked();
        return print_both();
    }
    if (!strcmp(argv[1], "nesting")) {
        nesting();
        return print_both();
    }
    if (!strcmp(argv[1], "buttons") && argc == 3) return buttons((unsigned)strtoul(argv[2], 0, 16));
    if (!strcmp(argv[1], "file")) return file();
    if (!strcmp(argv[1], "frame")) return frame();
    return 2;
}
