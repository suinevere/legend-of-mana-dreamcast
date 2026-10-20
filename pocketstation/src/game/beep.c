/*----------------------
 | beep.c
 | Description: Ring Ring Land's quit beep: a 63-step square wave on the DAC at a raised CPU clock, and the guard that plays it with interrupts masked.
 | Author: suinevere
 | Dependencies: game.h, psk_cpu.h, psk_swi.h
 | Globals: docked, com_flags, snd_mode
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "psk_cpu.h"
#include "psk_swi.h"

/*----------------------
 | CLK_MODE
 | Description: The clock control register; its low four bits are the CPU speed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CLK_MODE 0x0B000000u

/*----------------------
 | DAC_DATA
 | Description: The DAC sample register.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DAC_DATA 0x0D800014u

/*----------------------
 | game_quit_beep
 | Description: Switches to clock speed 3, writes a 63-step square wave whose level is shifted down by shift, then restores the clock (or clears the clock register if it was 0). Unverified: only the quit dialog reaches it.
 | Author: suinevere
 | Dependencies: psk_swi.h
 | Globals: N/A
 | Params: shift -- extra right shift of the wave's level
 | Returns: N/A
 ----------------------*/
void game_quit_beep(uint8_t shift) {
    uint32_t speed = psk_r32(CLK_MODE) & 0xFu;
    psk_swi(0x04u, 3u, 0xFu, shift);
    for (uint8_t step = 1; step < 64u; step++) {
        uint32_t level = (0x200u - (step & 1u)) << 22;
        psk_w32(DAC_DATA, psk_asr(level, (shift + 16u) & 0xFFu));
    }
    if (speed != 0) {
        psk_swi(0x04u, speed, 0x40u, shift);
    } else {
        psk_w32(CLK_MODE, 0);
    }
}

/*----------------------
 | game_quit_sound
 | Description: Unless docked, the kernel's flag bit 0 is set or snd_mode is 2, plays the quit beep with every other interrupt masked. The saved interrupt mask is 32-bit. Unverified: only the quit dialog reaches it.
 | Author: suinevere
 | Dependencies: rrl_ram.h, psk_cpu.h
 | Globals: docked, com_flags, snd_mode
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_quit_sound(void) {
    uint8_t mode;
    uint32_t mask;
    if (ram_docked() != 0u || (psk_ldr(ram_com_flags()) & 1u) != 0u) {
        return;
    }
    mode = ram_snd_mode();
    if (mode == 2u) {
        return;
    }
    mask = psk_ldr(INT_MASK_SET);
    psk_w32(INT_MASK_CLR, 0x35FFu);
    game_sound_on();
    game_quit_beep(mode != 0u ? 2u : 0u);
    game_sound_off();
    psk_w32(INT_MASK_SET, mask);
}
