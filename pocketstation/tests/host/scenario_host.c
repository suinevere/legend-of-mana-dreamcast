/*----------------------
 | scenario_host.c
 | Description: Runs a compiled scenario (scenario.py compile) on vmu_host.c and prints its event log, the oracle rt_test's RT_LOG must match. Its handler logs h with the latched, enabled lines it finds and acknowledges them; idle N logs i 1..N; a sound's rate is 0 slow or 1 fast. Press, release and dock ops apply at the start of their numbered vmu_idle call or standby, those at 0 before the first op. A present prints p, then f with the frame buffer; the end prints the buzzer log as z lines. Mask and unmask do nothing, as the host has no interrupts. An op not implemented, or a bad argument, stops the run with an error line.
 | Author: suinevere
 | Dependencies: vmu_hw.h, vmu_host.h, vmu_host.c, pocketstation/tools/scenario.py
 | Globals: N/A
 | Params: argv[1] -- the compiled scenario; argv[2], argv[3] -- optionally the tables and the pet save (scenario.py table and pet)
 | Returns: 0 at the end op, 1 on a bad file, 2 on an op not implemented or a bad argument, 5 from vmu_power's exits
 ----------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "vmu_host.h"

enum {
    OP_END = 0x00,
    OP_TICK = 0x01,
    OP_SOUND = 0x02,
    OP_SOUND_STOP = 0x03,
    OP_ENABLE = 0x04,
    OP_DISABLE = 0x05,
    OP_ACK = 0x06,
    OP_IDLE = 0x07,
    OP_PRESS = 0x08,
    OP_RELEASE = 0x09,
    OP_DOCK = 0x0A,
    OP_SAVE_WRITE = 0x0B,
    OP_SAVE_READ = 0x0C,
    OP_ROM = 0x0D,
    OP_DRAW = 0x0E,
    OP_PRESENT = 0x0F,
    OP_BUZZ = 0x10,
    OP_LCD = 0x11,
    OP_POWER = 0x12,
    OP_LOG_RAM = 0x13,
    OP_RAM_WRITE = 0x14,
    OP_LOG_BUTTONS = 0x15,
    OP_LOG_SYS = 0x16,
    OP_LOG_LEVELS = 0x17,
    OP_LOG_RUN = 0x18,
    OP_FILL = 0x19,
    OP_MASK = 0x1A,
    OP_UNMASK = 0x1B,
    OP_COUNT = 0x1C
};

/*----------------------
 | ARG_BYTES
 | Description: Argument bytes after each opcode, as scenario.py's OPS table encodes them.
 | Author: suinevere
 | Dependencies: pocketstation/tools/scenario.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const uint8_t ARG_BYTES[OP_COUNT] = {0, 1, 6, 0, 2, 2, 2, 2, 3, 3, 3, 4, 4, 2, 3, 0, 1, 1, 1, 2, 3, 0, 0, 0, 3, 4, 0, 0};

/*----------------------
 | Input
 | Description: A press, release or dock op: its opcode, mask or dock state, and the vmu_idle call it applies at.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct { uint8_t op, value; uint16_t idle; } Input;

static uint8_t g_scn[0x10000];
static uint32_t g_len, g_pos;
static uint8_t g_rom[0x10000], g_save[0x10000];
static uint32_t g_rom_len, g_save_len;
static const uint8_t g_none[1];
static Input g_in[0x4000];
static uint32_t g_in_count;
static uint32_t g_idles;
static uint8_t g_held;

static uint32_t take(uint32_t width) {
    uint32_t v = 0;
    if (g_pos + width > g_len) {
        fprintf(stderr, "scenario_host: scenario ends inside an op at %u\n", (unsigned)g_pos);
        exit(1);
    }
    while (width--) v = (v << 8) | g_scn[g_pos++];
    return v;
}

/*----------------------
 | load
 | Description: Reads a whole file into a buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- the file; buf -- destination; cap -- its size
 | Returns: bytes read; exits 1 when the file cannot be opened
 ----------------------*/
static uint32_t load(const char *path, uint8_t *buf, uint32_t cap) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "scenario_host: cannot open %s\n", path);
        exit(1);
    }
    uint32_t n = (uint32_t)fread(buf, 1, cap, f);
    fclose(f);
    return n;
}

/*----------------------
 | scan_inputs
 | Description: Collects every press, release and dock op of the scenario in file order, stopping on an unknown op.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_scn, g_len, g_in, g_in_count
 | Params: N/A
 | Returns: 0, or 1 on an unknown op or a scenario ending inside one
 ----------------------*/
static int scan_inputs(void) {
    for (uint32_t at = 0; at < g_len;) {
        uint8_t op = g_scn[at];
        if (op == OP_END) return 0;
        if (op >= OP_COUNT || at + 1u + ARG_BYTES[op] > g_len) return 1;
        if (op == OP_PRESS || op == OP_RELEASE || op == OP_DOCK) {
            Input *e = &g_in[g_in_count++];
            e->op = op;
            e->value = g_scn[at + 1u];
            e->idle = (uint16_t)((g_scn[at + 2u] << 8) | g_scn[at + 3u]);
        }
        at += 1u + ARG_BYTES[op];
    }
    return 1;
}

/*----------------------
 | apply_inputs
 | Description: Applies, in file order, the input ops numbered for one vmu_idle call or standby: buttons through vmu_host_set_buttons, the dock through vmu_host_set_dock.
 | Author: suinevere
 | Dependencies: vmu_host_set_buttons, vmu_host_set_dock
 | Globals: g_in, g_in_count, g_held
 | Params: idle -- the call's number, 0 before the first op
 | Returns: N/A
 ----------------------*/
static void apply_inputs(uint32_t idle) {
    for (uint32_t i = 0; i < g_in_count; i++) {
        const Input *e = &g_in[i];
        if (e->idle != idle) continue;
        if (e->op == OP_DOCK) {
            vmu_host_set_dock(e->value);
            continue;
        }
        g_held = (uint8_t)(e->op == OP_PRESS ? g_held | e->value : g_held & ~e->value);
        vmu_host_set_buttons(g_held);
    }
}

static void handler(void) {
    uint16_t lines = vmu_irq_pending() & vmu_irq_mask();
    printf("h %04X\n", lines);
    vmu_irq_ack(lines);
}

static int bad(uint32_t op) {
    printf("error %02X\n", (unsigned)op);
    fprintf(stderr, "scenario_host: op 0x%02X is not implemented or has a bad argument\n", (unsigned)op);
    return 2;
}

/*----------------------
 | end_log
 | Description: Prints the buzzer log at the end op: z, the idles begun before the call, 1 for vmu_lcd or 0 for vmu_buzz, and the tone or on.
 | Author: suinevere
 | Dependencies: vmu_host_buzz_log
 | Globals: N/A
 | Params: N/A
 | Returns: 0
 ----------------------*/
static int end_log(void) {
    uint32_t n;
    const VmuBuzz *log = vmu_host_buzz_log(&n);
    for (uint32_t i = 0; i < n; i++) printf("z %04X %u %02X\n", (unsigned)log[i].idle, log[i].lcd, log[i].value);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2 && argc != 4) {
        fprintf(stderr, "usage: scenario_host SCENARIO.bin [TABLES PET]\n");
        return 1;
    }
    g_len = load(argv[1], g_scn, sizeof g_scn);
    if (argc == 4) {
        g_rom_len = load(argv[2], g_rom, sizeof g_rom);
        g_save_len = load(argv[3], g_save, sizeof g_save);
    }
    if (scan_inputs()) {
        fprintf(stderr, "scenario_host: bad scenario\n");
        return 1;
    }
    vmu_host_init(g_rom_len ? g_rom : g_none, g_rom_len, g_save_len ? g_save : g_none, g_save_len);
    vmu_set_handler(handler);
    apply_inputs(0);
    for (;;) {
        uint32_t op = take(1);
        if (op == OP_END) return end_log();
        if (op == OP_RAM_WRITE) {
            uint16_t off = (uint16_t)take(2);
            vmu_w8(off, (uint8_t)take(1));
        } else if (op == OP_LOG_RAM) {
            uint16_t off = (uint16_t)take(2);
            printf("m %03X %02X\n", off, vmu_r8(off));
        } else if (op == OP_LOG_RUN) {
            uint16_t off = (uint16_t)take(2);
            uint32_t n = take(1);
            for (uint32_t i = 0; i < n; i++) printf("m %03X %02X\n", (unsigned)(off + i), vmu_r8((uint16_t)(off + i)));
        } else if (op == OP_FILL) {
            uint16_t off = (uint16_t)take(2);
            uint32_t n = take(1), start = take(1);
            for (uint32_t i = 0; i < n; i++) vmu_w8((uint16_t)(off + i), (uint8_t)(start + i));
        } else if (op == OP_TICK) {
            uint32_t mode = take(1);
            if (mode > VMU_TICK_STANDBY) return bad(op);
            vmu_tick_mode((uint8_t)mode);
        } else if (op == OP_SOUND) {
            uint32_t samples = take(4), rate = take(2);
            if (samples > 0xFFFFFFu || rate > 1) return bad(op);
            vmu_sound_play(samples, rate ? VMU_SOUND_FAST : VMU_SOUND_SLOW);
        } else if (op == OP_SOUND_STOP) {
            vmu_sound_stop();
        } else if (op == OP_ENABLE) {
            vmu_irq_enable((uint16_t)take(2));
        } else if (op == OP_DISABLE) {
            vmu_irq_disable((uint16_t)take(2));
        } else if (op == OP_ACK) {
            vmu_irq_ack((uint16_t)take(2));
        } else if (op == OP_IDLE) {
            uint32_t n = take(2);
            for (uint32_t i = 1; i <= n; i++) {
                apply_inputs(++g_idles);
                vmu_idle();
                printf("i %04X\n", (unsigned)i);
            }
        } else if (op == OP_PRESS || op == OP_RELEASE || op == OP_DOCK) {
            take(3);
        } else if (op == OP_MASK || op == OP_UNMASK) {
        } else if (op == OP_LOG_BUTTONS) {
            printf("b %02X\n", vmu_buttons());
        } else if (op == OP_LOG_SYS) {
            printf("s %02X\n", vmu_sys_flags());
        } else if (op == OP_LOG_LEVELS) {
            printf("l %04X\n", vmu_irq_levels());
        } else if (op == OP_SAVE_WRITE || op == OP_SAVE_READ) {
            uint16_t sector = (uint16_t)take(2), off = (uint16_t)take(2);
            uint8_t status = op == OP_SAVE_WRITE ? vmu_save_write(sector, off) : vmu_save_read(sector, off);
            printf("%c %04X %02X\n", op == OP_SAVE_WRITE ? 'w' : 'v', sector, status);
        } else if (op == OP_ROM) {
            printf("r %02X\n", vmu_rom8((uint16_t)take(2)));
        } else if (op == OP_DRAW) {
            uint32_t row = take(1), col = take(1), value = take(1);
            if (row >= 32 || col >= 6) return bad(op);
            vmu_fb()[row * 6 + col] = (uint8_t)value;
        } else if (op == OP_PRESENT) {
            vmu_present();
            printf("p\nf ");
            for (int i = 0; i < VMU_FB_BYTES; i++) printf("%02X", vmu_fb()[i]);
            printf("\n");
        } else if (op == OP_BUZZ) {
            vmu_buzz((uint8_t)take(1));
        } else if (op == OP_LCD) {
            vmu_lcd((uint8_t)take(1));
        } else if (op == OP_POWER) {
            uint32_t mode = take(1);
            if (mode < VMU_POWER_STANDBY || mode > VMU_POWER_EXIT_DOCKED) return bad(op);
            if (mode == VMU_POWER_STANDBY) {
                apply_inputs(++g_idles);
            } else {
                printf("x %02X\n", (unsigned)mode);
                fflush(stdout);
            }
            vmu_power((uint8_t)mode);
        } else {
            return bad(op);
        }
    }
}
