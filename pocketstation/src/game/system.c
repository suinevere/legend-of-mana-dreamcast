/*----------------------
 | system.c
 | Description: Ring Ring Land's hardware helpers: the LCD mode, the kernel's interrupt callbacks with their dispatch through the handler slots, and the halfword table at 0x02008094.
 | Author: suinevere
 | Dependencies: game.h, psk_cpu.h, rrl_rom.h
 | Globals: irq_handler, fiq_driver
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "psk_cpu.h"
#include "rrl_rom.h"

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
