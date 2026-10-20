/*----------------------
 | misc.c
 | Description: Ring Ring Land's small helpers: sound on/off and requests, the quit power-down, saving the state block to flash, digit placement, mirrored directions and the app's byte and nibble tables.
 | Author: suinevere
 | Dependencies: game.h, psk_cpu.h, psk_swi.h, rrl_rom.h
 | Globals: sound_req
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "rrl_rom.h"
#include "psk_cpu.h"
#include "psk_swi.h"

/*----------------------
 | SOUND_BASE
 | Description: The sound hardware's register block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SOUND_BASE 0x0D800000u

/*----------------------
 | ROM_BYTES_9EFC
 | Description: A byte table in the app read by index.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_BYTES_9EFC 0x02009EFCu

/*----------------------
 | SAVE_SECTOR
 | Description: The flash sector the state block is saved to.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SAVE_SECTOR 6u

/*----------------------
 | IRDA_MODE
 | Description: The infrared port's mode register.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define IRDA_MODE 0x0C800000u

/*----------------------
 | game_sound_request
 | Description: Stores a request for the sound driver.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: sound_req
 | Params: request -- request byte
 | Returns: N/A
 ----------------------*/
void game_sound_request(uint8_t request) {
    set_sound_req(request);
}

/*----------------------
 | game_sound_on
 | Description: Enables the sound output.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_on(void) {
    psk_w32(SOUND_BASE + 0x10u, 1u);
    psk_w32(SOUND_BASE + 8u, 0x20u);
}

/*----------------------
 | game_sound_off
 | Description: Disables the sound output.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_off(void) {
    psk_w32(SOUND_BASE + 4u, 0x20u);
    psk_w32(SOUND_BASE + 0x10u, 0);
}

/*----------------------
 | game_save
 | Description: Writes the 128-byte state block to flash, retrying until the kernel succeeds, with interrupt lines 6 and 13 masked meanwhile.
 | Author: suinevere
 | Dependencies: psk_swi.h
 | Globals: the block 0x4C4-0x543
 | Params: N/A
 | Returns: the interrupt mask in force before the save
 ----------------------*/
uint32_t game_save(void) {
    uint32_t mask = psk_ldr(INT_MASK_SET);
    psk_w32(INT_MASK_CLR, 0x2040u);
    while (psk_swi(0x03u, SAVE_SECTOR, STATE_BLOCK, 0x0A000000u) != 0) {
    }
    psk_w32(INT_MASK_SET, mask);
    return mask;
}

/*----------------------
 | game_byte_is
 | Description: Whether entry index of the byte table at 0x02009EFC equals value.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: index -- entry; value -- byte to compare
 | Returns: 1 when equal, else 0
 ----------------------*/
uint8_t game_byte_is(uint8_t index, uint32_t value) {
    return psk_r8(ROM_BYTES_9EFC + index) == value;
}

/*----------------------
 | game_nibble
 | Description: Entry index of the nibble table at 0x02009DF0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: index -- entry
 | Returns: the nibble
 ----------------------*/
uint8_t game_nibble(uint8_t index) {
    uint32_t w = psk_ldr(ROM_NIBBLES_9DF0 + ((index & 0xF8u) >> 1));
    return (uint8_t)((w >> ((index & 7u) * 4u)) & 0xFu);
}

/*----------------------
 | game_digits_x
 | Description: Moves a right-aligned number's x right by 4 pixels per digit it has fewer than five.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: x -- x for a five-digit number; n -- the number
 | Returns: the x to draw at
 ----------------------*/
uint32_t game_digits_x(uint32_t x, uint32_t n) {
    if (n >= 10000u) return x;
    if (n >= 1000u) return x + 4u;
    if (n >= 100u) return x + 8u;
    if (n >= 10u) return x + 12u;
    return x + 16u;
}

/*----------------------
 | game_mirror_direction
 | Description: Swaps the lowest set of bits 0-3 end for end (1 to 8, 2 to 4, 4 to 2, 8 to 1); returns bits unchanged when none is set.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: bits -- direction bits
 | Returns: the mirrored bit
 ----------------------*/
uint32_t game_mirror_direction(uint32_t bits) {
    if (bits & 1u) return 8u;
    if (bits & 2u) return 4u;
    if (bits & 4u) return 2u;
    if (bits & 8u) return 1u;
    return bits;
}

/*----------------------
 | game_power_down
 | Description: On quit, stops timers 1 and 2, parks the sound and infrared hardware, drops the kernel callbacks and leaves only lines 9 and 11 on, idling until lines 0-5 fall. No script reaches it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: psk_swi.h, psk_cpu.h
 | Globals: N/A
 | Params: N/A
 | Returns: the 32-bit interrupt input word last read, its low six bits clear
 ----------------------*/
uint32_t game_power_down(void) {
    uint32_t input;
    game_timer_run(1u, 0);
    game_timer_run(2u, 0);
    psk_w32(SOUND_BASE, 2u);
    psk_w32(SOUND_BASE + 4u, 0x62u);
    psk_w32(SOUND_BASE + 0x10u, 0);
    psk_w32(IRDA_MODE, 2u);
    psk_w32(INT_MASK_CLR, 0x3FFFu);
    psk_swi(0x01u, 2u, 0, 0);
    psk_swi(0x01u, 1u, 0, 0);
    psk_w32(INT_MASK_SET, 0xA00u);
    for (;;) {
        input = psk_ldr(INT_INPUT);
        if ((input & 0x3Fu) == 0u) {
            return input;
        }
        psk_idle();
    }
}
