/*----------------------
 | psk_hw.h
 | Description: The only way decompiled PocketStation C reaches memory, hardware, time and not-yet-decompiled code.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef PSK_HW_H
#define PSK_HW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------
 | PskCpu
 | Description: ARM register file for register-level functions: r0-r15 (r15 = where execution continues, bit 0 = Thumb) and NZCV in bits 31-28 of flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct PskCpu {
    uint32_t r[16];
    uint32_t flags;
} PskCpu;

/*----------------------
 | psk_r8
 | Description: Reads a byte of PocketStation memory or I/O.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: addr -- address
 | Returns: the byte
 ----------------------*/
uint8_t psk_r8(uint32_t addr);

/*----------------------
 | psk_r16
 | Description: Reads a halfword of PocketStation memory or I/O.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: addr -- address
 | Returns: the halfword
 ----------------------*/
uint16_t psk_r16(uint32_t addr);

/*----------------------
 | psk_r32
 | Description: Reads a word of PocketStation memory or I/O.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: addr -- address
 | Returns: the word
 ----------------------*/
uint32_t psk_r32(uint32_t addr);

/*----------------------
 | psk_w8
 | Description: Writes a byte of PocketStation memory or I/O.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: addr -- address; v -- value
 | Returns: N/A
 ----------------------*/
void psk_w8(uint32_t addr, uint8_t v);

/*----------------------
 | psk_w16
 | Description: Writes a halfword of PocketStation memory or I/O.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: addr -- address; v -- value
 | Returns: N/A
 ----------------------*/
void psk_w16(uint32_t addr, uint16_t v);

/*----------------------
 | psk_w32
 | Description: Writes a word of PocketStation memory or I/O.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: addr -- address; v -- value
 | Returns: N/A
 ----------------------*/
void psk_w32(uint32_t addr, uint32_t v);

/*----------------------
 | psk_frame
 | Description: Marks the point where the original waits for its next frame; the backend samples or presents the screen here.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void psk_frame(void);

/*----------------------
 | psk_idle
 | Description: Lets time pass and interrupts run inside a busy-wait, as the original's polling loop would.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void psk_idle(void);

/*----------------------
 | psk_call_arm
 | Description: Calls code that is still ARM with up to four arguments and returns r0; aborts when there is no ARM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: addr -- callee, bit 0 = Thumb; r0-r3 -- arguments
 | Returns: the callee's r0
 ----------------------*/
uint32_t psk_call_arm(uint32_t addr, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3);

/*----------------------
 | psk_call
 | Description: A register-level call (BL): sets lr to ret and runs target, decompiled or ARM, until it returns there.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file, updated by the callee; target -- callee, bit 0 = Thumb; ret -- return address, bit 0 = Thumb
 | Returns: N/A
 ----------------------*/
void psk_call(PskCpu *c, uint32_t target, uint32_t ret);

#ifdef __cplusplus
}
#endif

#endif
