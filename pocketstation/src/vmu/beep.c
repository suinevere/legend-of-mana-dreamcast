/*----------------------
 | beep.c
 | Description: Ring Ring Land's quit beep: a 63-step square wave on the DAC at a raised CPU clock, and the guard that plays it with interrupts masked.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: docked, snd_mode
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | game_quit_beep
 | Description: Switches to clock speed 3, writes a 63-step square wave whose level is shifted down by shift, then restores the clock (or clears the clock register if it was 0); the DAC log keeps each level's low 16 bits. Unverified: only the quit dialog reaches it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: shift -- extra right shift of the wave's level
 | Returns: N/A
 ----------------------*/
void game_quit_beep(uint8_t shift) {
    uint8_t speed = vmu_clock();
    vmu_set_clock(3u);
    for (uint8_t step = 1; step < 64u; step++) {
        uint32_t level = (0x200u - (step & 1u)) << 22;
        vmu_io_write(VMU_IO_DAC_DATA, (uint16_t)game_asr(level, (shift + 16u) & 0xFFu));
    }
    vmu_set_clock(speed);
}

/*----------------------
 | game_quit_sound
 | Description: Unless docked, the kernel's flag bit 0 is set or snd_mode is 2, plays the quit beep with every other interrupt masked. The saved interrupt mask is 32-bit. Unverified: only the quit dialog reaches it.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: docked, snd_mode
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_quit_sound(void) {
    uint8_t mode;
    uint16_t mask;
    if (ram_docked() != 0u || (vmu_sys_flags() & VMU_SYS_MUTE) != 0u) {
        return;
    }
    mode = ram_snd_mode();
    if (mode == 2u) {
        return;
    }
    mask = vmu_irq_mask();
    vmu_irq_disable(0x35FFu);
    game_sound_on();
    game_quit_beep(mode != 0u ? 2u : 0u);
    game_sound_off();
    vmu_irq_enable(mask);
}
