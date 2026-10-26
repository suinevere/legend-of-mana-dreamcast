/*----------------------
 | vmu_runner.c
 | Description: Runs the VMU-shaped Ring Ring Land on vmu_host.c under the same scripts and entry states as game_host, writing per-frame state dumps, the pet save at the end and, in counting builds, RAM access counts; in recording builds, sampled calls of every game function.
 | Author: suinevere
 | Dependencies: vmu_host.h, vmu_hw.h, game.h, vmu_record.h (VMU_RECORD builds)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "vmu_host.h"
#ifdef VMU_RECORD
#include "vmu_record.h"
#endif

/*----------------------
 | MAX_STEPS / MAX_BOOTS / FILE_MAX
 | Description: Limits for an expanded script, for the boots an entry-state file can hold, and for the game's file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MAX_STEPS 200000
#define MAX_BOOTS 8
#define FILE_MAX 0x10000u

/*----------------------
 | Step
 | Description: One expanded script step: a sample with a button mask, an RTC setting (ignored, the game reads no calendar), or a reboot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    enum { SAMPLE, RTC, REBOOT } kind;
    uint32_t a;
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
 | Description: The run: script steps, position, samples, entry states, the game's file as tables and as the pet save carried across reboots, the state dump and the escape back to main.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static Step g_steps[MAX_STEPS];
static int g_count, g_next;
static uint32_t g_samples;
static Entry g_entries[MAX_BOOTS];
static int g_boots;
static uint8_t g_file[FILE_MAX];
static uint8_t g_save[FILE_MAX];
static uint32_t g_file_len;
static FILE *g_state;
static jmp_buf g_top;

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
    fprintf(stderr, "vmu_runner: %s %s\n", what, arg);
    exit(2);
}

/*----------------------
 | vmu_index_fault
 | Description: The VMU_CHECK_BOUNDS report of an array index past its RAM field.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: field -- RAM field; i -- index
 | Returns: does not return
 ----------------------*/
void vmu_index_fault(const char *field, unsigned i) {
    fprintf(stderr, "vmu_runner: index %u past %s at sample %u\n", i, field, g_samples);
    exit(8);
}

/*----------------------
 | push
 | Description: Adds one expanded step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_steps, g_count
 | Params: kind, a -- the step
 | Returns: N/A
 ----------------------*/
static void push(int kind, uint32_t a) {
    if (g_count == MAX_STEPS) fail("script too long", "");
    g_steps[g_count].kind = kind;
    g_steps[g_count].a = a;
    g_count++;
}

/*----------------------
 | load_script
 | Description: Parses and expands a .pks script as pkrun does: wait, hold, press, rtc, reboot, end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_steps
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
            for (unsigned i = 0; i < b; i++) push(SAMPLE, 0);
        } else if (!strcmp(op, "hold") && sscanf(line, "%*s %x %u", &a, &b) == 2) {
            for (unsigned i = 0; i < b; i++) push(SAMPLE, a);
        } else if (!strcmp(op, "press") && sscanf(line, "%*s %x", &a) == 1) {
            push(SAMPLE, a);
            push(SAMPLE, 0);
        } else if (!strcmp(op, "reboot")) {
            push(REBOOT, 0);
        } else if (!strcmp(op, "rtc")) {
            push(RTC, 0);
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
 | load_file
 | Description: Builds the game's file from a 128 KB card: the app's 8 KB blocks in the order the directory chains them from block 1, as psk_host_init maps them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_file, g_file_len
 | Params: path -- card image
 | Returns: N/A
 ----------------------*/
static void load_file(const char *path) {
    static uint8_t card[0x20000];
    FILE *f = fopen(path, "rb");
    if (!f || fread(card, 1, sizeof card, f) != sizeof card) fail("cannot read a 128 KB card from", path);
    fclose(f);
    for (unsigned b = 1, n = 0; b >= 1 && b <= 15 && n < 15; n++) {
        if (g_file_len + 0x2000u > FILE_MAX) fail("file longer than 64 KB in", path);
        memcpy(g_file + g_file_len, card + b * 0x2000u, 0x2000u);
        g_file_len += 0x2000u;
        unsigned link = card[b * 0x80u + 8u] | (card[b * 0x80u + 9u] << 8);
        if (link == 0xFFFFu) break;
        b = link + 1u;
    }
}

/*----------------------
 | word_at
 | Description: A little-endian word of an entry's work RAM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ram -- work RAM; a -- address
 | Returns: the word
 ----------------------*/
static uint32_t word_at(const uint8_t *ram, uint32_t a) {
    return ram[a] | (ram[a + 1] << 8) | (ram[a + 2] << 16) | ((uint32_t)ram[a + 3] << 24);
}

/*----------------------
 | sys_flags
 | Description: The system flags an entry's kernel RAM gives: mute and quit from the communication flags at 0xC0 (bits 0 and 8), snd_mode from bits 18-19 of the user-interface status at 0xD8.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ram -- the entry's work RAM
 | Returns: VMU_SYS_* bits
 ----------------------*/
static uint8_t sys_flags(const uint8_t *ram) {
    uint32_t com = word_at(ram, 0xC0u), ui = word_at(ram, 0xD8u);
    return (uint8_t)(((com & 1u) ? VMU_SYS_MUTE : 0u) | ((com & 0x100u) ? VMU_SYS_QUIT : 0u) | (((ui >> 18) & 3u) << 2));
}

/*----------------------
 | advance
 | Description: Skips RTC steps up to the next sample (a reboot or the end leave through g_top) and sets its buttons.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_steps, g_next, g_count, g_top
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void advance(void) {
    while (g_next < g_count && g_steps[g_next].kind != SAMPLE) {
        if (g_steps[g_next++].kind == REBOOT) longjmp(g_top, 2);
    }
    if (g_next >= g_count) longjmp(g_top, 1);
    vmu_host_set_buttons((uint8_t)g_steps[g_next].a);
}

/*----------------------
 | frame
 | Description: The frame point: the frame's state when dumping (vmu_ram, then the frame buffer), then the next step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_state, g_samples, g_next
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void frame(void) {
    if (g_state && (fwrite(vmu_ram, 1, sizeof vmu_ram, g_state) != sizeof vmu_ram || fwrite(vmu_fb(), 1, VMU_FB_BYTES, g_state) != VMU_FB_BYTES)) {
        fail("cannot write the state", "");
    }
    g_samples++;
#ifdef VMU_RECORD
    rec_set_frame(g_samples);
#endif
    g_next++;
    advance();
}

/*----------------------
 | boot
 | Description: Starts boot k from its entry state with the file as the tables and the save carried from the last boot, and runs the game's entry until a step leaves.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_entries, g_file, g_save
 | Params: k -- boot index
 | Returns: does not return
 ----------------------*/
static void boot(int k) {
    if (k >= g_boots) fail("no entry state for a reboot", "");
#ifdef VMU_RECORD
    rec_reset();
#endif
    vmu_host_init(g_file, g_file_len, g_save, g_file_len);
    vmu_host_set_entry(g_entries[k].words, g_entries[k].n);
    vmu_host_set_sys(sys_flags(g_entries[k].wram));
    vmu_host_set_frame(frame);
    advance();
    game_entry();
    fail("the game returned", "");
}

/*----------------------
 | save
 | Description: Writes bytes to a file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- file; p -- bytes; n -- count
 | Returns: N/A
 ----------------------*/
static void save(const char *path, const void *p, size_t n) {
    FILE *f = fopen(path, "wb");
    if (!f || fwrite(p, 1, n, f) != n) fail("cannot write", path);
    fclose(f);
}

#ifdef VMU_RECORD
/*----------------------
 | script_name
 | Description: A script path's base name without its .pks, the name records carry.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- script path; out -- buffer; size -- its size
 | Returns: N/A
 ----------------------*/
static void script_name(const char *path, char *out, size_t size) {
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    snprintf(out, size, "%s", base);
    char *dot = strrchr(out, '.');
    if (dot) *dot = 0;
}
#endif

/*----------------------
 | main
 | Description: vmu_runner --card C --script S --entry E [--state OUT.state] [--file-out F] [--counts F] [--record DIR].
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_top, g_file, g_save
 | Params: argc, argv -- command line
 | Returns: 0, or 2 on a usage or input error
 ----------------------*/
int main(int argc, char **argv) {
    const char *card = 0, *script = 0, *entry = 0, *state = 0, *file_out = 0, *counts = 0, *record = 0;
    volatile int k = 0;
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "--card")) card = argv[i + 1];
        else if (!strcmp(argv[i], "--script")) script = argv[i + 1];
        else if (!strcmp(argv[i], "--entry")) entry = argv[i + 1];
        else if (!strcmp(argv[i], "--state")) state = argv[i + 1];
        else if (!strcmp(argv[i], "--file-out")) file_out = argv[i + 1];
        else if (!strcmp(argv[i], "--counts")) counts = argv[i + 1];
        else if (!strcmp(argv[i], "--record")) record = argv[i + 1];
        else fail("unknown option", argv[i]);
    }
    if (!card || !script || !entry) fail("usage:", "vmu_runner --card C --script S --entry E [--state F] [--file-out F] [--counts F] [--record DIR]");
    load_file(card);
    memcpy(g_save, g_file, g_file_len);
    if (state && !(g_state = fopen(state, "wb"))) fail("cannot write", state);
    load_script(script);
    load_entries(entry);
#ifdef VMU_RECORD
    if (record) {
        char name[64];
        script_name(script, name, sizeof name);
        rec_begin(record, name, 0);
    }
#else
    if (record) fail("--record needs a VMU_RECORD build", "");
#endif
    int how = setjmp(g_top);
    if (how == 2) {
        k++;
        memcpy(g_save, vmu_host_save(), g_file_len);
    }
    if (how != 1) boot(k);
    memcpy(g_save, vmu_host_save(), g_file_len);
    if (g_state) fclose(g_state);
    if (file_out) save(file_out, g_save, g_file_len);
#ifdef VMU_COUNT_ACCESS
    if (counts) save(counts, vmu_host_counts(), 0x400u * sizeof(uint32_t));
#else
    if (counts) fail("--counts needs a VMU_COUNT_ACCESS build", "");
#endif
#ifdef VMU_RECORD
    rec_finish();
#endif
    printf("vmu_runner: samples %u boots %d\n", g_samples, k + 1);
    return 0;
}
