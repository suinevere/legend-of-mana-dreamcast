/*----------------------
 | psk_bus_emu.h
 | Description: Runner-side control of the emulator backend: attaching the device, the hook table and the dispatcher.
 | Author: suinevere
 | Dependencies: pocketstation.h (patched), psk_hw.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef PSK_BUS_EMU_H
#define PSK_BUS_EMU_H

#include <cstdint>
#include <utility>
#include <vector>

#include "pocketstation.h"
#include "psk_hw.h"

/*----------------------
 | PskHook
 | Description: One hooked address (bit 0 = Thumb), the function that replaces the code there, whether it is register-level or returns r0, and which registers (bits 0-15) and flags (bit 16) a register-level hook's callers read.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
struct PskHook {
    uint32_t addr;
    void (*fn)(PskCpu *c);
    const char *name;
    int regs;
    int result;
    uint32_t live;
};

extern "C" const PskHook psk_hooks[];
extern "C" const int psk_hook_count;

namespace pskemu {

/*----------------------
 | Host
 | Description: What the runner lends the backend: the device, one-instruction stepping with sampling, and the frame sample.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
struct Host {
    PCSX::PocketStation::PocketStation *ps;
    void (*runStep)();
    void (*frame)();
    uint32_t (*sampleIndex)();
};

/*----------------------
 | Mode
 | Description: Off: hooks ignored. Drive: C runs in place of the ARM. Verify: the ARM runs, and every hooked call is first checked against its C in a sandbox.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum class Mode { Off, Drive, Verify };

void attach(const Host &host, uint32_t runnerBlock);
void setMode(Mode mode);
void setRamCompare(uint32_t high, const std::vector<std::pair<uint32_t, uint32_t>> &skip);
void exclude(uint32_t addr);
bool hooked(uint32_t addrWithThumbBit);
bool dispatch();
uint32_t verifiedCalls();
void idleEdge();
bool inSandbox();

}  // namespace pskemu

#endif
