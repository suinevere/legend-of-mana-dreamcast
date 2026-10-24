/*----------------------
 | system.c
 | Description: Ring Ring Land's hardware helpers: the LCD switch, the interrupt callback with its dispatch through the handler slots, the halfword table at 0x02008094, the dock check at start-up and quitting from the dialog or the dock.
 | Author: suinevere
 | Dependencies: game.h, rrl_rom.h
 | Globals: irq_handler, fiq_driver, docked
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "rrl_rom.h"

/*----------------------
 | game_lcd_mode
 | Description: Switches the LCD on, where the original set its mode's low seven bits to 0x68 and cleared LCD register 0x20.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_lcd_mode(void) {
    vmu_lcd(1u);
}

/*----------------------
 | game_set_irq_handler
 | Description: Stores the handler id the interrupt dispatcher calls for one interrupt line; line 13's slot is fiq_driver, which the original's slot table ran on into.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: irq_handler
 | Params: line -- interrupt line; handler -- HANDLER_*
 | Returns: N/A
 ----------------------*/
void game_set_irq_handler(uint8_t line, uint8_t handler) {
    if (line == 13u) {
        set_fiq_driver(handler);
    } else {
        set_irq_handler(line, handler);
    }
}

/*----------------------
 | game_call_handler
 | Description: Runs the routine a handler id names, with the interrupt line it serves where the original passed it in r0.
 | Author: suinevere
 | Dependencies: game_buttons_irq, game_timer_tick, game_sound_fiq, game_dock_irq
 | Globals: N/A
 | Params: id -- HANDLER_*; line -- interrupt line bit
 | Returns: N/A
 ----------------------*/
void game_call_handler(uint8_t id, uint16_t line) {
    if (id == HANDLER_BUTTONS) {
        game_buttons_irq(line);
    } else if (id == HANDLER_TIMER_TICK) {
        game_timer_tick(line);
    } else if (id == HANDLER_SOUND) {
        game_sound_fiq();
    } else if (id == HANDLER_DOCK) {
        game_dock_irq();
    }
}

/*----------------------
 | game_rom_word_8094
 | Description: Reads entry index of the halfword table at 0x02008094 without its top bit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: index -- entry
 | Returns: the entry & 0x7FFF
 ----------------------*/
uint16_t game_rom_word_8094(uint8_t index) {
    return (uint16_t)(rom16(ROM_WORDS_8094 + index * 2u) & 0x7FFFu);
}

/*----------------------
 | slot_handler
 | Description: The handler id in a line's slot: irq_handler for lines 0-12, fiq_driver for the sound end's line 13.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: irq_handler, fiq_driver
 | Params: line -- interrupt line 0-13
 | Returns: HANDLER_*
 ----------------------*/
static uint8_t slot_handler(uint8_t line) {
    return line == 13u ? ram_fiq_driver() : ram_irq_handler(line);
}

/*----------------------
 | game_irq_dispatch
 | Description: Calls each pending line's slot handler from line 13 down with its line bit, so a sound end goes before a tick as the original's FIQ did. Slots hold handler ids; skipping an empty slot is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h, vmu_hw.h
 | Globals: irq_handler, fiq_driver
 | Params: pending -- pending interrupt lines
 | Returns: N/A
 ----------------------*/
void game_irq_dispatch(uint16_t pending) {
    uint16_t bit = 0x2000u;
    uint8_t line = 13u;
    while (bit != 0u) {
        if (pending & bit) {
            uint8_t handler = slot_handler(line);
            if (handler != HANDLER_NONE) {
                game_call_handler(handler, bit);
            }
            pending &= (uint16_t)~bit;
            if (pending == 0u) {
                return;
            }
        }
        bit >>= 1;
        line--;
    }
}

/*----------------------
 | game_irq_callback
 | Description: The one interrupt callback, the original's IRQ and FIQ callbacks folded: dispatches the latched, enabled lines, the sound end through slot 13 first, then acknowledges them except the RTC.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: irq_handler, fiq_driver
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_irq_callback(void) {
    uint16_t pending = (uint16_t)(vmu_irq_pending() & vmu_irq_mask());
    game_irq_dispatch(pending);
    vmu_irq_ack((uint16_t)(pending & ~VMU_LINE_RTC));
}

/*----------------------
 | game_quit
 | Description: Leaves Ring Ring Land: powers down and leaves for the BIOS, where the original dropped PlayStation communication and asked the kernel to run its own block. No script quits: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: game_power_down
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_quit(void) {
    game_power_down();
    vmu_power(VMU_POWER_EXIT);
}

/*----------------------
 | game_dock_quit
 | Description: Leaves Ring Ring Land when docked: powers down and leaves for the BIOS, where the original asked the kernel to run block 0 with flag 0x10 and then stood by. No script docks: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: game_power_down, game_standby
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_dock_quit(void) {
    game_power_down();
    vmu_power(VMU_POWER_EXIT_DOCKED);
}

/*----------------------
 | DOCK_IRQ_ENTRY
 | Description: Handler id of the line-10 (dock) interrupt handler that game_dock_check installs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DOCK_IRQ_ENTRY HANDLER_DOCK

/*----------------------
 | game_dock_check
 | Description: At start-up, leaves if already docked; otherwise clears docked and installs the dock interrupt on line 10.
 | Author: suinevere
 | Dependencies: game_dock_quit, game_set_irq_handler
 | Globals: docked
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_dock_check(void) {
    if (vmu_irq_levels() & VMU_LINE_DOCK) {
        game_dock_quit();
        return;
    }
    set_docked(0);
    game_set_irq_handler(10u, DOCK_IRQ_ENTRY);
}
