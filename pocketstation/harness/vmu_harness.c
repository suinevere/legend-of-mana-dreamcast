/*----------------------
 | vmu_harness.c
 | Description: Test machine around the vendored LC86K core: boots a GAME from a 128 KB flash image, models the BIOS flash, page write and exit entries, runs .pks scripts and input events, logs RT_LOG bytes, RT_IDLE_MARK cycles and the buzzer and LCD, dumps frames at RT_PRESENT, runs on an idle or real schedule and reports the cycles each frame and the runtime in it cost.
 | Author: suinevere
 | Dependencies: lc86k/cpu.h, lc86k/cpu.c (patched copy under build/harness/lc86k/)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

/*----------------------
 | FLASH_SIZE / PAGE / RAM_IMAGE / FB_BYTES / BLOCK
 | Description: Flash image size, the BIOS flash page, the vmu_ram image size, the frame buffer size of a dumped frame and the flash block size.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FLASH_SIZE 0x20000
#define PAGE 128
#define RAM_IMAGE 0x400
#define FB_BYTES 192
#define BLOCK 512

/*----------------------
 | DIR_TOP / DIR_BLOCKS / GAME_TYPE / GAME_LAST
 | Description: The flash directory's top block and length, the GAME entry type, and bank 0 address 0x6D where the BIOS keeps the GAME's block count less one.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DIR_TOP 253
#define DIR_BLOCKS 13
#define GAME_TYPE 0xCC
#define GAME_LAST 0x6D

/*----------------------
 | ROM_WRITE / ROM_VERIFY / ROM_READ / ROM_TIMER / ROM_EXIT / ROM_PAGE
 | Description: The BIOS entries the harness models: FM_WRT_EX, FM_VRF_EX, FM_PRD_EX, TIMER_EX, the mode-change exit and the page write the Maple block write uses (american_v1.05.s label_E024); the flash calls return to entry + 5, TIMER_EX to 0x139, and the page write through the ROM address its caller pushed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_WRITE 0x0100
#define ROM_VERIFY 0x0110
#define ROM_READ 0x0120
#define ROM_PAGE 0xE024
#define ROM_TIMER 0x0130
#define ROM_EXIT 0x01F0

/*----------------------
 | SFR_ACC / SFR_PSW / SFR_B / SFR_TRL / SFR_TRH / SFR_SP / SFR_IE / SFR_EXT / SFR_OCR / SFR_T1CNT / SFR_T1LC / SFR_T1LR / SFR_VCCR / SFR_P1 / SFR_P1DDR / SFR_P1FCR / SFR_FPR / SFR_RT_IDLE_MARK / SFR_RT_LOG / SFR_RT_PRESENT
 | Description: SFR offsets the harness writes when booting and in modelled BIOS calls, the Timer 1, port 1 and LCD registers it reads for the buzzer and LCD log, and the three unused SFRs it treats as RT_IDLE_MARK (0x16D), RT_LOG (0x16E) and RT_PRESENT (0x16F).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SFR_ACC 0x00
#define SFR_PSW 0x01
#define SFR_B 0x02
#define SFR_TRL 0x04
#define SFR_TRH 0x05
#define SFR_SP 0x06
#define SFR_IE 0x08
#define SFR_EXT 0x0D
#define SFR_OCR 0x0E
#define SFR_T1CNT 0x18
#define SFR_T1LC 0x1A
#define SFR_T1LR 0x1B
#define SFR_VCCR 0x27
#define SFR_P1 0x44
#define SFR_P1DDR 0x45
#define SFR_P1FCR 0x46
#define SFR_FPR 0x54
#define SFR_BTCR 0x7F
#define SFR_RT_IDLE_MARK 0x6D
#define SFR_RT_LOG 0x6E
#define SFR_RT_PRESENT 0x6F

/*----------------------
 | PS_PER_SECOND / MAX_FIELDS / MAX_EVENTS / MAX_STEPS
 | Description: Picoseconds in a second for the clocks, and the limits on --hw fields, --inputs events and expanded script steps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PS_PER_SECOND 1000000000000ull
#define MAX_FIELDS 512
#define MAX_EVENTS 4096
#define MAX_STEPS 200000

/*----------------------
 | MAX_FRAMES / MAX_LABELS
 | Description: The limits on frames --report keeps and on labels in a --map link map.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MAX_FRAMES 131072
#define MAX_LABELS 4096

/*----------------------
 | RTV_TICK / RTV_STATE / RTV_BASE / RTI_FLAGS / RTI_SLEPT / SFR_C
 | Description: The runtime's bank 1 bytes --report reads: the tick countdown, the state byte (bits 0-1 tick mode, 2-3 clock) and the counts run since the last event from rt_events.s, and rt_input.s's flags with the bit set while Sleep has stopped the schedule; and the C register's SFR offset.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu_rt/rt_events.s, pocketstation/src/vmu_rt/rt_input.s
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RTV_TICK 0x04
#define RTV_STATE 0x14
#define RTV_BASE 0x10
#define RTI_FLAGS 0xFB
#define RTI_SLEPT 0x02
#define SFR_C 0x03

/*----------------------
 | POISON / SFR_XBNK
 | Description: The byte the harness leaves in each register a modelled BIOS routine destroys, so code holding a value across the call fails here as on hardware; and XBNK's SFR offset, which FM_WRT_EX's flash icon leaves at 2.
 | Author: suinevere
 | Dependencies: american_v1.05.s (label_3CB7, label_E348, label_E388, label_E3F6)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define POISON 0xA5
#define SFR_XBNK 0x25

/*----------------------
 | TICK_COUNTS / STANDBY_COUNTS / CLOCK_HZ / T0_CYCLES / LATE_PERIODS
 | Description: A tick period in timer counts (reload + 1) in frame and fast mode and in standby; each clock index's rate (a count being 2 / rate seconds: the launcher's clock 7, then 5, 6 and 2) and Timer 0 period in cycles (rt_ev_p); and the Timer 0 periods a tick may come after its due time before --report calls it late.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu_rt/rt_events.s (rt_ev_tick_load, rt_ev_rtc_edge, rt_ev_p)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define TICK_COUNTS 15626u
#define STANDBY_COUNTS 1954u
static const uint32_t CLOCK_HZ[4] = {3997696u, 1015808u, 1998848u, 126976u};
static const uint32_t T0_CYCLES[4] = {16u, 63u, 32u, 252u};
#define LATE_PERIODS 2u

/*----------------------
 | HwField / InputEvent / Step
 | Description: A vmu_ram field's packed offset and its bank 1 or WRAM place; an input event at an idle index; one expanded script step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct { uint16_t offset; uint16_t addr; uint16_t size; bool wram; } HwField;
typedef struct { long idle; int kind; long value; } InputEvent;
typedef struct { int kind; uint8_t buttons; } Step;
enum { EV_PRESS, EV_RELEASE, EV_DOCK };
enum { STEP_SAMPLE, STEP_REBOOT };

static uint8_t g_flash[FLASH_SIZE];
static uint8_t g_bank;
static bool g_booted;
static bool g_rom_pending;
static uint8_t g_p3 = 0xFF;
static uint8_t g_p7;
static uint64_t g_time;
static uint64_t g_next_base;
static uint64_t g_cpu_period = PS_PER_SECOND / 50000;
static uint64_t g_base_period = PS_PER_SECOND / 32768;
static uint64_t g_cycles;
static uint64_t g_base_ticks, g_timer_calls;
static uint64_t g_steps;
static FILE *g_log;
static FILE *g_dump;
static FILE *g_marks;
static FILE *g_buzz;
static const char *g_out_flash;
static HwField g_fields[MAX_FIELDS];
static int g_field_count;
static InputEvent g_events[MAX_EVENTS];
static int g_event_count, g_event_next;
static bool g_woke;
static Step g_script[MAX_STEPS];
static int g_script_count, g_script_next;
static bool g_scripted, g_reboot_pending, g_end_pending;
static unsigned g_samples;
static long g_idle_index, g_idle_marks;
static bool g_in_halt;
static bool g_report, g_frame_open;
static bool g_rt_code[0x10000];
static uint64_t g_frame_busy, g_frame_rt;
static uint32_t g_busy[MAX_FRAMES], g_rt[MAX_FRAMES], g_period[MAX_FRAMES];
static long g_ev_step = -1;
static uint64_t g_ticks, g_late, g_late_max, g_due, g_idle_start;
static long g_ev_step_at = -1;
static double g_true_counts, g_stepped, g_drift0, g_drift_min, g_drift_max;
static bool g_drift_set;
static uint8_t g_tick_st;
static int g_frames;

/*----------------------
 | fail
 | Description: Prints a harness error and exits with the usage status.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: what -- message; path -- the file or option concerned
 | Returns: does not return
 ----------------------*/
static _Noreturn void fail(const char *what, const char *path)
{
    fprintf(stderr, "vmu_harness: %s %s\n", what, path);
    exit(2);
}

/*----------------------
 | load_flash
 | Description: Reads a flash image, or a raw GAME, into flash from address 0, zero-filling the rest.
 | Author: suinevere
 | Dependencies: fail
 | Globals: g_flash
 | Params: path -- file to load
 | Returns: N/A
 ----------------------*/
static void load_flash(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) fail("cannot open", path);
    memset(g_flash, 0, sizeof g_flash);
    size_t got = fread(g_flash, 1, sizeof g_flash, f);
    (void)got;
    fclose(f);
}

/*----------------------
 | json_number / json_string_is
 | Description: The number after a quoted key in one JSON object's text, or whether the key's value is a given quoted string.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: obj -- object text; key -- quoted key; value -- quoted value
 | Returns: the number, or -1 when the key is missing; true when the value matches
 ----------------------*/
static long json_number(const char *obj, const char *key)
{
    const char *p = strstr(obj, key);
    if (!p || !(p = strchr(p + strlen(key), ':'))) return -1;
    return strtol(p + 1, NULL, 0);
}

static bool json_string_is(const char *obj, const char *key, const char *value)
{
    const char *p = strstr(obj, key);
    if (!p || !(p = strchr(p + strlen(key), ':'))) return false;
    p += strspn(p + 1, " \t\r\n") + 1;
    return strncmp(p, value, strlen(value)) == 0;
}

/*----------------------
 | load_hw
 | Description: Reads gen_ram.py's hardware placement JSON: each field's packed offset, bank 1 or WRAM address and size.
 | Author: suinevere
 | Dependencies: json_number, json_string_is, fail
 | Globals: g_fields, g_field_count
 | Params: path -- the JSON file
 | Returns: N/A
 ----------------------*/
static void load_hw(const char *path)
{
    static char text[1 << 20];
    FILE *f = fopen(path, "rb");
    if (!f) fail("cannot open", path);
    size_t n = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[n] = 0;
    for (char *obj = strchr(text, '{'); obj; obj = strchr(obj + 1, '{')) {
        char *end = strchr(obj, '}');
        if (!end || g_field_count == MAX_FIELDS) fail("bad placement", path);
        *end = 0;
        long offset = json_number(obj, "\"offset\""), addr = json_number(obj, "\"addr\""), size = json_number(obj, "\"size\"");
        if (offset < 0 || addr < 0 || size < 0 || offset + size > RAM_IMAGE) fail("bad field in", path);
        HwField *h = &g_fields[g_field_count++];
        h->offset = (uint16_t)offset;
        h->addr = (uint16_t)addr;
        h->size = (uint16_t)size;
        h->wram = json_string_is(obj, "\"space\"", "\"wram\"");
        obj = end;
    }
}

/*----------------------
 | push_step
 | Description: Adds one expanded script step.
 | Author: suinevere
 | Dependencies: fail
 | Globals: g_script, g_script_count
 | Params: kind -- STEP_SAMPLE or STEP_REBOOT; buttons -- the sample's held buttons
 | Returns: N/A
 ----------------------*/
static void push_step(int kind, unsigned buttons)
{
    if (g_script_count == MAX_STEPS) fail("script too long", "");
    g_script[g_script_count].kind = kind;
    g_script[g_script_count].buttons = (uint8_t)buttons;
    g_script_count++;
}

/*----------------------
 | load_script
 | Description: Parses and expands a .pks script as vmu_runner does: wait N, hold MASK N, press MASK (one sample held, one released), reboot, end; rtc and # comments are ignored.
 | Author: suinevere
 | Dependencies: push_step, fail
 | Globals: g_script, g_scripted
 | Params: path -- the script
 | Returns: N/A
 ----------------------*/
static void load_script(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[256], op[32];
    if (!f) fail("cannot open", path);
    g_scripted = true;
    while (fgets(line, sizeof line, f)) {
        char *hash = strchr(line, '#');
        unsigned a = 0, b = 0;
        if (hash) *hash = 0;
        if (sscanf(line, "%31s", op) != 1) continue;
        if (!strcmp(op, "end")) break;
        if (!strcmp(op, "wait") && sscanf(line, "%*s %u", &b) == 1) {
            for (unsigned i = 0; i < b; i++) push_step(STEP_SAMPLE, 0);
        } else if (!strcmp(op, "hold") && sscanf(line, "%*s %x %u", &a, &b) == 2) {
            for (unsigned i = 0; i < b; i++) push_step(STEP_SAMPLE, a);
        } else if (!strcmp(op, "press") && sscanf(line, "%*s %x", &a) == 1) {
            push_step(STEP_SAMPLE, a);
            push_step(STEP_SAMPLE, 0);
        } else if (!strcmp(op, "reboot")) {
            push_step(STEP_REBOOT, 0);
        } else if (strcmp(op, "rtc") != 0) {
            fail("bad line in", path);
        }
    }
    fclose(f);
}

/*----------------------
 | load_inputs
 | Description: Reads scenario input events, one a line: press MASK IDLE, release MASK IDLE or dock ON IDLE (port 3 bits for press and release), and orders them by idle index, keeping file order within one.
 | Author: suinevere
 | Dependencies: fail
 | Globals: g_events, g_event_count
 | Params: path -- the events file
 | Returns: N/A
 ----------------------*/
static void load_inputs(const char *path)
{
    char op[32];
    long value, idle;
    FILE *f = fopen(path, "r");
    if (!f) fail("cannot open", path);
    while (fscanf(f, "%31s %li %li", op, &value, &idle) == 3) {
        if (g_event_count == MAX_EVENTS) fail("too many events in", path);
        InputEvent *e = &g_events[g_event_count++];
        e->idle = idle;
        e->value = value;
        if (strcmp(op, "press") == 0) e->kind = EV_PRESS;
        else if (strcmp(op, "release") == 0) e->kind = EV_RELEASE;
        else if (strcmp(op, "dock") == 0) e->kind = EV_DOCK;
        else fail("unknown event in", path);
    }
    if (!feof(f)) fail("bad line in", path);
    fclose(f);
    for (int i = 1; i < g_event_count; i++) {
        InputEvent e = g_events[i];
        int j = i;
        for (; j > 0 && g_events[j - 1].idle > e.idle; j--) g_events[j] = g_events[j - 1];
        g_events[j] = e;
    }
}

/*----------------------
 | load_map
 | Description: Reads a link map (vmu_map.py), one "ADDR NAME" line per label in address order, and marks the code from each rt_ label to the next label as the runtime's, the rt_test program's labels excepted, and notes rt_idle_step's and rt_ev_step's addresses for the late-tick and count checks.
 | Author: suinevere
 | Dependencies: fail
 | Globals: g_rt_code, g_ev_step, g_ev_step_at
 | Params: path -- the map
 | Returns: N/A
 ----------------------*/
static void load_map(const char *path)
{
    static unsigned addr[MAX_LABELS + 1];
    static bool rt[MAX_LABELS];
    char name[128];
    int n = 0;
    FILE *f = fopen(path, "r");
    if (!f) fail("cannot open", path);
    while (n < MAX_LABELS && fscanf(f, "%x %127s", &addr[n], name) == 2) {
        if (addr[n] > 0xFFFF || (n && addr[n] < addr[n - 1])) fail("bad map", path);
        rt[n] = strncmp(name, "rt_", 3) == 0 && strncmp(name, "rt_test", 7) != 0;
        if (strcmp(name, "rt_idle_step") == 0) g_ev_step = (long)addr[n];
        if (strcmp(name, "rt_ev_step") == 0) g_ev_step_at = (long)addr[n];
        n++;
    }
    if (!feof(f) || !n) fail("bad map", path);
    fclose(f);
    addr[n] = 0x10000;
    for (int i = 0; i < n; i++)
        for (unsigned a = addr[i]; a < addr[i + 1]; a++) g_rt_code[a] = rt[i];
}

/*----------------------
 | rtv32
 | Description: A 32-bit little-endian runtime count from bank 1.
 | Author: suinevere
 | Dependencies: cpu_ram
 | Globals: N/A
 | Params: addr -- its first byte
 | Returns: the count
 ----------------------*/
static uint32_t rtv32(uint8_t addr)
{
    uint32_t v = 0;
    for (int i = 3; i >= 0; i--) v = (v << 8) | cpu_ram(1, (uint16_t)(addr + i));
    return v;
}

/*----------------------
 | tick_period
 | Description: A tick mode's period in CPU cycles: reload + 1 counts times the cycles a count lasts at the state's clock; 0 with the tick off.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_cpu_period
 | Params: st -- RTV_STATE
 | Returns: cycles, or 0
 ----------------------*/
static uint32_t tick_period(uint8_t st)
{
    uint8_t mode = st & 3;
    uint64_t counts = mode == 3 ? STANDBY_COUNTS : TICK_COUNTS, hz = CLOCK_HZ[(st >> 2) & 3];
    if (!mode) return 0;
    return (uint32_t)((counts * 2u * PS_PER_SECOND + hz * g_cpu_period / 2) / (hz * g_cpu_period));
}

/*----------------------
 | track_tick
 | Description: Follows the tick's due time in cycles after each instruction: a mode starting from off or off forgets it, so check_tick_step takes the next tick as on time; a change between running modes keeps the time left scaled to the new period; and time asleep, with the schedule stopped, moves it on.
 | Author: suinevere
 | Dependencies: tick_period, cpu_ram
 | Globals: g_tick_st, g_due, g_cycles
 | Params: cycles -- the instruction's cycles
 | Returns: N/A
 ----------------------*/
static void track_tick(int cycles)
{
    uint8_t st = cpu_ram(1, RTV_STATE) & 0x0F;
    if (g_due && (cpu_ram(1, RTI_FLAGS) & RTI_SLEPT)) g_due += (unsigned)cycles;
    if (st == g_tick_st) return;
    if (!(st & 3) || !(g_tick_st & 3)) g_due = 0;
    else if (g_due) g_due = g_cycles + (g_due > g_cycles ? (g_due - g_cycles) * tick_period(st) / tick_period(g_tick_st) : 0);
    g_tick_st = st;
}

/*----------------------
 | T0PRR_SFR
 | Description: Timer 0's prescaler reload SFR offset: rt_ev_start sets it to 256 - its period in cycles.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu_rt/sfr.inc
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define T0PRR_SFR 0x11

/*----------------------
 | track_counts
 | Description: Adds an instruction's cycles to the timer counts that have really run, at Timer 0's prescale rate (32 counts a period, 16 for clock 2's 252-cycle period), except while Sleep has stopped the schedule.
 | Author: suinevere
 | Dependencies: cpu_sfr, cpu_ram
 | Globals: g_true_counts
 | Params: cycles -- the instruction's cycles
 | Returns: N/A
 ----------------------*/
static void track_counts(int cycles)
{
    unsigned period = 256u - cpu_sfr(T0PRR_SFR);
    if ((cpu_ram(1, RTI_FLAGS) & RTI_SLEPT) || !period) return;
    g_true_counts += (double)cycles * (period == 252u ? 16.0 : 32.0) / period;
}

/*----------------------
 | check_counts
 | Description: At rt_ev_step's entry, with RTV_BASE just synced and k in TRH:TRL:B:C: how far the counts the runtime has stepped and holds in RTV_BASE stand from the counts Timer 0 has really run, relative to the first step, kept as its minimum and maximum; the steps then add k.
 | Author: suinevere
 | Dependencies: rtv32, cpu_sfr
 | Globals: g_true_counts, g_stepped, g_drift0, g_drift_min, g_drift_max, g_drift_set
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void check_counts(void)
{
    uint32_t k = ((uint32_t)cpu_sfr(SFR_TRH) << 24) | ((uint32_t)cpu_sfr(SFR_TRL) << 16) | ((uint32_t)cpu_sfr(SFR_B) << 8) | cpu_sfr(SFR_C);
    double drift = g_stepped + rtv32(RTV_BASE) - g_true_counts;
    if (!g_drift_set) {
        g_drift_set = true;
        g_drift0 = drift;
    }
    drift -= g_drift0;
    if (drift < g_drift_min) g_drift_min = drift;
    if (drift > g_drift_max) g_drift_max = drift;
    g_stepped += k;
}

/*----------------------
 | check_tick_step
 | Description: At rt_idle_step, where rt_idle has found the nearest event k (in TRH:TRL:B:C) due and goes to step it: when a tick mode runs and the tick countdown is k, this call delivers a tick, which is late when it comes more than LATE_PERIODS Timer 0 periods after the later of its due time and the start of the rt_idle call delivering it, as waiting for the game's next idle is the host's semantics; the first after a mode starts sets the time the rest are due by, a period apart.
 | Author: suinevere
 | Dependencies: rtv32, tick_period, cpu_ram, cpu_sfr
 | Globals: g_ticks, g_late, g_late_max, g_due, g_idle_start, g_cycles
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void check_tick_step(void)
{
    uint8_t st = cpu_ram(1, RTV_STATE);
    uint32_t k = ((uint32_t)cpu_sfr(SFR_TRH) << 24) | ((uint32_t)cpu_sfr(SFR_TRL) << 16) | ((uint32_t)cpu_sfr(SFR_B) << 8) | cpu_sfr(SFR_C);
    uint64_t over, from;
    if (!(st & 3) || rtv32(RTV_TICK) != k) return;
    if (!g_due) g_due = g_cycles;
    from = g_due > g_idle_start ? g_due : g_idle_start;
    over = g_cycles > from ? g_cycles - from : 0;
    g_ticks++;
    if (over > g_late_max) g_late_max = over;
    if (over > LATE_PERIODS * T0_CYCLES[(st >> 2) & 3]) g_late++;
    g_due += tick_period(st);
}

/*----------------------
 | apply_event
 | Description: Applies the next input event to port 3 (active-low buttons) or to port 7 bit 0 with the INT0 line (dock), and waits for a wake before the one after.
 | Author: suinevere
 | Dependencies: execute_set_input
 | Globals: g_events, g_event_next, g_p3, g_p7, g_woke
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void apply_event(void)
{
    const InputEvent *e = &g_events[g_event_next++];
    if (e->kind == EV_PRESS) g_p3 &= (uint8_t)~e->value;
    else if (e->kind == EV_RELEASE) g_p3 |= (uint8_t)e->value;
    else {
        g_p7 = e->value ? 1 : 0;
        execute_set_input(LC8670_EXT_INT0, g_p7);
    }
    g_woke = false;
}

/*----------------------
 | event_due
 | Description: Whether the next input event's idle index has been reached.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_events, g_event_next, g_event_count, g_idle_index
 | Params: N/A
 | Returns: true when one is due
 ----------------------*/
static bool event_due(void)
{
    return g_event_next < g_event_count && g_events[g_event_next].idle <= g_idle_index;
}

/*----------------------
 | begin_idle
 | Description: Starts the next idle index: an event left from an earlier one stops the run, and the new index's first event applies now.
 | Author: suinevere
 | Dependencies: apply_event, event_due, fail
 | Globals: g_events, g_event_next, g_idle_index
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void begin_idle(void)
{
    char at[32];
    g_idle_index++;
    if (event_due() && g_events[g_event_next].idle < g_idle_index) {
        snprintf(at, sizeof at, "%ld", g_events[g_event_next].idle);
        fail("the program left an idle before all its inputs applied, idle", at);
    }
    if (event_due()) apply_event();
}

/*----------------------
 | port3_for
 | Description: Port 3 for a script sample's held buttons: Fire on A, Right, Left, Down, Up on their lines, Mode (0x20) and Sleep (0x40) on theirs, active-low.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: buttons -- VMU_BTN_* bits plus 0x20 Mode and 0x40 Sleep
 | Returns: the port 3 input byte
 ----------------------*/
static uint8_t port3_for(uint8_t buttons)
{
    static const uint8_t line[7] = {0x10, 0x08, 0x04, 0x02, 0x01, 0x40, 0x80};
    uint8_t held = 0;
    for (int i = 0; i < 7; i++)
        if (buttons & (1u << i)) held |= line[i];
    return (uint8_t)~held;
}

/*----------------------
 | advance
 | Description: Moves the script to its next step: a sample sets port 3, a reboot or the end is marked for the main loop.
 | Author: suinevere
 | Dependencies: port3_for
 | Globals: g_script, g_script_next, g_script_count, g_p3, g_reboot_pending, g_end_pending
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void advance(void)
{
    if (g_script_next >= g_script_count) {
        g_end_pending = true;
    } else if (g_script[g_script_next].kind == STEP_REBOOT) {
        g_script_next++;
        g_reboot_pending = true;
    } else {
        g_p3 = port3_for(g_script[g_script_next].buttons);
    }
}

/*----------------------
 | write_frame
 | Description: Appends one frame to the dump: the vmu_ram image gathered from bank 1 and WRAM in packed-offset order, then the 192-byte XRAM frame buffer.
 | Author: suinevere
 | Dependencies: cpu_ram, cpu_wram, cpu_xram
 | Globals: g_dump, g_fields, g_field_count
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void write_frame(void)
{
    uint8_t frame[RAM_IMAGE + FB_BYTES] = {0};
    for (int i = 0; i < g_field_count; i++) {
        const HwField *h = &g_fields[i];
        for (int k = 0; k < h->size; k++)
            frame[h->offset + k] = h->wram ? cpu_wram((uint16_t)(h->addr + k)) : cpu_ram(1, (uint16_t)(h->addr + k));
    }
    for (int k = 0; k < FB_BYTES; k++) frame[RAM_IMAGE + k] = cpu_xram((uint16_t)k);
    fwrite(frame, 1, sizeof frame, g_dump);
}

/*----------------------
 | present
 | Description: The frame point, as vmu_runner's frame callback: closes the --report frame the last one opened, dumps the frame, counts the sample and moves the script on.
 | Author: suinevere
 | Dependencies: write_frame, advance, fail, tick_period
 | Globals: g_dump, g_samples, g_scripted, g_script_next, g_report, g_frame_open, g_frame_busy, g_frame_rt, g_busy, g_rt, g_period, g_frames
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void present(void)
{
    if (g_report && g_frame_open) {
        if (g_frames == MAX_FRAMES) fail("too many frames for", "--report");
        g_busy[g_frames] = (uint32_t)g_frame_busy;
        g_period[g_frames] = tick_period(cpu_ram(1, RTV_STATE));
        g_rt[g_frames++] = (uint32_t)g_frame_rt;
    }
    g_frame_open = true;
    g_frame_busy = g_frame_rt = 0;
    if (g_dump) write_frame();
    g_samples++;
    if (g_scripted && !g_reboot_pending && !g_end_pending) {
        g_script_next++;
        advance();
    }
}

/*----------------------
 | by_value
 | Description: qsort order for frame cycle counts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: a, b -- two uint32_t counts
 | Returns: negative, zero or positive
 ----------------------*/
static int by_value(const void *a, const void *b)
{
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}

/*----------------------
 | print_report
 | Description: Prints --report: the frames from one present to the next with the tick period at the last (tick_period), each frame's executed cycles (halted ones left out) as mean, p90 and max with the max's frame, the runtime code's share as mean and max, the ticks stepped with the latest, the drift of the runtime's counts from Timer 0's (check_counts), and missed ticks: frames that executed for longer than their tick period (overrun) plus ticks stepped late (check_tick_step), the latest's lateness counted from the later of its due time and its idle's start.
 | Author: suinevere
 | Dependencies: by_value
 | Globals: g_busy, g_rt, g_period, g_frames, g_ticks, g_late, g_late_max, g_drift_min, g_drift_max
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void print_report(void)
{
    static uint32_t sorted[MAX_FRAMES];
    uint64_t busy = 0, rt = 0;
    uint32_t rt_max = 0, period = g_frames ? g_period[g_frames - 1] : 0, p90 = 0;
    int at = 0, overrun = 0;
    for (int i = 0; i < g_frames; i++) {
        busy += g_busy[i];
        rt += g_rt[i];
        if (g_rt[i] > rt_max) rt_max = g_rt[i];
        if (g_busy[i] > g_busy[at]) at = i;
        if (g_period[i] && g_busy[i] > g_period[i]) overrun++;
        sorted[i] = g_busy[i];
    }
    qsort(sorted, (size_t)g_frames, sizeof sorted[0], by_value);
    if (g_frames) p90 = sorted[(g_frames * 9 + 9) / 10 - 1];
    printf("frames %d period %u\n", g_frames, period);
    if (g_frames) {
        printf("cycles/frame mean %llu p90 %u max %u frame %d\n", (unsigned long long)((busy + g_frames / 2) / g_frames), p90,
               g_busy[at], at + 1);
        printf("runtime cycles/frame mean %llu max %u\n", (unsigned long long)((rt + g_frames / 2) / g_frames), rt_max);
    }
    printf("ticks %llu late max %llu cycles\n", (unsigned long long)g_ticks, (unsigned long long)g_late_max);
    printf("counts drift min %.0f max %.0f\n", g_drift_min, g_drift_max);
    printf("missed ticks %llu (overrun %d, late %llu)\n", (unsigned long long)(overrun + g_late), overrun, (unsigned long long)g_late);
}

/*----------------------
 | finish
 | Description: Ends the run: writes --out-flash, closes the log, dump, marks and buzzer log, prints --report, then the TIMER_EX calls against the base timer 0 overflows since boot with the BIOS clock's seconds, and exits.
 | Author: suinevere
 | Dependencies: fail, print_report
 | Globals: g_out_flash, g_flash, g_log, g_dump, g_marks, g_buzz, g_timer_calls, g_base_ticks, g_report
 | Params: code -- exit status
 | Returns: does not return
 ----------------------*/
static _Noreturn void finish(int code)
{
    if (g_out_flash) {
        FILE *f = fopen(g_out_flash, "wb");
        if (!f || fwrite(g_flash, 1, sizeof g_flash, f) != sizeof g_flash) fail("cannot write", g_out_flash);
        fclose(f);
    }
    if (g_log) fclose(g_log);
    if (g_dump) fclose(g_dump);
    if (g_marks) fclose(g_marks);
    if (g_buzz) fclose(g_buzz);
    if (g_report) print_report();
    printf("timer_ex %llu overflows %llu seconds %u\n", (unsigned long long)g_timer_calls, (unsigned long long)(g_base_ticks / 16384u),
           cpu_ram(0, 0x1C) * 60u + cpu_ram(0, 0x1D));
    fflush(stdout);
    exit(code);
}

/*----------------------
 | bank_for_ext
 | Description: The code bank the core selects for an EXT value: bit 0 set is flash bank 0, else bit 3 set is ROM, else flash bank 1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ext -- EXT register value
 | Returns: LC8670_BANK_*
 ----------------------*/
static uint8_t bank_for_ext(uint8_t ext)
{
    return (ext & 0x01) ? LC8670_BANK_FLASH_0 : (ext & 0x08) ? LC8670_BANK_ROM : LC8670_BANK_FLASH_1;
}

/*----------------------
 | ram_bank
 | Description: The RAM bank PSW selects, which the BIOS flash routines take their arguments and page from.
 | Author: suinevere
 | Dependencies: cpu_sfr
 | Globals: N/A
 | Params: N/A
 | Returns: 0 or 1
 ----------------------*/
static int ram_bank(void)
{
    return (cpu_sfr(SFR_PSW) >> 1) & 1;
}

/*----------------------
 | poison
 | Description: Leaves POISON in the registers a BIOS flash routine destroys: R0 in the current bank, TRL, TRH and FPR always, ACC unless it carries the routine's status, and B and C after the page write.
 | Author: suinevere
 | Dependencies: ram_bank, cpu_ram_write, cpu_sfr_write
 | Globals: N/A
 | Params: acc -- poison ACC too; bc -- poison B and C too
 | Returns: N/A
 ----------------------*/
static void poison(bool acc, bool bc)
{
    cpu_ram_write(ram_bank(), 0x00, POISON);
    cpu_sfr_write(SFR_TRL, POISON);
    cpu_sfr_write(SFR_TRH, POISON);
    cpu_sfr_write(SFR_FPR, POISON);
    if (acc) cpu_sfr_write(SFR_ACC, POISON);
    if (bc) {
        cpu_sfr_write(SFR_B, POISON);
        cpu_sfr_write(SFR_C, POISON);
    }
}

/*----------------------
 | fm_write
 | Description: FM_WRT_EX as the US BIOS runs it (american_v1.05.s label_3CB7, label_E3CE): refuses a nonzero bank at 0x7D or a page past the GAME's blocks, else writes bank 1 0x80-0xFF to flash bank 0 through the page write, whose registers it poisons; its flash icon leaves XBNK 2 either way.
 | Author: suinevere
 | Dependencies: cpu_ram, cpu_ram_write, cpu_sfr_write, poison
 | Globals: g_flash
 | Params: N/A
 | Returns: ACC: 0x00, or 0xFF on a refusal
 ----------------------*/
static uint8_t fm_write(void)
{
    uint8_t last = cpu_ram(0, GAME_LAST);
    cpu_sfr_write(SFR_XBNK, 2);
    if (cpu_ram(1, 0x7D) != 0 || last == 0 || (last & 0x80) || cpu_ram(1, 0x7E) > (uint8_t)((last << 1) | 1)) return 0xFF;
    cpu_ram_write(1, 0x7F, cpu_ram(1, 0x7F) & 0x80);
    uint32_t base = ((uint32_t)cpu_ram(1, 0x7E) << 8) | cpu_ram(1, 0x7F);
    for (int i = 0; i < PAGE; i++) g_flash[base + i] = cpu_ram(1, (uint16_t)(0x80 + i));
    poison(false, true);
    return 0x00;
}

/*----------------------
 | flash_at
 | Description: The flash image byte the BIOS reaches with FPR's bank bit and TRH:TRL.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_flash
 | Params: bank -- FPR (bit 0 used); trh, trl -- the address in the bank
 | Returns: pointer into g_flash
 ----------------------*/
static uint8_t *flash_at(uint8_t bank, uint8_t trh, uint8_t trl)
{
    return &g_flash[((uint32_t)(bank & 1) << 16) | ((uint32_t)trh << 8) | trl];
}

/*----------------------
 | fm_read
 | Description: FM_PRD_EX as the US BIOS runs it (label_E3F6): FPR from 0x7D, then bytes from 0x7E:0x7F stored through R0 from 0x80 until R0 wraps, so 128 bytes on any address, TRL counting on without carry; poisons ACC, R0, TRL, TRH and FPR.
 | Author: suinevere
 | Dependencies: ram_bank, flash_at, cpu_ram, cpu_ram_write, poison
 | Globals: g_flash
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void fm_read(void)
{
    int rb = ram_bank();
    uint8_t bank = cpu_ram(rb, 0x7D), trh = cpu_ram(rb, 0x7E), trl = cpu_ram(rb, 0x7F), r0 = 0x80;
    do {
        cpu_ram_write(rb, r0, *flash_at(bank, trh, trl));
        r0++;
        trl++;
    } while (r0 != 0);
    poison(true, false);
}

/*----------------------
 | fm_verify
 | Description: FM_VRF_EX as the US BIOS runs it (label_E388): FPR from 0x7D, then compares the page at 0x7E:0x7F, TRL wrapping inside its 128-byte half, with 0x80-0xFF through R0, stopping at the first difference; poisons R0, TRL, TRH and FPR.
 | Author: suinevere
 | Dependencies: ram_bank, flash_at, cpu_ram, poison
 | Globals: g_flash
 | Params: N/A
 | Returns: ACC: 0x00 when every byte matches, else the first difference's XOR
 ----------------------*/
static uint8_t fm_verify(void)
{
    int rb = ram_bank();
    uint8_t bank = cpu_ram(rb, 0x7D), trh = cpu_ram(rb, 0x7E), trl = cpu_ram(rb, 0x7F), r0 = 0x80, diff = 0;
    do {
        diff = *flash_at(bank, trh, trl) ^ cpu_ram(rb, r0);
        if (diff) break;
        trl = (uint8_t)((trl & 0x80) | ((trl + 1) & 0x7F));
        r0++;
    } while (r0 != 0);
    poison(false, false);
    return diff;
}

/*----------------------
 | fm_page
 | Description: The page write at ROM 0xE024 (label_E3CE, label_E348), which the Maple block write calls with interrupts masked and which checks nothing; entered with IE bit 7 set it stops the run. FPR from 0x7D's bit 0, then 0x80-0xFF through R0 into the 128-byte half at 0x7E:0x7F, TRL wrapping inside it; the flash takes the page whole, so data polling ends at once. Poisons ACC, B, C, R0, TRL, TRH and FPR.
 | Author: suinevere
 | Dependencies: ram_bank, flash_at, cpu_ram, cpu_sfr, poison, finish
 | Globals: g_flash
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void fm_page(void)
{
    if (cpu_sfr(SFR_IE) & 0x80) {
        printf("rom entry %04X entered with interrupts enabled\n", ROM_PAGE);
        finish(3);
    }
    int rb = ram_bank();
    uint8_t bank = cpu_ram(rb, 0x7D), trh = cpu_ram(rb, 0x7E), trl = cpu_ram(rb, 0x7F), r0 = 0x80;
    for (;;) {
        *flash_at(bank, trh, trl) = cpu_ram(rb, r0);
        r0++;
        if (r0 == 0) break;
        trl = (uint8_t)((trl & 0x80) | ((trl + 1) & 0x7F));
    }
    poison(true, true);
}

/*----------------------
 | clock_tick
 | Description: The BIOS's half-second clock step (label_E412): every second call advances the seconds in bank 0 and carries into minutes, hours, days, months and years, the leap flag at 0x1F choosing February's length.
 | Author: suinevere
 | Dependencies: cpu_ram, cpu_ram_write
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void clock_tick(void)
{
    static const uint8_t last_day[13] = {0, 0x20, 0x1D, 0x20, 0x1F, 0x20, 0x1F, 0x20, 0x20, 0x1F, 0x20, 0x1F, 0x20};
    uint8_t month, end;
    if (!cpu_ram(0, 0x1E)) {
        cpu_ram_write(0, 0x1E, cpu_ram(0, 0x1E) | 1);
        return;
    }
    cpu_ram_write(0, 0x1E, 0);
    cpu_ram_write(0, 0x1D, cpu_ram(0, 0x1D) + 1);
    if (cpu_ram(0, 0x1D) != 60) return;
    cpu_ram_write(0, 0x1D, 0);
    cpu_ram_write(0, 0x1C, cpu_ram(0, 0x1C) + 1);
    if (cpu_ram(0, 0x1C) != 60) return;
    cpu_ram_write(0, 0x1C, 0);
    cpu_ram_write(0, 0x1B, cpu_ram(0, 0x1B) + 1);
    if (cpu_ram(0, 0x1B) != 24) return;
    cpu_ram_write(0, 0x1B, 0);
    cpu_ram_write(0, 0x1A, cpu_ram(0, 0x1A) + 1);
    month = cpu_ram(0, 0x19);
    end = month < 13 ? last_day[month] : 0x20;
    if (month == 2 && cpu_ram(0, 0x1F)) end = 0x1E;
    if (month == 0) end = 0x20;
    if (cpu_ram(0, 0x1A) != end) return;
    cpu_ram_write(0, 0x1A, 1);
    cpu_ram_write(0, 0x19, month + 1);
    if (cpu_ram(0, 0x19) != 13) return;
    cpu_ram_write(0, 0x19, 1);
    cpu_ram_write(0, 0x18, cpu_ram(0, 0x18) + 1);
    if (!cpu_ram(0, 0x18)) cpu_ram_write(0, 0x17, cpu_ram(0, 0x17) + 1);
    {
        unsigned year = (cpu_ram(0, 0x17) << 8) | cpu_ram(0, 0x18);
        cpu_ram_write(0, 0x1F, (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 1 : 0);
    }
}

/*----------------------
 | timer_ex
 | Description: TIMER_EX (label_3DC9) on RAM bank 0: base timer interrupt 1's flag counts at 0x74 unless 0x73 is set; interrupt 0's flag runs clock_tick, the idle counters at 0x70/0x72 and the 0x33/0x34 counters, stopping on the low-battery path it does not model. It poisons no register: the BIOS keeps ACC, B, C, PSW and R0 and touches no other.
 | Author: suinevere
 | Dependencies: clock_tick, cpu_sfr, cpu_sfr_write, cpu_ram, cpu_ram_write, ext_io_r, finish
 | Globals: g_timer_calls
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void timer_ex(void)
{
    uint8_t btcr = cpu_sfr(SFR_BTCR);
    g_timer_calls++;
    if (!cpu_ram(0, 0x73) && (btcr & 0x08)) {
        btcr &= (uint8_t)~0x08;
        cpu_sfr_write(SFR_BTCR, btcr);
        cpu_ram_write(0, 0x74, cpu_ram(0, 0x74) + 1);
    }
    if (!(btcr & 0x02)) return;
    cpu_sfr_write(SFR_BTCR, btcr & (uint8_t)~0x02);
    clock_tick();
    if (ext_io_r(LC8670_PORT3) == 0xFF) {
        cpu_ram_write(0, 0x72, cpu_ram(0, 0x70));
        cpu_ram_write(0, 0x70, 0);
    }
    if (cpu_ram(0, 0x6E) != 0xFF && !(ext_io_r(LC8670_PORT7) & 0x02)) {
        printf("rom entry %04X low-battery path not modelled\n", ROM_TIMER);
        finish(3);
    }
    cpu_ram_write(0, 0x33, cpu_ram(0, 0x33) + 1);
    cpu_ram_write(0, 0x34, cpu_ram(0, 0x34) + 1);
}

/*----------------------
 | rom_return
 | Description: The page write's RET: pops the return address its caller pushed and follows it if it is one of the BIOS entries' tails (not1 ext,0 then jmpf into flash), 0x0103, 0x0113 and 0x0123 to entry + 5 and 0x0133 to 0x139; any other stops the run.
 | Author: suinevere
 | Dependencies: finish, bank_for_ext, cpu_ram, cpu_sfr, cpu_sfr_write, cpu_set_pc
 | Globals: g_bank
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void rom_return(void)
{
    uint8_t sp = cpu_sfr(SFR_SP), ext = cpu_sfr(SFR_EXT) ^ 0x01;
    uint16_t ret = (uint16_t)((cpu_ram(0, sp) << 8) | cpu_ram(0, (uint8_t)(sp - 1)));
    uint16_t back = ret == 0x0103 ? 0x0105 : ret == 0x0113 ? 0x0115 : ret == 0x0123 ? 0x0125 : ret == 0x0133 ? 0x0139 : 0;
    if (!back) {
        printf("rom return %04X not modelled\n", ret);
        finish(3);
    }
    cpu_sfr_write(SFR_SP, (uint8_t)(sp - 2));
    cpu_sfr_write(SFR_EXT, ext);
    cpu_set_pc(back);
    g_bank = bank_for_ext(ext);
}

/*----------------------
 | rom_call
 | Description: Runs the BIOS entry execution just jumped to: a flash write, verify or read, then the BIOS's return to entry + 5 in flash, TIMER_EX returning to 0x139, the page write returning through rom_return, or the exit; any other entry stops the run.
 | Author: suinevere
 | Dependencies: fm_write, fm_verify, fm_read, fm_page, rom_return, timer_ex, finish, bank_for_ext, cpu_sfr, cpu_sfr_write, cpu_set_pc, cpu_acc
 | Globals: g_bank, g_cycles, g_steps
 | Params: pc -- the ROM address entered
 | Returns: N/A
 ----------------------*/
static void rom_call(uint16_t pc)
{
    uint8_t ext = cpu_sfr(SFR_EXT) ^ 0x01;
    if (pc == ROM_WRITE) {
        cpu_sfr_write(SFR_ACC, fm_write());
    } else if (pc == ROM_VERIFY) {
        cpu_sfr_write(SFR_ACC, fm_verify());
    } else if (pc == ROM_READ) {
        fm_read();
    } else if (pc == ROM_PAGE) {
        fm_page();
        rom_return();
        return;
    } else if (pc == ROM_TIMER) {
        timer_ex();
        cpu_sfr_write(SFR_EXT, ext);
        cpu_set_pc(0x0139);
        g_bank = bank_for_ext(ext);
        return;
    } else if (pc == ROM_EXIT) {
        printf("exit %02X\ncycles %llu steps %llu\n", cpu_acc(), (unsigned long long)g_cycles, (unsigned long long)g_steps);
        finish(0);
    } else {
        printf("rom entry %04X not modelled\n", pc);
        finish(3);
    }
    cpu_sfr_write(SFR_EXT, ext);
    cpu_set_pc((uint16_t)(pc + 5));
    g_bank = bank_for_ext(ext);
}

/*----------------------
 | cycle_end
 | Description: Ends one CPU cycle: one Timer 0/1 clock tick, run by the core or skipped over when it cannot overflow, then every base timer tick now due.
 | Author: suinevere
 | Dependencies: device_timer, cpu_clock_advance
 | Globals: g_time, g_next_base, g_cpu_period, g_base_period, g_cycles, g_base_ticks
 | Params: real -- nonzero to run the clock tick through the core
 | Returns: N/A
 ----------------------*/
static void cycle_end(int real)
{
    if (real) device_timer(CLOCK_TIMER);
    else cpu_clock_advance(1);
    g_time += g_cpu_period;
    g_cycles++;
    while (g_next_base <= g_time) {
        device_timer(BASE_TIMER);
        g_next_base += g_base_period;
        g_base_ticks++;
    }
}

/*----------------------
 | quiet_cycles
 | Description: Skips n cycles in which no timer overflows or raises a flag, advancing the counters arithmetically.
 | Author: suinevere
 | Dependencies: cpu_clock_advance, cpu_base_advance
 | Globals: g_time, g_next_base, g_cpu_period, g_base_period, g_cycles, g_base_ticks
 | Params: n -- cycles
 | Returns: N/A
 ----------------------*/
static void quiet_cycles(uint64_t n)
{
    cpu_clock_advance((uint32_t)n);
    g_time += n * g_cpu_period;
    g_cycles += n;
    if (g_next_base <= g_time) {
        uint64_t due = (g_time - g_next_base) / g_base_period + 1;
        cpu_base_advance((uint32_t)due);
        g_next_base += due * g_base_period;
        g_base_ticks += due;
    }
}

/*----------------------
 | idle_skip
 | Description: The idle schedule for a halted CPU: jumps to the cycle of the next timer or base timer event and delivers it through the core.
 | Author: suinevere
 | Dependencies: cpu_clock_ticks_to_event, cpu_base_ticks_to_event, quiet_cycles, cycle_end
 | Globals: g_time, g_next_base, g_cpu_period, g_base_period
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void idle_skip(void)
{
    uint32_t kc = cpu_clock_ticks_to_event(0), kb = cpu_base_ticks_to_event(0);
    uint64_t j = kc == CPU_NO_EVENT ? UINT64_MAX : kc;
    if (kb != CPU_NO_EVENT) {
        uint64_t tb = g_next_base + (uint64_t)(kb - 1) * g_base_period;
        uint64_t jb = (tb - g_time + g_cpu_period - 1) / g_cpu_period;
        if (jb < j) j = jb;
    }
    if (j == UINT64_MAX) return;
    if (j > 1) quiet_cycles(j - 1);
    cycle_end(j == kc);
}

/*----------------------
 | game_blocks_less_one
 | Description: The GAME's block count less one from the flash directory, as the BIOS leaves it at bank 0 0x6D; 0 when the image has no GAME entry.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_flash
 | Params: N/A
 | Returns: block count less one
 ----------------------*/
static uint8_t game_blocks_less_one(void)
{
    for (int b = 0; b < DIR_BLOCKS; b++) {
        for (int e = 0; e < BLOCK; e += 32) {
            const uint8_t *entry = g_flash + (DIR_TOP - b) * BLOCK + e;
            unsigned size = entry[0x18] | (entry[0x19] << 8);
            if (entry[0] == GAME_TYPE && size) return (uint8_t)(size - 1);
        }
    }
    return 0;
}

/*----------------------
 | boot
 | Description: Resets the core and starts the GAME in flash bank 0 the way the BIOS does: its block count at bank 0 0x6D, OCR 0xA3, SP 0x7F, RAM bank 1, EXT on flash.
 | Author: suinevere
 | Dependencies: device_reset, device_start, cpu_sfr_write, cpu_ram_write, game_blocks_less_one
 | Globals: g_bank, g_booted, g_next_base, g_time, g_base_period, g_idle_index, g_idle_marks, g_in_halt, g_base_ticks
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void boot(void)
{
    g_booted = false;
    device_reset();
    device_start();
    cpu_ram_write(0, GAME_LAST, game_blocks_less_one());
    cpu_sfr_write(SFR_OCR, 0xA3);
    cpu_sfr_write(SFR_SP, 0x7F);
    cpu_sfr_write(SFR_PSW, 0x02);
    cpu_sfr_write(SFR_EXT, 0x09);
    g_bank = LC8670_BANK_FLASH_0;
    g_booted = true;
    g_next_base = g_time + g_base_period;
    g_idle_index = 0;
    g_idle_marks = 0;
    g_in_halt = false;
    g_base_ticks = 0;
}

/*----------------------
 | ext_io_r
 | Description: Port reads: port 3 is the script's or the inputs' buttons (active-low), port 7 bit 0 the dock line with bit 1 high (battery good), port 1 idle-high.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_p3, g_p7
 | Params: iop -- the port being read
 | Returns: the port's input bits
 ----------------------*/
uint8_t ext_io_r(enum IoPortNum iop)
{
    if (iop == LC8670_PORT3) return g_p3;
    if (iop == LC8670_PORT7) return (uint8_t)(g_p7 | 0x02);
    return 0xFF;
}

/*----------------------
 | ext_io_w
 | Description: Port writes, discarded; the buzzer and other outputs are not modelled.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: iop -- the port being written; val -- the value written
 | Returns: N/A
 ----------------------*/
void ext_io_w(enum IoPortNum iop, uint8_t val)
{
    (void)iop;
    (void)val;
}

/*----------------------
 | ext_read_code
 | Description: A code byte from the current bank: flash bank 0 or 1 of the image; ROM reads 0xFF, as execution never stays there.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_flash, g_bank
 | Params: addr -- code address, 0x0000-0xFFFF
 | Returns: the byte at that address
 ----------------------*/
uint8_t ext_read_code(uint16_t addr)
{
    if (g_bank == LC8670_BANK_FLASH_0) return g_flash[addr];
    if (g_bank == LC8670_BANK_FLASH_1) return g_flash[0x10000 | addr];
    return 0xFF;
}

/*----------------------
 | ext_code_bankswitch
 | Description: Records the code bank; after boot, a switch to ROM marks a BIOS entry for rom_call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_bank, g_booted, g_rom_pending
 | Params: bank -- LC8670_BANK_ROM, LC8670_BANK_FLASH_0 or LC8670_BANK_FLASH_1
 | Returns: N/A
 ----------------------*/
void ext_code_bankswitch(uint8_t bank)
{
    g_bank = bank;
    if (g_booted && bank == LC8670_BANK_ROM) g_rom_pending = true;
}

/*----------------------
 | ext_set_cpu_freq
 | Description: Clock-rate notification; timer_adust already sets the cycle period.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: freq -- the new instruction clock in Hz
 | Returns: N/A
 ----------------------*/
void ext_set_cpu_freq(uint32_t freq)
{
    (void)freq;
}

/*----------------------
 | ext_cpu_is_running_in_rom
 | Description: Whether the code bank is ROM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_bank
 | Params: N/A
 | Returns: true in ROM
 ----------------------*/
bool ext_cpu_is_running_in_rom(void)
{
    return g_bank == LC8670_BANK_ROM;
}

/*----------------------
 | ext_write_flash_direct
 | Description: The core's STF path, used only by code running in ROM, which the harness never runs; writes through to the image.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_flash
 | Params: address -- flash address; data -- bytes to write; len -- byte count
 | Returns: N/A
 ----------------------*/
void ext_write_flash_direct(uint32_t address, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) g_flash[(address + i) % FLASH_SIZE] = data[i];
}

/*----------------------
 | buzz_tone
 | Description: The rt_buzz tone a T1CNT write sounds: 0 unless it runs T1L in 8-bit mode with P17 an output carrying the PWM over a low latch; else the t whose T1LR = t >> 1 and T1LC = T1LR + (256 - T1LR) / 2 + (t & 1) match, stopping the run on settings no tone has.
 | Author: suinevere
 | Dependencies: cpu_sfr, finish
 | Globals: N/A
 | Params: t1cnt -- the value written to T1CNT
 | Returns: the tone, 0-255
 ----------------------*/
static uint8_t buzz_tone(uint8_t t1cnt)
{
    uint8_t lr = cpu_sfr(SFR_T1LR), lc = cpu_sfr(SFR_T1LC);
    if (!(t1cnt & 0x40) || (t1cnt & 0x20) || !(cpu_sfr(SFR_P1DDR) & 0x80) || !(cpu_sfr(SFR_P1FCR) & 0x80) || (cpu_sfr(SFR_P1) & 0x80))
        return 0;
    for (unsigned t = 1; t < 256; t++)
        if (lr == (t >> 1) && lc == (uint8_t)((t >> 1) + ((256 - (t >> 1)) >> 1) + (t & 1))) return (uint8_t)t;
    printf("buzzer T1LR %02X T1LC %02X is no tone\n", lr, lc);
    finish(3);
}

/*----------------------
 | ext_sfr_w
 | Description: SFR write callback: an RT_LOG byte goes to --log, any RT_PRESENT write is a frame point, any RT_IDLE_MARK write begins the next --inputs idle index, writes the cycle count to --marks and notes it for check_tick_step, and each T1CNT or VCCR write goes to --buzz as z IDLE 0 TONE or z IDLE 1 ON, the idle being the marks so far, as vmu_host_buzz_log has it.
 | Author: suinevere
 | Dependencies: present, buzz_tone
 | Globals: g_log, g_idle_marks, g_marks, g_cycles, g_buzz, g_idle_index, g_idle_start
 | Params: offset -- SFR offset 0x00-0x7F; data -- the byte written
 | Returns: N/A
 ----------------------*/
void ext_sfr_w(uint16_t offset, uint8_t data)
{
    if (offset == SFR_T1CNT && g_buzz) fprintf(g_buzz, "z %04lX 0 %02X\n", g_idle_index, buzz_tone(data));
    if (offset == SFR_VCCR && g_buzz) fprintf(g_buzz, "z %04lX 1 %02X\n", g_idle_index, (data & 0x80) ? 1 : 0);
    if (offset == SFR_RT_LOG && g_log) fputc(data, g_log);
    if (offset == SFR_RT_PRESENT) present();
    if (offset == SFR_RT_IDLE_MARK) {
        g_idle_start = g_cycles;
        g_idle_marks++;
        if (g_marks) fprintf(g_marks, "%llu\n", (unsigned long long)g_cycles);
    }
}

/*----------------------
 | timer_adust
 | Description: Sets the cycle period or the base timer period from the core's clock rates.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_cpu_period, g_base_period
 | Params: id -- BASE_TIMER or CLOCK_TIMER; freq -- the timer's input frequency in Hz
 | Returns: N/A
 ----------------------*/
void timer_adust(int id, uint32_t freq)
{
    if (!freq) return;
    if (id == CLOCK_TIMER) g_cpu_period = PS_PER_SECOND / freq;
    else g_base_period = PS_PER_SECOND / freq;
}

/*----------------------
 | after_step
 | Description: Handles what an instruction left pending: a BIOS entry, the script's end, or a reboot from flash with the next script step.
 | Author: suinevere
 | Dependencies: rom_call, finish, boot, advance, cpu_pc
 | Globals: g_rom_pending, g_end_pending, g_reboot_pending, g_samples, g_cycles, g_steps
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void after_step(void)
{
    if (g_rom_pending) {
        g_rom_pending = false;
        rom_call(cpu_pc());
    }
    while (g_end_pending || g_reboot_pending) {
        if (g_end_pending) {
            printf("end samples %u cycles %llu steps %llu\n", g_samples, (unsigned long long)g_cycles, (unsigned long long)g_steps);
            finish(0);
        }
        g_reboot_pending = false;
        boot();
        advance();
    }
}

/*----------------------
 | step
 | Description: Runs one instruction (or one halted cycle) through the core, ends its cycles and handles what it left pending; for --report it adds an executed instruction's cycles to the frame, and to the runtime's share when the runtime's code in flash bank 0 holds it, and checks each rt_idle_step entry for a tick and each rt_ev_step entry for the counts' drift.
 | Author: suinevere
 | Dependencies: execute_run, cycle_end, after_step, cpu_halted, cpu_pc, check_tick_step, track_tick, check_counts, track_counts
 | Globals: g_steps, g_frame_busy, g_frame_rt, g_rt_code, g_bank, g_report, g_ev_step, g_ev_step_at
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void step(void)
{
    bool halted = cpu_halted(), rt = g_bank == LC8670_BANK_FLASH_0 && g_rt_code[cpu_pc()];
    if (g_report && !halted && g_bank == LC8670_BANK_FLASH_0 && (long)cpu_pc() == g_ev_step) check_tick_step();
    if (g_report && !halted && g_bank == LC8670_BANK_FLASH_0 && (long)cpu_pc() == g_ev_step_at) check_counts();
    int cycles = execute_run(1);
    if (g_report) {
        track_tick(cycles);
        track_counts(cycles);
    }
    if (!halted) {
        g_frame_busy += (unsigned)cycles;
        if (rt) g_frame_rt += (unsigned)cycles;
    }
    g_steps++;
    for (int i = 0; i < cycles; i++) cycle_end(1);
    after_step();
}

/*----------------------
 | main
 | Description: Parses the options, boots the GAME and runs it until it exits, ends its script, stops in ROM, halts with nothing left to wake it, or passes --max-cycles. Each idle index's first input applies at its start (index 0 before the first instruction, later ones at their RT_IDLE_MARK write), each next at a quiet halt after a wake, or at once if nothing could wake the CPU.
 | Author: suinevere
 | Dependencies: load_flash, load_hw, load_map, load_script, load_inputs, boot, advance, after_step, step, idle_skip, apply_event, event_due, begin_idle, finish
 | Globals: g_log, g_dump, g_marks, g_buzz, g_out_flash, g_cycles, g_steps, g_idle_index, g_idle_marks, g_in_halt, g_woke, g_scripted, g_report
 | Params: argc, argv -- --flash IMG or --game FILE, [--script S.pks] [--inputs EVENTS] [--scenario S.bin] [--schedule idle|real] [--dump OUT --hw HW.json] [--log OUT] [--marks OUT] [--buzz OUT] [--out-flash OUT] [--report --map GAME.map] [--max-cycles N]
 | Returns: 0 on exit, script end or a final halt, 1 on the cycle limit, 2 on a usage error, 3 on an unmodelled ROM entry or return or a buzzer setting no tone has
 ----------------------*/
int main(int argc, char **argv)
{
    const char *image = NULL, *script = NULL, *inputs = NULL, *scenario = NULL, *dump = NULL, *log = NULL, *hw = NULL, *marks = NULL, *buzz = NULL,
               *map = NULL;
    bool idle = true;
    long long max_cycles = -1;

    for (int i = 1; i < argc; i += 2) {
        const char *a = argv[i], *v = i + 1 < argc ? argv[i + 1] : NULL;
        if (strcmp(a, "--report") == 0) {
            g_report = true;
            i--;
            continue;
        }
        if (!v) fail("missing value for", a);
        if (strcmp(a, "--flash") == 0 || strcmp(a, "--game") == 0) image = v;
        else if (strcmp(a, "--script") == 0) script = v;
        else if (strcmp(a, "--inputs") == 0) inputs = v;
        else if (strcmp(a, "--scenario") == 0) scenario = v;
        else if (strcmp(a, "--schedule") == 0 && (strcmp(v, "idle") == 0 || strcmp(v, "real") == 0)) idle = v[0] == 'i';
        else if (strcmp(a, "--dump") == 0) dump = v;
        else if (strcmp(a, "--log") == 0) log = v;
        else if (strcmp(a, "--marks") == 0) marks = v;
        else if (strcmp(a, "--buzz") == 0) buzz = v;
        else if (strcmp(a, "--out-flash") == 0) g_out_flash = v;
        else if (strcmp(a, "--hw") == 0) hw = v;
        else if (strcmp(a, "--map") == 0) map = v;
        else if (strcmp(a, "--max-cycles") == 0) max_cycles = strtoll(v, NULL, 10);
        else fail("bad option", a);
    }
    if (!image || (script && scenario) || (dump && !hw) || (g_report && !map)) {
        fprintf(stderr, "usage: vmu_harness --flash IMG [--script S.pks | --scenario S.bin] [--inputs EVENTS] [--schedule idle|real] "
                        "[--dump OUT.state --hw HW.json] [--log OUT.log] [--marks OUT] [--buzz OUT] [--out-flash OUT.bin] "
                        "[--report --map GAME.map] [--max-cycles N]\n");
        return 2;
    }
    (void)scenario;

    load_flash(image);
    if (hw) load_hw(hw);
    if (map) load_map(map);
    if (script) load_script(script);
    if (inputs) load_inputs(inputs);
    if (log && !(g_log = fopen(log, "wb"))) fail("cannot write", log);
    if (dump && !(g_dump = fopen(dump, "wb"))) fail("cannot write", dump);
    if (marks && !(g_marks = fopen(marks, "w"))) fail("cannot write", marks);
    if (buzz && !(g_buzz = fopen(buzz, "w"))) fail("cannot write", buzz);

    boot();
    if (g_scripted) {
        advance();
        after_step();
    }
    if (event_due()) apply_event();
    while (max_cycles < 0 || g_cycles < (unsigned long long)max_cycles) {
        if (!cpu_halted()) {
            if (g_in_halt) g_woke = true;
            g_in_halt = false;
        } else if (!g_in_halt) {
            g_in_halt = true;
        } else if (!cpu_irq_pending()) {
            bool stuck = cpu_clock_ticks_to_event(1) == CPU_NO_EVENT && cpu_base_ticks_to_event(1) == CPU_NO_EVENT;
            if (event_due() && (g_woke || stuck)) {
                apply_event();
            } else if (stuck) {
                printf("halted pc %04X cycles %llu steps %llu\nacc %02X\n", cpu_pc(), (unsigned long long)g_cycles,
                       (unsigned long long)g_steps, cpu_acc());
                finish(0);
            } else if (idle) {
                idle_skip();
            }
        }
        step();
        while (g_idle_marks) {
            g_idle_marks--;
            begin_idle();
        }
    }
    printf("limit cycles %llu steps %llu\n", (unsigned long long)g_cycles, (unsigned long long)g_steps);
    finish(1);
}
