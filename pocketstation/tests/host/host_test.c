/*----------------------
 | host_test.c
 | Description: Drives psk_host.c without any decompiled code: LCD, buttons, RTC, and a replay of a reference run's SWI log.
 | Author: suinevere
 | Dependencies: psk_host.h, psk_hw.h, psk_swi.h
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

const PskHookEntry psk_hooks[] = {{0u, 0, 0, 0, 0, 0u}};
const int psk_hook_count = 0;

static unsigned char card[0x20000];
static unsigned char kram[0x200];

static void load(const char *path, unsigned char *buf, size_t n) {
    FILE *f = fopen(path, "rb");
    if (!f || fread(buf, 1, n, f) != n) {
        fprintf(stderr, "host_test: cannot read %s\n", path);
        exit(2);
    }
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    if (!strcmp(argv[1], "lcd")) {
        psk_host_init(card, kram);
        for (unsigned row = 0; row < 32; row++) psk_w32(0x0D000100u + 4u * row, 0x80000001u | (row << 8));
        const unsigned char *v = psk_host_vram();
        for (int i = 0; i < 128; i++) printf("%02X", v[i]);
        printf("\n");
        return 0;
    }
    if (!strcmp(argv[1], "buttons")) {
        psk_host_init(card, kram);
        psk_host_set_buttons((unsigned)strtoul(argv[2], 0, 16));
        printf("%02X\n", psk_r32(0x0A000004u) & 0x1Fu);
        return 0;
    }
    if (!strcmp(argv[1], "rtc")) {
        psk_host_init(card, kram);
        psk_host_set_rtc((unsigned)strtoul(argv[2], 0, 16), (unsigned)strtoul(argv[3], 0, 16));
        printf("%08X %08X\n", psk_r32(0x0B800008u), psk_r32(0x0B80000Cu));
        psk_host_advance(1.0);
        printf("%08X %08X\n", psk_r32(0x0B800008u), psk_r32(0x0B80000Cu));
        psk_host_advance(59.0);
        printf("%08X %08X\n", psk_r32(0x0B800008u), psk_r32(0x0B80000Cu));
        return 0;
    }
    if (!strcmp(argv[1], "replay")) {
        load(argv[3], card, sizeof card);
        load(argv[4], kram, sizeof kram);
        psk_host_init(card, kram);
        FILE *f = fopen(argv[2], "r");
        char line[1024];
        int calls = 0, bad = 0;
        while (f && fgets(line, sizeof line, f)) {
            unsigned n, r0, r1, r2, out;
            int used = 0;
            if (sscanf(line, "%x %x %x %x %x%n", &n, &r0, &r1, &r2, &out, &used) != 5) continue;
            char *p = line + used;
            unsigned b;
            int k;
            for (unsigned i = 0; sscanf(p, "%x%n", &b, &k) == 1; i++, p += k) psk_w8(r1 + i, (uint8_t)b);
            unsigned got = psk_swi(n, r0, r1, r2);
            calls++;
            if (got != out) {
                bad++;
                printf("mismatch swi %02X: host %08X reference %08X\n", n, got, out);
            }
        }
        printf("calls %d mismatches %d card %08X\n", calls, bad, psk_host_card_crc());
        return 0;
    }
    return 2;
}
