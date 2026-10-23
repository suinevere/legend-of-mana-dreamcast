/*----------------------
 | system.c
 | Description: Ring Ring Land's hardware helpers: the LCD mode, the kernel's interrupt callbacks with their dispatch through the handler slots, and the halfword table at 0x02008094, the dock check at start-up and quitting to the kernel from the dialog or the dock.
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
 | Description: Sets the LCD mode's low seven bits to 0x48 | low | high << 4 and clears LCD register 0x20.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: low -- bits 0-3; high -- bits 4-6
 | Returns: N/A
 ----------------------*/
void game_lcd_mode(uint8_t low, uint8_t high) {
    uint16_t v = vmu_io_read(VMU_IO_LCD_MODE);
    v = (uint16_t)((v & ~0x7Fu) | 0x48u | low | ((uint16_t)high << 4));
    vmu_io_write(VMU_IO_LCD_MODE, v);
    vmu_io_write(VMU_IO_LCD_REG20, 0);
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
 | game_irq_dispatch
 | Description: Calls each pending line's slot handler from line 12 down with its line bit; pending stays local since the handlers clear at most their own line; slots hold handler ids where the original held addresses. Skipping an empty slot is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h, vmu_hw.h
 | Globals: irq_handler
 | Params: pending -- pending interrupt lines
 | Returns: N/A
 ----------------------*/
void game_irq_dispatch(uint16_t pending) {
    uint16_t bit = 0x1000u;
    uint8_t line = 12u;
    while (bit != 0u) {
        if (pending & bit) {
            uint8_t handler = ram_irq_handler(line);
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
 | Description: IRQ callback: dispatches the latched, enabled lines other than 6 and 13, then acknowledges them except lines 9 and 11.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: irq_handler
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_irq_callback(void) {
    uint16_t pending = (uint16_t)(vmu_irq_pending() & vmu_irq_mask() & ~(VMU_LINE_COM | VMU_LINE_TIMER2));
    game_irq_dispatch(pending);
    vmu_irq_ack((uint16_t)(pending & ~(VMU_LINE_RTC | VMU_LINE_LINK)));
}

/*----------------------
 | game_fiq_callback
 | Description: FIQ callback: with Timer 2 pending it acknowledges it and runs fiq_driver's handler, or the IRQ callback when no driver is set. The no-Timer-2 return and the IRQ fall-through are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h, vmu_hw.h
 | Globals: fiq_driver, irq_handler
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_fiq_callback(void) {
    if ((vmu_irq_pending() & vmu_irq_mask() & VMU_LINE_TIMER2) == 0u) {
        return;
    }
    vmu_irq_ack(VMU_LINE_TIMER2);
    if (ram_fiq_driver() != HANDLER_NONE) {
        game_call_handler(ram_fiq_driver(), VMU_LINE_TIMER2);
        return;
    }
    game_irq_callback();
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
