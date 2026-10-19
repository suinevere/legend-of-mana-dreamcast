/*----------------------
 | psk_bus_emu.cc
 | Description: psk_hw.h and psk_swi.h on the reference emulator: memory through the bus, ARM call-outs, idling, hook dispatch, and per-call verification of C against ARM.
 | Author: suinevere
 | Dependencies: psk_bus_emu.h, psk_hw.h, psk_swi.h, pocketstation.h (patched)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/

#include "psk_bus_emu.h"

#include <algorithm>
#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <unordered_map>

#include "psk_swi.h"

/*----------------------
 | SENTINEL
 | Description: Return address psk_call_arm gives ARM code; the vector at 4 is never a real return target.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static constexpr uint32_t SENTINEL = 0x00000004;

/*----------------------
 | IDLE_LIMIT
 | Description: Most steps psk_idle runs when no interrupt comes back to the idle loop.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static constexpr uint64_t IDLE_LIMIT = 4000000;

/*----------------------
 | ARM_LIMIT
 | Description: Most steps a sandboxed ARM run may take to reach the C replacement's continuation.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static constexpr uint64_t ARM_LIMIT = 20000000;

/*----------------------
 | EXIT_MISMATCH
 | Description: Exit code when verification finds a C replacement that differs from its ARM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static constexpr int EXIT_MISMATCH = 6;

/*----------------------
 | State
 | Description: The attached device, mode, hook index, exclusions, sandbox flags and the RAM comparison window.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
struct State {
    pskemu::Host host{};
    uint32_t idle = 0;
    uint32_t stubs = 0;
    pskemu::Mode mode = pskemu::Mode::Off;
    std::unordered_map<uint32_t, const PskHook *> table;
    std::set<uint32_t> excluded;
    std::set<uint32_t> called;
    bool sandbox = false;
    bool sandboxIrqs = false;
    std::jmp_buf restart;
    uint32_t ramHigh = 0x700;
    std::vector<std::pair<uint32_t, uint32_t>> ramSkip;
    uint32_t verified = 0;
    uint32_t gateMode = 0;
};

static State g;

/*----------------------
 | Saved
 | Description: A copy of the visible register file, flags and PC.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
struct Saved {
    uint32_t r[15];
    uint32_t flags;
    uint32_t pc;
    bool thumb;
};

/*----------------------
 | Outcome
 | Description: What one run of a function left behind: registers, flags, continuation, RAM, live stack, VRAM, I/O setup and flash writes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
struct Outcome {
    uint32_t r[15];
    uint32_t flags;
    uint32_t pc;
    std::vector<uint8_t> ram;
    std::vector<uint8_t> stack;
    std::vector<uint8_t> vram;
    std::vector<uint32_t> io;
    std::vector<std::pair<uint32_t, uint16_t>> flash;
};

/*----------------------
 | save
 | Description: Copies the device's registers, flags and PC.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: Saved
 ----------------------*/
static Saved save() {
    Saved s;
    for (int i = 0; i < 15; i++) s.r[i] = g.host.ps->reg(i);
    s.flags = g.host.ps->cpsr() & 0xF0000000u;
    s.pc = g.host.ps->pc();
    s.thumb = g.host.ps->thumb();
    return s;
}

/*----------------------
 | restore
 | Description: Puts back registers, flags and PC saved by save().
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: s -- saved state
 | Returns: N/A
 ----------------------*/
static void restore(const Saved &s) {
    for (int i = 0; i < 15; i++) g.host.ps->setReg(i, s.r[i]);
    g.host.ps->setFlags(s.flags);
    g.host.ps->setPc(s.pc, s.thumb);
}

/*----------------------
 | pcKey
 | Description: The current PC with bit 0 set in Thumb state.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: address key
 ----------------------*/
static uint32_t pcKey() { return g.host.ps->pc() | (g.host.ps->thumb() ? 1u : 0u); }

/*----------------------
 | find
 | Description: The hook for an address key, or null when unhooked, excluded or hooks are off.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: key -- address, bit 0 = Thumb
 | Returns: hook or nullptr
 ----------------------*/
static const PskHook *find(uint32_t key) {
    if (g.mode == pskemu::Mode::Off || g.excluded.count(key & ~1u)) return nullptr;
    auto it = g.table.find(key);
    return it == g.table.end() ? nullptr : it->second;
}

/*----------------------
 | IDLE_EDGES
 | Description: Back edges of the loops that wait for an interrupt (Thumb bit set), the same ones the lifter gives psk_idle().
 | Author: suinevere
 | Dependencies: psk_lift.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static constexpr uint32_t IDLE_EDGES[] = {0x02003E64u, 0x02003D01u, 0x020038D0u};

/*----------------------
 | branchTaken
 | Description: Whether the conditional branch at an idle edge will loop, from its condition field and the flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: key -- edge address, bit 0 = Thumb
 | Returns: true when the branch is taken
 ----------------------*/
static bool branchTaken(uint32_t key) {
    PCSX::PocketStation::PocketStation &ps = *g.host.ps;
    const uint32_t cond = (key & 1u) ? (ps.read16(key & ~1u) >> 8) & 15u : ps.read32(key) >> 28;
    const uint32_t f = ps.cpsr();
    const bool n = f >> 31, z = (f >> 30) & 1, c = (f >> 29) & 1, v = (f >> 28) & 1;
    switch (cond) {
        case 0: return z;
        case 1: return !z;
        case 2: return c;
        case 3: return !c;
        case 4: return n;
        case 5: return !n;
        case 6: return v;
        case 7: return !v;
        case 8: return c && !z;
        case 9: return !c || z;
        case 10: return n == v;
        case 11: return n != v;
        case 12: return !z && n == v;
        case 13: return z || n != v;
        default: return true;
    }
}

/*----------------------
 | deliverable
 | Description: Pending interrupts the CPU would take now: FIQ sources unless CPSR.F, IRQ sources unless CPSR.I.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: interrupt bits
 ----------------------*/
static uint32_t deliverable() {
    const uint32_t cpsr = g.host.ps->cpsr();
    return g.host.ps->pendingIrqs() & (((cpsr & 0x40u) ? 0u : 0x2040u) | ((cpsr & 0x80u) ? 0u : 0x1F9Fu));
}

/*----------------------
 | edge
 | Description: Idle-time schedule before each instruction: closes the interrupt gate once the CPU is back in the mode it idled from with nothing deliverable, and at a looping idle edge advances the devices to their next event and opens it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void edge() {
    PCSX::PocketStation::PocketStation &ps = *g.host.ps;
    if (!ps.idleTime() || ps.halted()) return;
    if (ps.irqGateOpen()) {
        if ((ps.cpsr() & 0x1Fu) == g.gateMode && deliverable() == 0) ps.setIrqGate(false);
        return;
    }
    const uint32_t key = pcKey();
    bool hit = false;
    for (uint32_t e : IDLE_EDGES) hit = hit || e == key;
    if (!hit || !branchTaken(key)) return;
    ps.advanceToEvent();
    if (deliverable()) {
        g.gateMode = ps.cpsr() & 0x1Fu;
        ps.setIrqGate(true);
    }
}

/*----------------------
 | step
 | Description: One instruction: raw (no sampling, no hooks) inside the sandbox, through the runner otherwise.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void step() {
    if (g.sandbox) {
        edge();
        g.host.ps->stepRaw();
    } else {
        g.host.runStep();
    }
}

/*----------------------
 | maskIrqs
 | Description: Sets the CPSR I and F bits so no interrupt is taken.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void maskIrqs() {
    auto s = g.host.ps->saveState();
    s.cpu.cpsr |= 0xC0u;
    g.host.ps->loadState(s);
}

/*----------------------
 | toCpu
 | Description: Loads a register file into the device and continues at its r15.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: c -- register file
 | Returns: N/A
 ----------------------*/
static void toCpu(const PskCpu &c) {
    for (int i = 0; i < 15; i++) g.host.ps->setReg(i, c.r[i]);
    g.host.ps->setFlags(c.flags);
    g.host.ps->setPc(c.r[15] & ~1u, c.r[15] & 1u);
}

/*----------------------
 | fromCpu
 | Description: A register file from the device, r15 preset to lr as for a call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: PskCpu
 ----------------------*/
static PskCpu fromCpu() {
    PskCpu c;
    for (int i = 0; i < 15; i++) c.r[i] = g.host.ps->reg(i);
    c.r[15] = c.r[14];
    c.flags = g.host.ps->cpsr() & 0xF0000000u;
    return c;
}

/*----------------------
 | capture
 | Description: Records the device after a run: registers, flags, continuation, compared RAM, live stack from stackLow, VRAM, I/O and flash writes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: stackLow -- lowest stack address to compare; undo -- flash writes made during the run
 | Returns: Outcome
 ----------------------*/
static Outcome capture(uint32_t stackLow, const std::vector<std::pair<uint32_t, uint16_t>> &undo) {
    Outcome o;
    for (int i = 0; i < 15; i++) o.r[i] = g.host.ps->reg(i);
    o.flags = g.host.ps->cpsr() & 0xF0000000u;
    o.pc = pcKey();
    const uint8_t *w = g.host.ps->wramData();
    o.ram.assign(w + 0x200, w + g.ramHigh);
    for (const auto &span : g.ramSkip) {
        for (uint32_t a = span.first; a < span.second && a < g.ramHigh; a++) o.ram[a - 0x200] = 0;
    }
    if (stackLow < 0x800) o.stack.assign(w + stackLow, w + 0x800);
    o.vram.assign(g.host.ps->vram(), g.host.ps->vram() + 128);
    o.io = g.host.ps->ioSignature();
    for (const auto &u : undo) {
        uint16_t v = uint16_t(g.host.ps->flashData()[u.first] | (g.host.ps->flashData()[u.first + 1] << 8));
        o.flash.push_back({u.first, v});
    }
    return o;
}

/*----------------------
 | undoFlash
 | Description: Puts back every flash halfword a sandboxed run programmed, newest first.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: undo -- old values in write order
 | Returns: N/A
 ----------------------*/
static void undoFlash(std::vector<std::pair<uint32_t, uint16_t>> &undo) {
    uint8_t *f = g.host.ps->flashDataMutable();
    for (auto it = undo.rbegin(); it != undo.rend(); ++it) {
        f[it->first] = uint8_t(it->second);
        f[it->first + 1] = uint8_t(it->second >> 8);
    }
    undo.clear();
}

/*----------------------
 | differ
 | Description: Describes the first difference between the C and ARM outcomes, or returns an empty string.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c, a -- outcomes; regs -- whether all registers and flags count (register-level); result -- whether r0 is a result
 | Returns: description
 ----------------------*/
static std::string differ(const Outcome &c, const Outcome &a, bool regs, bool result) {
    char buf[160];
    if (c.pc != a.pc) return "continuation";
    for (int i = 0; i < 15; i++) {
        if ((regs || (i == 0 && result) || i == 13) && c.r[i] != a.r[i]) {
            snprintf(buf, sizeof buf, "r%d C %08X ARM %08X", i, c.r[i], a.r[i]);
            return buf;
        }
    }
    if (regs && c.flags != a.flags) {
        snprintf(buf, sizeof buf, "flags C %08X ARM %08X", c.flags, a.flags);
        return buf;
    }
    for (size_t i = 0; i < c.ram.size(); i++) {
        if (c.ram[i] != a.ram[i]) {
            snprintf(buf, sizeof buf, "ram %03X C %02X ARM %02X", unsigned(0x200 + i), c.ram[i], a.ram[i]);
            return buf;
        }
    }
    if (c.stack != a.stack) {
        for (size_t i = 0; i < c.stack.size(); i++) {
            if (c.stack[i] != a.stack[i]) {
                snprintf(buf, sizeof buf, "stack %03X C %02X ARM %02X", unsigned(0x800 - c.stack.size() + i), c.stack[i],
                         a.stack[i]);
                return buf;
            }
        }
    }
    if (c.vram != a.vram) return "vram";
    if (c.io != a.io) return "io";
    if (c.flash != a.flash) return "flash";
    return "";
}

/*----------------------
 | verify
 | Description: Runs a hooked function's C and then its ARM from one snapshot, interrupts masked unless the C idles, compares, and restores the snapshot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: h -- the hook at the current PC
 | Returns: N/A (exits with EXIT_MISMATCH on a difference)
 ----------------------*/
static void verify(const PskHook *h) {
    PCSX::PocketStation::PocketStation &ps = *g.host.ps;
    const auto snap = ps.saveState();
    const uint32_t entrySp = ps.reg(13);
    const uint32_t gateMode = g.gateMode;
    std::vector<std::pair<uint32_t, uint16_t>> undo;
    ps.setFlashUndo(&undo);
    g.sandbox = true;
    g.sandboxIrqs = ps.idleTime();
    if (setjmp(g.restart)) {
        undoFlash(undo);
        ps.loadState(snap);
        g.gateMode = gateMode;
        g.sandboxIrqs = true;
    }
    if (!g.sandboxIrqs) maskIrqs();
    PskCpu c = fromCpu();
    h->fn(&c);
    toCpu(c);
    const uint32_t low = std::min(entrySp, c.r[13]) & ~3u;
    const Outcome oc = capture(low, undo);
    undoFlash(undo);
    ps.loadState(snap);
    g.gateMode = gateMode;
    if (!g.sandboxIrqs) maskIrqs();
    uint64_t n = 0;
    while (!(pcKey() == oc.pc && ps.reg(13) == oc.r[13]) && n < ARM_LIMIT) {
        edge();
        ps.stepRaw();
        n++;
    }
    const Outcome oa = capture(low, undo);
    undoFlash(undo);
    ps.loadState(snap);
    g.gateMode = gateMode;
    ps.setFlashUndo(nullptr);
    g.sandbox = false;
    g.verified++;
    std::string d = n >= ARM_LIMIT ? std::string("ARM never reached the C continuation") :
                    differ(oc, oa, h->regs, h->result);
    if (!d.empty()) {
        printf("PSK: verify %08X mismatch sample %u: %s (continuation %08X)\n", h->addr & ~1u, g.host.sampleIndex(),
               d.c_str(), oc.pc);
        exit(EXIT_MISMATCH);
    }
}

/*----------------------
 | run
 | Description: Calls a hooked function on a register file, reporting its first call with the sample it fell in.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: h -- hook; c -- register file
 | Returns: N/A
 ----------------------*/
static void run(const PskHook *h, PskCpu *c) {
    if (g.called.insert(h->addr).second && !g.sandbox) {
        printf("PSK: hook %08X first call sample %u\n", h->addr & ~1u, g.host.sampleIndex());
    }
    h->fn(c);
}

namespace pskemu {

/*----------------------
 | attach
 | Description: Connects the backend to the runner and indexes the hook table; runnerBlock is the launcher's marked idle/stub block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: host -- runner services; runnerBlock -- address of the marker word
 | Returns: N/A
 ----------------------*/
void attach(const Host &host, uint32_t runnerBlock) {
    g.host = host;
    g.idle = runnerBlock + 4;
    g.stubs = runnerBlock + 8;
    for (int i = 0; i < psk_hook_count; i++) g.table[psk_hooks[i].addr] = &psk_hooks[i];
}

/*----------------------
 | setMode
 | Description: Chooses whether hooks are ignored, drive the run, or are verified against the ARM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: mode -- the mode
 | Returns: N/A
 ----------------------*/
void setMode(Mode mode) { g.mode = mode; }

/*----------------------
 | setRamCompare
 | Description: The RAM window verification compares: 0x200 up to high, with skipped spans zeroed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: high -- end of compared RAM; skip -- [start, end) spans
 | Returns: N/A
 ----------------------*/
void setRamCompare(uint32_t high, const std::vector<std::pair<uint32_t, uint32_t>> &skip) {
    g.ramHigh = high;
    g.ramSkip = skip;
}

/*----------------------
 | exclude
 | Description: Leaves one hooked address to the ARM code (for bisecting).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: addr -- function start
 | Returns: N/A
 ----------------------*/
void exclude(uint32_t addr) { g.excluded.insert(addr & ~1u); }

/*----------------------
 | hooked
 | Description: Whether C runs in place of the ARM at an address (drive mode only).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: addrWithThumbBit -- address, bit 0 = Thumb
 | Returns: bool
 ----------------------*/
bool hooked(uint32_t addrWithThumbBit) { return g.mode == Mode::Drive && find(addrWithThumbBit) != nullptr; }

/*----------------------
 | dispatch
 | Description: Step hook: in drive mode runs the C in place of the instruction; in verify mode checks the C and lets the ARM run.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: true when the C ran in place of the instruction
 ----------------------*/
bool dispatch() {
    if (g.mode == Mode::Off || g.sandbox || g.host.ps->halted()) return false;
    const PskHook *h = find(pcKey());
    if (!h) return false;
    if (g.mode == Mode::Verify) {
        if (g.called.insert(h->addr).second) {
            printf("PSK: hook %08X first call sample %u\n", h->addr & ~1u, g.host.sampleIndex());
        }
        verify(h);
        return false;
    }
    PskCpu c = fromCpu();
    run(h, &c);
    toCpu(c);
    return true;
}

/*----------------------
 | verifiedCalls
 | Description: Number of hooked calls verified so far.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: count
 ----------------------*/
uint32_t verifiedCalls() { return g.verified; }

void idleEdge() { edge(); }

/*----------------------
 | inSandbox
 | Description: Whether a verification sandbox run is in progress (its SWIs and frames are not the real run's).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: bool
 ----------------------*/
bool inSandbox() { return g.sandbox; }

}  // namespace pskemu

extern "C" {

/*----------------------
 | psk_r8 / psk_r16 / psk_r32 / psk_w8 / psk_w16 / psk_w32
 | Description: Memory and I/O access through the emulated bus.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: addr -- address; v -- value to write
 | Returns: the value read
 ----------------------*/
uint8_t psk_r8(uint32_t addr) { return g.host.ps->read8(addr); }
uint16_t psk_r16(uint32_t addr) { return g.host.ps->read16(addr); }
uint32_t psk_r32(uint32_t addr) { return g.host.ps->read32(addr); }
void psk_w8(uint32_t addr, uint8_t v) { g.host.ps->write8(addr, v); }
void psk_w16(uint32_t addr, uint16_t v) { g.host.ps->write16(addr, v); }
void psk_w32(uint32_t addr, uint32_t v) { g.host.ps->write32(addr, v); }

/*----------------------
 | psk_frame
 | Description: The frame sample; ignored inside the verification sandbox.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void psk_frame(void) {
    if (!g.sandbox) g.host.frame();
}

/*----------------------
 | psk_idle
 | Description: Runs the launcher's idle loop until an interrupt has come and gone; in a masked sandbox restarts the check with interrupts on.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void psk_idle(void) {
    if (g.host.ps->idleTime()) {
        const Saved s = save();
        g.host.ps->advanceToEvent();
        if (deliverable()) {
            g.gateMode = g.host.ps->cpsr() & 0x1Fu;
            g.host.ps->setIrqGate(true);
            g.host.ps->setPc(g.idle, false);
            for (uint64_t n = 0; n < IDLE_LIMIT && (g.host.ps->irqGateOpen() || g.host.ps->pc() != g.idle); n++) step();
        }
        restore(s);
        return;
    }
    if (g.sandbox && !g.sandboxIrqs) std::longjmp(g.restart, 1);
    const Saved s = save();
    g.host.ps->setPc(g.idle, false);
    bool left = false;
    for (uint64_t n = 0; n < IDLE_LIMIT; n++) {
        step();
        if (g.host.ps->pc() != g.idle) {
            left = true;
        } else if (left) {
            break;
        }
    }
    restore(s);
}

/*----------------------
 | psk_call_arm
 | Description: Runs ARM code at addr with r0-r3 until it returns to the sentinel; every register but r0 is put back.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: addr -- callee, bit 0 = Thumb; r0-r3 -- arguments
 | Returns: the callee's r0
 ----------------------*/
uint32_t psk_call_arm(uint32_t addr, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3) {
    const Saved s = save();
    g.host.ps->setReg(0, r0);
    g.host.ps->setReg(1, r1);
    g.host.ps->setReg(2, r2);
    g.host.ps->setReg(3, r3);
    g.host.ps->setReg(14, SENTINEL);
    g.host.ps->setPc(addr & ~1u, addr & 1u);
    while (g.host.ps->pc() != SENTINEL) step();
    const uint32_t result = g.host.ps->reg(0);
    restore(s);
    return result;
}

/*----------------------
 | psk_call
 | Description: A register-level call: decompiled code runs directly, continuation after continuation, and ARM code runs until it returns to ret at the same stack depth.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: c -- register file; target -- callee, bit 0 = Thumb; ret -- return address, bit 0 = Thumb
 | Returns: N/A
 ----------------------*/
void psk_call(PskCpu *c, uint32_t target, uint32_t ret) {
    const uint32_t sp = c->r[13];
    c->r[14] = ret;
    c->r[15] = target;
    while (!(c->r[15] == ret && c->r[13] == sp)) {
        const PskHook *h = g.mode == pskemu::Mode::Drive ? find(c->r[15]) : nullptr;
        if (h) {
            run(h, c);
            continue;
        }
        const Saved s = save();
        toCpu(*c);
        while (!(pcKey() == ret && g.host.ps->reg(13) == sp)) step();
        for (int i = 0; i < 15; i++) c->r[i] = g.host.ps->reg(i);
        c->flags = g.host.ps->cpsr() & 0xF0000000u;
        c->r[15] = ret;
        restore(s);
    }
}

/*----------------------
 | psk_swi
 | Description: A kernel call through the launcher's `swi #n; bx lr` stub.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g
 | Params: number -- SWI number; r0-r2 -- arguments
 | Returns: the kernel's r0
 ----------------------*/
uint32_t psk_swi(uint32_t number, uint32_t r0, uint32_t r1, uint32_t r2) {
    return psk_call_arm(g.stubs + 8u * (number & 0x1Fu), r0, r1, r2, 0);
}

}  // extern "C"
