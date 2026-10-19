/*----------------------
 | pkrun.cc
 | Description: Reference runner: boots a card under OpenPSK on the pinned PocketStation core, drives a .pks script at the app's frame points, writes a PKTR1 trace, and can replace ARM functions by C.
 | Author: suinevere
 | Dependencies: pocketstation.h (patched by core.patch), psk_bus_emu.h, generated hooks.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/

#include <chrono>
#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "pocketstation.h"
#include "psk_bus_emu.h"
#include "psk_swi.h"

using PCSX::PocketStation::PocketStation;

/*----------------------
 | EXIT_TIMEOUT
 | Description: Exit code when --max-seconds of host time runs out.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static constexpr int EXIT_TIMEOUT = 3;

/*----------------------
 | EXIT_SWI
 | Description: Exit code when the app calls an SWI neither OpenPSK nor launch.c implements.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static constexpr int EXIT_SWI = 4;

/*----------------------
 | RUNNER_MAGIC
 | Description: Marker word before the launcher's idle loop and SWI stubs in the kernel ROM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static constexpr uint32_t RUNNER_MAGIC = 0x4B53504C;

/*----------------------
 | Command
 | Description: One parsed script line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
struct Command {
    std::string op;
    uint32_t a = 0, b = 0;
};

/*----------------------
 | Step
 | Description: One script step: a sample with a button mask, a clock change, or a reboot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
struct Step {
    enum Kind { Sample, Rtc, Reboot } kind;
    uint32_t a = 0, b = 0;
};

/*----------------------
 | Runner
 | Description: Device, script position, samples, sampling triggers, SWI census and debugging aids.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
struct Runner {
    PocketStation ps;
    std::vector<Step> steps;
    size_t next = 0;
    uint32_t buttons = 0;
    std::vector<uint8_t> trace;
    uint32_t samples = 0;
    uint32_t frameAddr = 0x02003E48;
    bool frameArmed = true;
    double timeoutMs = 1000.0;
    uint64_t activeCycles = 0;
    uint32_t stackLow = 0x700;
    std::vector<std::pair<uint32_t, uint32_t>> ramSkip = {{0x5C4, 0x5D2}};
    uint32_t minSp = 0x800;
    uint32_t byFrame = 0, byHalt = 0, byTimeout = 0;
    bool lastHalted = false;
    double seconds = 0;
    uint64_t steps_run = 0;
    std::set<uint32_t> seenSwis;
    std::set<uint32_t> missingSwis;
    uint32_t swiProbe = 0;
    bool swiProbePending = false;
    bool swiProbeC = false;
    uint32_t pcRing[4096] = {};
    uint32_t pcPos = 0;
    bool pcLog = false;
    std::vector<uint8_t> coverage = std::vector<uint8_t>(0x10000);
    int watch = -1;
    uint8_t watchLast = 0;
    uint32_t watchPrevPc = 0;
    std::chrono::steady_clock::time_point deadline;
    std::jmp_buf top;
    FILE* swiLog = nullptr;
    bool swiPending = false;
    uint32_t swiN = 0, swiIn[3] = {}, swiRet = 0, swiMode = 0;
    bool swiThumb = false;
    std::vector<uint8_t> swiSrc;
    std::string entryRamPath;
    bool entryDumped = false;
    bool idleTime = false;
    bool appEntered = false;
    std::string entryStatePath;
    bool drive = false;
    bool lastApp = false;
    std::set<uint32_t> armEntries;
};

static Runner *g_runner;
/*----------------------
 | crc32
 | Description: Standard CRC-32 (zlib polynomial) of a buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- data; n -- length
 | Returns: the CRC
 ----------------------*/
static uint32_t crc32(const uint8_t* p, size_t n) {
    static uint32_t table[256];
    if (!table[1]) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
    }
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

/*----------------------
 | put32
 | Description: Appends a little-endian word to a byte buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: out -- buffer; v -- value
 | Returns: N/A
 ----------------------*/
static void put32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; i++) out.push_back(uint8_t(v >> (8 * i)));
}

/*----------------------
 | readFile
 | Description: Loads a whole file, exiting with code 2 if it is missing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- file
 | Returns: the bytes
 ----------------------*/
static std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        fprintf(stderr, "pkrun: cannot read %s\n", path.c_str());
        exit(2);
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

/*----------------------
 | bcd
 | Description: Packs 0..99 as BCD.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value
 | Returns: BCD byte
 ----------------------*/
static uint32_t bcd(int v) { return uint32_t(((v / 10) << 4) | (v % 10)); }

/*----------------------
 | dayOfWeek
 | Description: Day of the week of a Gregorian date, 1 = Sunday .. 7 = Saturday as the RTC counts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: y, m, d -- the date
 | Returns: 1..7
 ----------------------*/
static int dayOfWeek(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7 + 1;
}

/*----------------------
 | parseScript
 | Description: Parses a .pks script; rtc is stored as RTC_DATE in a and RTC_TIME in b.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- script file
 | Returns: the commands; exits with code 2 on a bad line
 ----------------------*/
static std::vector<Command> parseScript(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        fprintf(stderr, "pkrun: cannot read %s\n", path.c_str());
        exit(2);
    }
    std::vector<Command> out;
    std::string line;
    int lineNo = 0;
    while (std::getline(f, line)) {
        lineNo++;
        auto hash = line.find('#');
        if (hash != std::string::npos) line.resize(hash);
        std::istringstream in(line);
        Command c;
        if (!(in >> c.op)) continue;
        bool ok = true;
        if (c.op == "rtc") {
            int y, mo, d, h, mi, s;
            std::string date, time;
            ok = bool(in >> date >> time) && sscanf(date.c_str(), "%d-%d-%d", &y, &mo, &d) == 3 &&
                 sscanf(time.c_str(), "%d:%d:%d", &h, &mi, &s) == 3;
            if (ok) {
                c.a = bcd(d) | (bcd(mo) << 8) | (bcd(y % 100) << 16);
                c.b = bcd(s) | (bcd(mi) << 8) | (bcd(h) << 16) | (uint32_t(dayOfWeek(y, mo, d)) << 24);
            }
        } else if (c.op == "wait") {
            ok = bool(in >> c.b);
        } else if (c.op == "hold") {
            ok = bool(in >> std::hex >> c.a >> std::dec >> c.b);
        } else if (c.op == "press") {
            ok = bool(in >> std::hex >> c.a);
        } else if (c.op != "reboot" && c.op != "end") {
            ok = false;
        }
        if (!ok) {
            fprintf(stderr, "pkrun: %s:%d: bad line\n", path.c_str(), lineNo);
            exit(2);
        }
        out.push_back(c);
    }
    return out;
}

/*----------------------
 | expand
 | Description: Turns script commands into per-sample steps; leading rtc lines are returned for the power-on clock.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: commands -- parsed script; r -- the Runner to fill
 | Returns: N/A
 ----------------------*/
static void expand(const std::vector<Command>& commands, Runner& r) {
    for (const Command& c : commands) {
        if (c.op == "end") break;
        if (c.op == "rtc") {
            r.steps.push_back({Step::Rtc, c.a, c.b});
        } else if (c.op == "wait") {
            for (uint32_t i = 0; i < c.b; i++) r.steps.push_back({Step::Sample, 0});
        } else if (c.op == "hold") {
            for (uint32_t i = 0; i < c.b; i++) r.steps.push_back({Step::Sample, c.a});
        } else if (c.op == "press") {
            r.steps.push_back({Step::Sample, c.a});
            r.steps.push_back({Step::Sample, 0});
        } else if (c.op == "reboot") {
            r.steps.push_back({Step::Reboot});
        }
    }
}

/*----------------------
 | ramCrc
 | Description: CRC32 of the app's work RAM from 0x200 up to the stack area, with the interrupt-paced ranges (the sound driver's playback state) zeroed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: r -- the Runner
 | Returns: the CRC
 ----------------------*/
static uint32_t ramCrc(Runner& r) {
    std::vector<uint8_t> ram(r.ps.wramData() + 0x200, r.ps.wramData() + r.stackLow);
    for (const auto& span : r.ramSkip) {
        for (uint32_t a = span.first; a < span.second && a < r.stackLow; a++) ram[a - 0x200] = 0;
    }
    return crc32(ram.data(), ram.size());
}

/*----------------------
 | advance
 | Description: Applies script steps up to the next sample: clock changes now, a reboot or the end by jumping to the top level.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: r -- the Runner
 | Returns: N/A
 ----------------------*/
static void advance(Runner& r) {
    while (r.next < r.steps.size() && r.steps[r.next].kind != Step::Sample) {
        const Step& s = r.steps[r.next];
        if (s.kind == Step::Rtc) {
            r.ps.setRtc(s.a, s.b);
            r.next++;
        } else {
            r.next++;
            std::longjmp(r.top, 2);
        }
    }
    if (r.next >= r.steps.size()) std::longjmp(r.top, 1);
    r.buttons = r.steps[r.next].a;
    r.ps.setButtons(r.buttons);
}

/*----------------------
 | takeSample
 | Description: Records one sample with the buttons held during it (dumping RAM first at sample PKRUN_DUMP_SAMPLE), then moves the script on.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: r -- the Runner
 | Returns: N/A
 ----------------------*/
static void takeSample(Runner& r) {
    static const char* dumpAt = getenv("PKRUN_DUMP_SAMPLE");
    if (dumpAt && uint32_t(atoi(dumpAt)) == r.samples) {
        for (int a = 0x200; a < 0x800; a += 16) {
            printf("PSK: ram %03X", a);
            for (int k = 0; k < 16; k++) printf(" %02X", r.ps.wramData()[a + k]);
            printf("\n");
        }
    }
    put32(r.trace, r.samples);
    put32(r.trace, r.buttons);
    r.trace.insert(r.trace.end(), r.ps.vram(), r.ps.vram() + 128);
    put32(r.trace, ramCrc(r));
    r.samples++;
    r.activeCycles = 0;
    r.next++;
    advance(r);
}

/*----------------------
 | checkStop
 | Description: Exits when host time runs out.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: r -- the Runner
 | Returns: N/A
 ----------------------*/
static void checkStop(Runner& r) {
    if (std::chrono::steady_clock::now() > r.deadline) {
        printf("PSK: timeout at sample %u\n", r.samples);
        exit(EXIT_TIMEOUT);
    }
}

/*----------------------
 | runStep
 | Description: Runs one instruction (or one C replacement): entry RAM dump and SWI log results first, frame-point sample, then halt or timeout samples.
 | Author: suinevere
 | Dependencies: psk_bus_emu.h
 | Globals: g_runner
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void runStep() {
    Runner& r = *g_runner;
    const bool halted = r.ps.halted();
    if (!halted) {
        const uint32_t pc = r.ps.pc();
        if (pc == r.frameAddr && !r.ps.thumb() && !pskemu::hooked(pc)) {
            if (r.frameArmed) {
                r.frameArmed = false;
                r.byFrame++;
                takeSample(r);
            }
        } else {
            r.frameArmed = true;
        }
        if (!r.appEntered && pc - 0x02000000u < 0x20000u) {
            r.appEntered = true;
            if (r.idleTime) r.ps.setIdleTime(true);
            if (!r.entryStatePath.empty()) {
                std::ofstream out(r.entryStatePath, std::ios::binary | std::ios::app);
                const auto w = r.ps.deviceWords();
                const uint32_t n = uint32_t(w.size());
                out.write(reinterpret_cast<const char*>(r.ps.wramData()), 0x800);
                out.write(reinterpret_cast<const char*>(&n), 4);
                out.write(reinterpret_cast<const char*>(w.data()), std::streamsize(4 * w.size()));
            }
        }
        if (!r.entryDumped && pc - 0x02000000u < 0x20000u) {
            r.entryDumped = true;
            if (!r.entryRamPath.empty()) {
                std::ofstream out(r.entryRamPath, std::ios::binary);
                out.write(reinterpret_cast<const char*>(r.ps.wramData()), 0x200);
            }
        }
        if (r.swiPending && pc == r.swiRet && r.ps.thumb() == r.swiThumb && (r.ps.cpsr() & 0x1F) == r.swiMode) {
            r.swiPending = false;
            fprintf(r.swiLog, "%02X %08X %08X %08X %08X", r.swiN, r.swiIn[0], r.swiIn[1], r.swiIn[2], r.ps.reg(0));
            for (uint8_t b : r.swiSrc) fprintf(r.swiLog, " %02X", b);
            fprintf(r.swiLog, "\n");
        }
        if (r.swiProbe && pc == r.swiProbe && !r.swiProbePending) {
            r.swiProbePending = true;
            if (r.swiProbeC) {
                const uint32_t n = r.ps.read32(pc) & 0xFF;
                const uint32_t v = psk_swi(n, r.ps.reg(0), r.ps.reg(1), r.ps.reg(2));
                r.ps.setReg(0, v);
                r.ps.setPc(pc + 4, false);
                printf("PSK: swi-probe %08X r0=%08X\n", pc, v);
                return;
            }
        } else if (r.swiProbePending && r.swiProbe && pc == r.swiProbe + 4 && !r.ps.thumb()) {
            printf("PSK: swi-probe %08X r0=%08X\n", r.swiProbe, r.ps.reg(0));
            r.swiProbe = 0;
        }
    }
    pskemu::idleEdge();
    const uint32_t cycles = r.ps.stepOne();
    r.seconds += double(cycles) / r.ps.armClockHz();
    if (!halted) r.activeCycles += cycles;
    const bool nowHalted = r.ps.halted();
    if (nowHalted && !r.lastHalted) {
        r.lastHalted = true;
        r.byHalt++;
        takeSample(r);
    }
    r.lastHalted = nowHalted;
    if (r.timeoutMs > 0 && r.activeCycles >= uint64_t(r.timeoutMs * r.ps.armClockHz() / 1000.0)) {
        r.byTimeout++;
        takeSample(r);
    }
    if ((++r.steps_run & 0xFFFFF) == 0) checkStop(r);
}

/*----------------------
 | onStep
 | Description: Step hook: coverage, PC ring, byte watch and stack depth, then C dispatch for hooked addresses.
 | Author: suinevere
 | Dependencies: psk_bus_emu.h
 | Globals: N/A
 | Params: ctx -- the Runner
 | Returns: true when a C replacement ran in place of the instruction
 ----------------------*/
static bool onStep(void* ctx) {
    auto* r = static_cast<Runner*>(ctx);
    if (!r->ps.halted()) {
        const uint32_t pc = r->ps.pc();
        r->pcRing[r->pcPos++ & 4095] = pc | (r->ps.thumb() ? 1u : 0u);
        if (pc - 0x02000000u < 0x20000u) r->coverage[(pc - 0x02000000u) >> 1] = r->ps.thumb() ? 2 : 1;
        const bool app = pc - 0x02000000u < 0x20000u;
        if (r->drive && app && !r->lastApp && !pskemu::inSandbox() && !pskemu::hooked(pc | (r->ps.thumb() ? 1u : 0u))) {
            r->armEntries.insert(pc | (r->ps.thumb() ? 1u : 0u));
        }
        r->lastApp = app;
        if ((r->ps.cpsr() & 0x1F) == 0x10 && r->ps.reg(13) < r->minSp && r->ps.reg(13) >= 0x200) r->minSp = r->ps.reg(13);
    }
    if (r->watch >= 0) {
        const uint8_t v = r->ps.wramData()[r->watch];
        if (v != r->watchLast) {
            printf("PSK: watch %03X %02X->%02X after pc %08X sample %u\n", r->watch, r->watchLast, v, r->watchPrevPc, r->samples);
            r->watchLast = v;
        }
        r->watchPrevPc = r->ps.pc();
    }
    return pskemu::dispatch();
}

/*----------------------
 | onSwi
 | Description: SWI hook: notes the call for the SWI log, prints the census line the first time a number appears, stops on an SWI with no handler.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ctx -- the Runner; number -- SWI number
 | Returns: N/A
 ----------------------*/
static void onSwi(void* ctx, uint32_t number) {
    auto* r = static_cast<Runner*>(ctx);
    if (r->swiLog && !pskemu::inSandbox() && !r->swiPending) {
        r->swiPending = true;
        r->swiN = number;
        for (int i = 0; i < 3; i++) r->swiIn[i] = r->ps.reg(i);
        r->swiThumb = r->ps.thumb();
        r->swiRet = r->ps.pc() + (r->swiThumb ? 2 : 4);
        r->swiMode = r->ps.cpsr() & 0x1F;
        r->swiSrc.clear();
        if (number == 3 || number == 0x10) {
            for (uint32_t i = 0; i < 0x80; i++) r->swiSrc.push_back(r->ps.read8(r->swiIn[1] + i));
        }
    }
    if (r->seenSwis.insert(number).second) {
        printf("PSK: swi %u r0=%08X r1=%08X r2=%08X pc=%08X\n", number, r->ps.reg(0), r->ps.reg(1), r->ps.reg(2),
               r->ps.pc());
    }
    if (r->missingSwis.count(number)) {
        printf("PSK: unimplemented swi %u at pc %08X sample %u\n", number, r->ps.pc(), r->samples);
        exit(EXIT_SWI);
    }
}

/*----------------------
 | frameFromC
 | Description: psk_frame from a C replacement: the same frame-point sample the ARM PC check takes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_runner
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void frameFromC() {
    g_runner->byFrame++;
    takeSample(*g_runner);
}

/*----------------------
 | sampleIndex
 | Description: Index of the sample being run.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_runner
 | Params: N/A
 | Returns: sample index
 ----------------------*/
static uint32_t sampleIndex() { return g_runner->samples; }

/*----------------------
 | dumpPcs
 | Description: At exit, prints the last 4096 executed PCs (bit 0 = Thumb), the registers and work RAM when PKRUN_PCLOG is set.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_runner
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void dumpPcs() {
    if (!g_runner || !g_runner->pcLog) return;
    for (uint32_t i = 0; i < 4096; i++) printf("PSK: pc %08X\n", g_runner->pcRing[(g_runner->pcPos + i) & 4095]);
    for (int i = 0; i < 16; i++) printf("PSK: r%d %08X\n", i, g_runner->ps.reg(i));
    for (int a = 0; a < 0x800; a += 16) {
        printf("PSK: ram %03X", a);
        for (int k = 0; k < 16; k++) printf(" %02X", g_runner->ps.wramData()[a + k]);
        printf("\n");
    }
}

/*----------------------
 | findRunnerBlock
 | Description: Finds the launcher's marked idle loop and SWI stub block in the kernel ROM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: kernel -- ROM bytes
 | Returns: its address at 0x04000000, or exits 2 when the ROM lacks it
 ----------------------*/
static uint32_t findRunnerBlock(const std::vector<uint8_t>& kernel) {
    for (size_t i = 0; i + 4 <= kernel.size(); i += 4) {
        uint32_t w;
        std::memcpy(&w, &kernel[i], 4);
        if (w == RUNNER_MAGIC) return 0x04000000u + uint32_t(i);
    }
    fprintf(stderr, "pkrun: kernel has no runner block\n");
    exit(2);
}

/*----------------------
 | main
 | Description: pkrun --kernel K --card C --script S --trace OUT [--max-seconds N] [--coverage FILE] [--hooks | --verify] [--hooks-exclude A,B] [--frame ADDR] [--timeout-ms N] [--stack-low ADDR] [--ram-skip A-B,...] [--swi-probe ADDR | --swi-probe-c ADDR] [--swi-log FILE] [--dump-entry-ram FILE] [--idle-time] [--dump-entry-state FILE]
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_runner
 | Params: argc, argv -- command line
 | Returns: 0 on success, 2 bad input, 3 timeout, 4 unimplemented SWI, 5 a C call returned elsewhere, 6 a verified C differs
 ----------------------*/
int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IOLBF, 0);
    std::string kernel, card, script, tracePath, coveragePath;
    int maxSeconds = 600;
    pskemu::Mode mode = pskemu::Mode::Off;
    std::vector<uint32_t> excluded;
    static Runner r;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                fprintf(stderr, "pkrun: %s needs a value\n", a.c_str());
                exit(2);
            }
            return argv[++i];
        };
        if (a == "--kernel") kernel = next();
        else if (a == "--card") card = next();
        else if (a == "--script") script = next();
        else if (a == "--trace") tracePath = next();
        else if (a == "--max-seconds") maxSeconds = atoi(next().c_str());
        else if (a == "--coverage") coveragePath = next();
        else if (a == "--hooks") {
            mode = pskemu::Mode::Drive;
            r.drive = true;
        }
        else if (a == "--verify") mode = pskemu::Mode::Verify;
        else if (a == "--hooks-exclude") {
            std::stringstream ss(next());
            std::string item;
            while (std::getline(ss, item, ',')) excluded.push_back(uint32_t(strtoul(item.c_str(), nullptr, 16)));
        } else if (a == "--frame") r.frameAddr = uint32_t(strtoul(next().c_str(), nullptr, 16));
        else if (a == "--timeout-ms") r.timeoutMs = atof(next().c_str());
        else if (a == "--stack-low") r.stackLow = uint32_t(strtoul(next().c_str(), nullptr, 16));
        else if (a == "--ram-skip") {
            r.ramSkip.clear();
            std::stringstream ss(next());
            std::string item;
            while (std::getline(ss, item, ',')) {
                const auto dash = item.find('-');
                if (dash == std::string::npos) continue;
                r.ramSkip.push_back({uint32_t(strtoul(item.substr(0, dash).c_str(), nullptr, 16)),
                                     uint32_t(strtoul(item.substr(dash + 1).c_str(), nullptr, 16))});
            }
        }
        else if (a == "--swi-log") {
            r.swiLog = fopen(next().c_str(), "w");
            if (!r.swiLog) {
                fprintf(stderr, "pkrun: cannot write the SWI log\n");
                return 2;
            }
        } else if (a == "--dump-entry-ram") r.entryRamPath = next();
        else if (a == "--idle-time") r.idleTime = true;
        else if (a == "--dump-entry-state") {
            r.entryStatePath = next();
            std::ofstream(r.entryStatePath, std::ios::binary | std::ios::trunc);
        }
        else if (a == "--swi-probe") r.swiProbe = uint32_t(strtoul(next().c_str(), nullptr, 16));
        else if (a == "--swi-probe-c") {
            r.swiProbe = uint32_t(strtoul(next().c_str(), nullptr, 16));
            r.swiProbeC = true;
        }
        else {
            fprintf(stderr, "pkrun: unknown option %s\n", a.c_str());
            return 2;
        }
    }
    if (kernel.empty() || card.empty() || script.empty() || tracePath.empty()) {
        fprintf(stderr, "usage: pkrun --kernel K --card C --script S --trace OUT [options]\n");
        return 2;
    }
    auto kernelData = readFile(kernel);
    auto cardData = readFile(card);
    expand(parseScript(script), r);

    g_runner = &r;
    r.pcLog = getenv("PKRUN_PCLOG") != nullptr;
    if (const char* w = getenv("PKRUN_WATCH")) r.watch = int(strtoul(w, nullptr, 16)) & 0x7FF;
    atexit(dumpPcs);
    r.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(maxSeconds);
    for (uint32_t n : {5u, 9u, 18u}) r.missingSwis.insert(n);
    for (uint32_t n = 0x19; n < 0x100; n++) r.missingSwis.insert(n);
    r.ps.setKernel(kernelData.data(), kernelData.size());
    r.ps.setFlash(cardData.data(), cardData.size());
    while (r.next < r.steps.size() && r.steps[r.next].kind == Step::Rtc) {
        r.ps.setRtc(r.steps[r.next].a, r.steps[r.next].b);
        r.next++;
    }
    pskemu::attach({&r.ps, runStep, frameFromC, sampleIndex}, findRunnerBlock(kernelData));
    pskemu::setMode(mode);
    pskemu::setRamCompare(r.stackLow, r.ramSkip);
    for (uint32_t x : excluded) pskemu::exclude(x);
    r.ps.reset();
    r.ps.setStepHook(onStep, &r);
    r.ps.setSwiHook(onSwi, &r);
    r.trace.insert(r.trace.end(), {'P', 'K', 'T', 'R', '1'});
    const int how = setjmp(r.top);
    if (how != 1) {
        if (how == 2) {
            r.ps.setRtc(r.ps.rtcDate(), r.ps.rtcTime());
            r.ps.setButtons(0);
            r.ps.reset();
            r.lastHalted = false;
            r.appEntered = false;
            r.frameArmed = true;
            r.activeCycles = 0;
        }
        advance(r);
        for (;;) runStep();
    }
    put32(r.trace, crc32(r.ps.flashData(), r.ps.flashSize()));
    std::ofstream out(tracePath, std::ios::binary);
    out.write(reinterpret_cast<const char*>(r.trace.data()), std::streamsize(r.trace.size()));
    if (!out) {
        fprintf(stderr, "pkrun: cannot write %s\n", tracePath.c_str());
        return 2;
    }
    if (!coveragePath.empty()) {
        FILE* f = fopen(coveragePath.c_str(), "w");
        if (!f) {
            fprintf(stderr, "pkrun: cannot write %s\n", coveragePath.c_str());
            return 2;
        }
        for (uint32_t i = 0; i < r.coverage.size(); i++) {
            if (r.coverage[i]) fprintf(f, "%08X\n", (0x02000000u + (i << 1)) | (r.coverage[i] == 2 ? 1u : 0u));
        }
        fclose(f);
    }
    if (r.swiLog) fclose(r.swiLog);
    printf("PSK: samples frame %u halt %u timeout %u\n", r.byFrame, r.byHalt, r.byTimeout);
    printf("PSK: min sp %08X\n", r.minSp);
    printf("PSK: verified calls %u\n", pskemu::verifiedCalls());
    if (r.drive) {
        printf("PSK: drive ran ARM app code from %zu entries:", r.armEntries.size());
        for (uint32_t e : r.armEntries) printf(" %08X", e);
        printf("\n");
    }
    printf("PSK: done samples %u seconds %.1f\n", r.samples, r.seconds);
    return 0;
}
