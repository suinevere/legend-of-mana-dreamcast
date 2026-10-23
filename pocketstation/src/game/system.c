/*----------------------
 | system.c
 | Description: Ring Ring Land's hardware helpers: the LCD mode, the kernel's interrupt callbacks with their dispatch through the handler slots, and the halfword table at 0x02008094, the dock check at start-up and quitting to the kernel from the dialog or the dock.
 | Author: suinevere
 | Dependencies: game.h, psk_cpu.h, rrl_rom.h, psk_swi.h
 | Globals: irq_handler, fiq_driver, docked, kernel_calls
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "psk_cpu.h"
#include "rrl_rom.h"
#include "psk_swi.h"

/*----------------------
 | IRQ_HANDLER_RETURN
 | Description: The dispatcher's return address in the original, given to each handler as its link register.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define IRQ_HANDLER_RETURN 0x02002BACu

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
    uint32_t v = psk_r32(LCD_MODE);
    v = (v & ~0x7Fu) | 0x48u | low | ((uint32_t)high << 4);
    psk_w32(LCD_MODE, v);
    psk_w32(LCD_REG20, 0);
}

/*----------------------
 | game_set_irq_handler
 | Description: Stores the handler the interrupt dispatcher calls for one interrupt line.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: irq_handler
 | Params: line -- interrupt line; handler -- routine address (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void game_set_irq_handler(uint8_t line, uint32_t handler) {
    set_irq_handler(line, handler);
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
    return (uint16_t)(psk_r16(ROM_WORDS_8094 + index * 2u) & 0x7FFFu);
}

/*----------------------
 | game_irq_dispatch
 | Description: Calls each pending line's slot handler from line 12 down, bit in r0, through psk_call; pending stays local since the handlers clear at most their own line from r9; handler slots hold 32-bit addresses. Skipping an empty slot is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h, psk_hw.h, psk_cpu.h
 | Globals: irq_handler
 | Params: c -- register file the handlers run in; pending -- pending interrupt lines
 | Returns: N/A
 ----------------------*/
void game_irq_dispatch(PskCpu *c, uint16_t pending) {
    uint16_t bit = 0x1000u;
    uint8_t line = 12u;
    while (bit != 0u) {
        if (pending & bit) {
            uint32_t handler = ram_irq_handler(line);
            if (handler != 0u) {
                c->r[0] = bit;
                psk_call(c, psk_bx(handler), IRQ_HANDLER_RETURN);
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
 | Description: Kernel IRQ callback: dispatches the latched, enabled lines other than 6 and 13, then acknowledges them except lines 9 and 11; the interrupt words are the controller's 32-bit registers.
 | Author: suinevere
 | Dependencies: psk_hw.h
 | Globals: irq_handler
 | Params: c -- register file the handlers run in
 | Returns: N/A
 ----------------------*/
void game_irq_callback(PskCpu *c) {
    uint32_t pending = psk_ldr(INT_LATCH) & psk_ldr(INT_MASK_SET) & ~0x2040u;
    game_irq_dispatch(c, (uint16_t)pending);
    psk_w32(INT_ACK, pending & ~0xA00u);
}

/*----------------------
 | game_fiq_callback
 | Description: Kernel FIQ callback: with Timer 2 pending it acknowledges it and hands over to fiq_driver, a 32-bit address, or runs the IRQ callback when no driver is set. The no-Timer-2 return and the IRQ fall-through are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h, psk_hw.h
 | Globals: fiq_driver, irq_handler
 | Params: c -- register file the handlers run in
 | Returns: the driver to continue at, or 0 when the callback is done
 ----------------------*/
uint32_t game_fiq_callback(PskCpu *c) {
    if ((psk_ldr(INT_LATCH) & psk_ldr(INT_MASK_SET) & 0x2000u) == 0u) {
        return 0u;
    }
    psk_w32(INT_ACK, 0x2000u);
    if (ram_fiq_driver() != 0u) {
        return ram_fiq_driver();
    }
    game_irq_callback(c);
    return 0u;
}

/*----------------------
 | game_quit
 | Description: Leaves Ring Ring Land: drops PlayStation communication, powers down, asks the kernel to run its own block (the app's block with bits 4-5 set) and terminates, then powers down again. No script quits: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: psk_swi.h, game_power_down
 | Globals: N/A
 | Params: N/A
 | Returns: the block word given to the kernel
 ----------------------*/
uint32_t game_quit(void) {
    uint32_t block;
    psk_swi(0x11u, 0, 0, 0);
    game_power_down();
    block = psk_swi(0x16u, 0, 0, 0) | 0x30u;
    psk_swi(0x08u, 1u, 0, block);
    psk_swi(0x09u, 0, 0, block);
    game_power_down();
    return block;
}

/*----------------------
 | game_dock_quit
 | Description: Leaves Ring Ring Land for the kernel when docked: drops PlayStation communication, powers down, asks the kernel to run block 0 with flag 0x10 and terminates, then goes to standby. No script docks: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: psk_swi.h, game_power_down, game_standby
 | Globals: N/A
 | Params: N/A
 | Returns: what game_standby returns
 ----------------------*/
uint32_t game_dock_quit(void) {
    psk_swi(0x11u, 0, 0, 0);
    game_power_down();
    psk_swi(0x08u, 1u, 0, 0x10u);
    psk_swi(0x09u, 0, 0, 0x10u);
    return game_standby();
}

/*----------------------
 | DOCK_IRQ_ENTRY
 | Description: Address of the line-10 (dock) interrupt handler that game_dock_check installs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DOCK_IRQ_ENTRY 0x020005A0u

/*----------------------
 | game_dock_check
 | Description: At start-up, quits to the kernel if already docked; otherwise clears docked, keeps the first word of the kernel's system call table and installs the dock interrupt on line 10.
 | Author: suinevere
 | Dependencies: psk_swi.h, game_dock_quit, game_set_irq_handler
 | Globals: docked, kernel_calls
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_dock_check(void) {
    if (psk_ldr(INT_INPUT) & 0x400u) {
        game_dock_quit();
        return;
    }
    set_docked(0);
    set_kernel_calls(psk_ldr(psk_swi(0x14u, 0, 0, 0)));
    game_set_irq_handler(10u, DOCK_IRQ_ENTRY);
}
