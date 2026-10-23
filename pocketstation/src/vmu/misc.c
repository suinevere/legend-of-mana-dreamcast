/*----------------------
 | misc.c
 | Description: Ring Ring Land's small helpers: sound on/off and requests, the quit power-down, standby, the dock screen and interrupt, saving the state block to flash, digit placement, mirrored directions and the app's byte and nibble tables.
 | Author: suinevere
 | Dependencies: game.h, rrl_rom.h
 | Globals: sound_req, docked
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "rrl_rom.h"

/*----------------------
 | park_sound
 | Description: Parks the sound port and the DAC as every way out of the game does.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void park_sound(void) {
    vmu_io_write(VMU_IO_PORT, 2u);
    vmu_io_write(VMU_IO_PORT_SET, 0x62u);
    vmu_io_write(VMU_IO_DAC_CTRL, 0);
}

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
    vmu_io_write(VMU_IO_DAC_CTRL, 1u);
    vmu_io_write(VMU_IO_PORT_CLR, 0x20u);
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
    vmu_io_write(VMU_IO_PORT_SET, 0x20u);
    vmu_io_write(VMU_IO_DAC_CTRL, 0);
}

/*----------------------
 | game_save
 | Description: Writes the 128-byte state block to flash, retrying until the kernel succeeds, with interrupt lines 6 and 13 masked meanwhile.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: the block 0x4C4-0x543
 | Params: N/A
 | Returns: the interrupt mask in force before the save
 ----------------------*/
void game_save(void) {
    uint16_t mask = vmu_irq_mask();
    vmu_irq_disable(VMU_LINE_COM | VMU_LINE_TIMER2);
    while (vmu_file_write(SAVE_SECTOR, STATE_BLOCK) != 0) {
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
 | Description: On quit, stops timers 1 and 2, parks the sound and infrared hardware, drops the kernel callbacks and leaves only lines 9 and 11 on, idling until lines 0-5 fall. No script reaches it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_power_down(void) {
    game_timer_run(1u, 0);
    game_timer_run(2u, 0);
    park_sound();
    vmu_io_write(VMU_IO_IRDA, 2u);
    vmu_irq_disable(0x3FFFu);
    vmu_set_handlers(0, 0);
    vmu_irq_enable(0xA00u);
    while ((vmu_irq_levels() & 0x3Fu) != 0u) {
        vmu_idle();
    }
}

/*----------------------
 | game_standby
 | Description: Standby after inactivity: silences the sound, stops timers 1 and 2, parks the sound and infrared hardware, switches the LCD off and stops the clock until a button wakes line 9, then restarts timer 1. The runner cannot sample a stopped clock: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: game_sound_stop, game_timer_run
 | Globals: N/A
 | Params: N/A
 | Returns: the mode timer 1 restarts with
 ----------------------*/
void game_standby(void) {
    game_sound_stop();
    game_timer_run(1u, 0);
    game_timer_run(2u, 0);
    park_sound();
    vmu_io_write(VMU_IO_PORT_20, 4u);
    vmu_io_write(VMU_IO_IRDA, 2u);
    vmu_io_write(VMU_IO_LCD_MODE, (uint16_t)(vmu_io_read(VMU_IO_LCD_MODE) & ~0x48u));
    vmu_irq_disable(VMU_LINE_RTC);
    vmu_power(VMU_POWER_STANDBY);
    vmu_irq_enable(VMU_LINE_RTC);
    game_timer_run(1u, 1u);
}

/*----------------------
 | game_dock_screen
 | Description: While docked, or as the dock line rises (then with Timer 2, its interrupt and the sound hardware stopped and the clock at speed 2), shows a blank screen for 90 ticks of Timer 1 (the VMU has no kernel dock picture to draw) and leaves for the BIOS. No script docks: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: game_timer_run, game_timer_irq, game_fb_clear, game_sprite_draw, game_fb_present, game_ticks_start, game_wait_ticks, game_dock_quit
 | Globals: docked
 | Params: N/A
 | Returns: what game_dock_quit returns, 0 when not docked
 ----------------------*/
void game_dock_screen(void) {
    if (ram_docked() == 0u) {
        if ((vmu_irq_levels() & VMU_LINE_DOCK) == 0u) {
            return;
        }
        game_timer_run(2u, 0);
        game_timer_irq(2u, 0);
        park_sound();
        vmu_set_clock(2u);
    }
    game_fb_clear();
    game_fb_present();
    game_ticks_start(1u, 0x7A1u, 0);
    game_wait_ticks(1u, 0);
    game_wait_ticks(1u, 0x5Au);
    game_dock_quit();
}

/*----------------------
 | game_dock_irq
 | Description: The line-10 (dock) interrupt handler: marks the app docked, stops Timer 2 and its interrupt, parks the sound and infrared hardware and sets clock speed 2, which the original asked of the kernel's system call 4. No script docks: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: vmu_hw.h, game_timer_run, game_timer_irq
 | Globals: docked
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_dock_irq(void) {
    set_docked(1);
    game_timer_run(2u, 0);
    game_timer_irq(2u, 0);
    park_sound();
    vmu_io_write(VMU_IO_IRDA, 2u);
    vmu_set_clock(2u);
}
