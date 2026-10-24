/*----------------------
 | beep.c
 | Description: Ring Ring Land's quit beep: one buzzer tone, and the guard that plays it with interrupts masked.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: docked, snd_mode
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | BEEP_TONE
 | Description: The quit beep's buzzer pitch step at full level; the original wrote a 63-step square wave to the DAC at clock speed 3.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BEEP_TONE 192u

/*----------------------
 | game_quit_beep
 | Description: Plays one buzzer tone, BEEP_TONE shifted down by shift; the buzzer has no volume, so the original's quieter level becomes a lower step. Unverified: only the quit dialog reaches it.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: N/A
 | Params: shift -- the original's extra right shift of the wave's level
 | Returns: N/A
 ----------------------*/
void game_quit_beep(uint8_t shift) {
    vmu_buzz((uint8_t)(BEEP_TONE >> shift));
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
