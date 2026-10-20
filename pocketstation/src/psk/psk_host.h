/*----------------------
 | psk_host.h
 | Description: Control of the no-emulator PocketStation backend: loading a card, buttons, the clock, time, the screen and the card's CRC.
 | Author: suinevere
 | Dependencies: psk_hw.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef PSK_HOST_H
#define PSK_HOST_H

#include <stdint.h>

#include "psk_hw.h"

/*----------------------
 | PskHookEntry
 | Description: One decompiled function in the generated table: address (bit 0 = Thumb), entry, name, register-level flag, r0-result flag, live outputs (bits 0-15 registers, bit 16 flags).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint32_t addr;
    void (*fn)(PskCpu *c);
    const char *name;
    int regs;
    int result;
    uint32_t live;
} PskHookEntry;

extern const PskHookEntry psk_hooks[];
extern const int psk_hook_count;

/*----------------------
 | psk_host_init
 | Description: Starts the device: the card image, the kernel's RAM page as it stands at app entry, the app's flash banks and the launcher's interrupt and LCD setup.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: card -- 128 KB card image; kernel_ram -- 0x200 bytes
 | Returns: N/A
 ----------------------*/
void psk_host_init(const uint8_t *card, const uint8_t *kernel_ram);

/*----------------------
 | psk_host_set_buttons
 | Description: Holds the button mask (bits 0-4 Fire, Right, Left, Down, Up); a press raises the input level and its interrupt.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mask -- buttons held
 | Returns: N/A
 ----------------------*/
void psk_host_set_buttons(uint32_t mask);

/*----------------------
 | psk_host_set_rtc
 | Description: Sets the clock in RTC_DATE / RTC_TIME BCD layout.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: date -- day, month, year; time -- seconds, minutes, hours, day of week
 | Returns: N/A
 ----------------------*/
void psk_host_set_rtc(uint32_t date, uint32_t time);

/*----------------------
 | psk_host_advance
 | Description: Lets emulated time pass, running timer, RTC and button interrupts as they fall due.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: seconds -- time to pass
 | Returns: N/A
 ----------------------*/
void psk_host_advance(double seconds);

/*----------------------
 | psk_host_set_frame
 | Description: The function psk_frame calls.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: frame -- callback, or null
 | Returns: N/A
 ----------------------*/
void psk_host_set_frame(void (*frame)(void));

/*----------------------
 | psk_host_vram
 | Description: The 128-byte LCD frame buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: pointer to it
 ----------------------*/
const uint8_t *psk_host_vram(void);

/*----------------------
 | psk_host_ram
 | Description: The 2 KB work RAM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: pointer to it
 ----------------------*/
const uint8_t *psk_host_ram(void);

/*----------------------
 | psk_host_card_crc
 | Description: CRC-32 of the card image, flash writes included.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: the CRC
 ----------------------*/
uint32_t psk_host_card_crc(void);

/*----------------------
 | psk_host_run
 | Description: Runs decompiled code from c->r[15] until it continues at ret with the stack pointer at sp.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file; ret -- stop address (bit 0 = Thumb); sp -- stack pointer at the stop
 | Returns: N/A
 ----------------------*/
void psk_host_run(PskCpu *c, uint32_t ret, uint32_t sp);
void psk_host_load_entry(const uint8_t *wram, const uint32_t *w, uint32_t n);
uint8_t *psk_host_card(void);

#endif
