/*----------------------
 | misc.c
 | Description: Ring Ring Land's small helpers: the buzzer on/off and sound requests, the quit power-down, standby, the dock screen and interrupt, saving the state block to the pet save, digit placement, mirrored directions and the app's byte and nibble tables.
 | Author: suinevere
 | Dependencies: game.h, rrl_rom.h
 | Globals: sound_req, docked
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "rrl_rom.h"

/*----------------------
 | SOUND_TONE
 | Description: The buzzer's pitch step while a sound plays, standing in for the original's DAC samples.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SOUND_TONE 96u

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
 | Description: Starts the buzzer at SOUND_TONE, where the original enabled the DAC and the speaker.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_on(void) {
    vmu_buzz(SOUND_TONE);
}

/*----------------------
 | game_sound_off
 | Description: Silences the buzzer, where the original disabled the speaker and the DAC.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_off(void) {
    vmu_buzz(0);
}

/*----------------------
 | game_save
 | Description: Writes the 128-byte state block to the pet save's SAVE_SECTOR, retrying until it succeeds, with the sound end masked meanwhile.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: the block 0x4C4-0x543
 | Params: N/A
 | Returns: the interrupt mask in force before the save
 ----------------------*/
void game_save(void) {
    uint16_t mask = vmu_irq_mask();
    vmu_irq_disable(VMU_LINE_SOUND);
    while (vmu_save_write(SAVE_SECTOR, STATE_BLOCK) != 0) {
    }
    vmu_irq_enable(mask);
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
    return rom8(ROM_BYTES_9EFC + index) == value;
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
    uint32_t w = rom32(ROM_NIBBLES_9DF0 + ((index & 0xF8u) >> 1));
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
 | Description: On quit, stops the tick, the sound end and the buzzer, drops the callback and leaves only the RTC line on, idling until the buttons fall. No script reaches it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: game_sound_off
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_power_down(void) {
    vmu_tick_mode(VMU_TICK_OFF);
    vmu_sound_stop();
    game_sound_off();
    vmu_irq_disable(0x3FFFu);
    vmu_set_handler(0);
    vmu_irq_enable(VMU_LINE_RTC);
    while ((vmu_irq_levels() & VMU_LINE_BUTTONS) != 0u) {
        vmu_idle();
    }
}

/*----------------------
 | game_standby
 | Description: Standby after inactivity: stops the sound, the tick, the sound end and the buzzer, switches the LCD off and stops until a wake, then restarts the frame tick with a full period. The runner cannot sample a stopped clock: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: game_sound_stop, game_sound_off
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_standby(void) {
    game_sound_stop();
    vmu_tick_mode(VMU_TICK_OFF);
    vmu_sound_stop();
    game_sound_off();
    vmu_lcd(0);
    vmu_irq_disable(VMU_LINE_RTC);
    vmu_power(VMU_POWER_STANDBY);
    vmu_irq_enable(VMU_LINE_RTC);
    vmu_tick_mode(VMU_TICK_FRAME);
}

/*----------------------
 | game_dock_screen
 | Description: While docked, or as the dock line rises (then with the sound end, its interrupt and the buzzer stopped), shows a blank screen for 90 standby ticks (the VMU has no kernel dock picture) and leaves for the BIOS. No script docks: unverified.
 | Author: suinevere
 | Dependencies: game_timer_irq, game_sound_off, game_fb_clear, game_sprite_draw, game_fb_present, game_ticks_start, game_wait_ticks, game_dock_quit
 | Globals: docked
 | Params: N/A
 | Returns: what game_dock_quit returns, 0 when not docked
 ----------------------*/
void game_dock_screen(void) {
    if (ram_docked() == 0u) {
        if ((vmu_irq_levels() & VMU_LINE_DOCK) == 0u) {
            return;
        }
        vmu_sound_stop();
        game_timer_irq(2u, 0);
        game_sound_off();
    }
    game_fb_clear();
    game_fb_present();
    game_ticks_start(VMU_TICK_STANDBY);
    game_wait_ticks(1u, 0);
    game_wait_ticks(1u, 0x5Au);
    game_dock_quit();
}

/*----------------------
 | game_dock_irq
 | Description: The line-10 (dock) interrupt handler: marks the app docked and stops the sound end, its interrupt and the buzzer; clock speed 2 comes with the dock screen's standby ticks. No script docks: unverified.
 | Author: suinevere
 | Dependencies: vmu_hw.h, game_timer_irq, game_sound_off
 | Globals: docked
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_dock_irq(void) {
    set_docked(1);
    vmu_sound_stop();
    game_timer_irq(2u, 0);
    game_sound_off();
}
