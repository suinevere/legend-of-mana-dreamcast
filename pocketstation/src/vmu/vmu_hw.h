/*----------------------
 | vmu_hw.h
 | Description: The only way the VMU-shaped Ring Ring Land reaches RAM, the screen, buttons, interrupts, timers, the clock, the system and its file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: vmu_ram
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef VMU_HW_H
#define VMU_HW_H

#include <stdint.h>

/*----------------------
 | VMU_FB_BYTES
 | Description: Size of the 48x32 frame buffer: 6 bytes a row, bit 7 leftmost.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_FB_BYTES 192

/*----------------------
 | VMU_BTN_FIRE / VMU_BTN_RIGHT / VMU_BTN_LEFT / VMU_BTN_DOWN / VMU_BTN_UP
 | Description: Button bits as vmu_buttons returns them; each is also its interrupt line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_BTN_FIRE 0x01
#define VMU_BTN_RIGHT 0x02
#define VMU_BTN_LEFT 0x04
#define VMU_BTN_DOWN 0x08
#define VMU_BTN_UP 0x10

/*----------------------
 | VMU_LINE_BUTTONS / VMU_LINE_COM / VMU_LINE_TIMER0 / VMU_LINE_TIMER1 / VMU_LINE_RTC / VMU_LINE_DOCK / VMU_LINE_LINK / VMU_LINE_TIMER2
 | Description: Interrupt lines, numbered as the PocketStation's; COM and Timer 2 go to the FIQ handler, the rest to the IRQ handler.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_LINE_BUTTONS 0x001Fu
#define VMU_LINE_COM 0x0040u
#define VMU_LINE_TIMER0 0x0080u
#define VMU_LINE_TIMER1 0x0100u
#define VMU_LINE_RTC 0x0200u
#define VMU_LINE_DOCK 0x0400u
#define VMU_LINE_LINK 0x0800u
#define VMU_LINE_TIMER2 0x2000u

/*----------------------
 | VMU_SYS_MUTE / VMU_SYS_QUIT / VMU_SYS_SND_MODE
 | Description: vmu_sys_flags bits: sound muted, the system asks the game to end, and the two snd_mode bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_SYS_MUTE 0x01
#define VMU_SYS_QUIT 0x02
#define VMU_SYS_SND_MODE 0x0C

/*----------------------
 | VMU_POWER_STANDBY / VMU_POWER_EXIT / VMU_POWER_EXIT_DOCKED
 | Description: vmu_power modes: stop until an enabled line, leave to the BIOS, leave to the BIOS because docked.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_POWER_STANDBY 1
#define VMU_POWER_EXIT 2
#define VMU_POWER_EXIT_DOCKED 3

/*----------------------
 | VMU_IO_PORT / VMU_IO_DAC_CTRL / VMU_IO_DAC_DATA / VMU_IO_IRDA / VMU_IO_LCD_MODE / VMU_IO_LCD_REG20 / VMU_IO_PORT_SET / VMU_IO_PORT_CLR / VMU_IO_PORT_20
 | Description: Peripheral registers for vmu_io_write and vmu_io_read: sound port control, DAC control and sample, infrared mode, LCD mode, LCD register 0x20, and the sound port's set-bits, clear-bits and 0x20 registers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_IO_PORT 0
#define VMU_IO_DAC_CTRL 1
#define VMU_IO_DAC_DATA 2
#define VMU_IO_IRDA 3
#define VMU_IO_LCD_MODE 4
#define VMU_IO_LCD_REG20 5
#define VMU_IO_PORT_SET 6
#define VMU_IO_PORT_CLR 7
#define VMU_IO_PORT_20 8

/*----------------------
 | vmu_ram
 | Description: The game's whole RAM, laid out by ram_map.txt's VMU column.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern uint8_t vmu_ram[0x400];

/*----------------------
 | vmu_r8
 | Description: Reads a byte of vmu_ram.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: vmu_ram
 | Params: off -- RAM offset
 | Returns: the byte
 ----------------------*/
uint8_t vmu_r8(uint16_t off);

/*----------------------
 | vmu_w8
 | Description: Writes a byte of vmu_ram.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: vmu_ram
 | Params: off -- RAM offset; v -- value
 | Returns: N/A
 ----------------------*/
void vmu_w8(uint16_t off, uint8_t v);

/*----------------------
 | vmu_buttons
 | Description: The buttons held now, as VMU_BTN_* bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: button bits
 ----------------------*/
uint8_t vmu_buttons(void);

/*----------------------
 | vmu_fb
 | Description: The frame buffer the game draws into.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: VMU_FB_BYTES bytes
 ----------------------*/
uint8_t *vmu_fb(void);

/*----------------------
 | vmu_present
 | Description: Marks the end of a frame; the frame buffer is shown as it stands.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void vmu_present(void);

/*----------------------
 | vmu_idle
 | Description: The game's only wait point: lets time run to the next timer expiry or RTC edge, then calls the handlers for the lines that are due.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void vmu_idle(void);

/*----------------------
 | vmu_irq_enable
 | Description: Unmasks interrupt lines; a line already latched becomes deliverable.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: lines -- VMU_LINE_* bits
 | Returns: N/A
 ----------------------*/
void vmu_irq_enable(uint16_t lines);

/*----------------------
 | vmu_irq_disable
 | Description: Masks interrupt lines; a later expiry or press still latches but is not delivered.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: lines -- VMU_LINE_* bits
 | Returns: N/A
 ----------------------*/
void vmu_irq_disable(uint16_t lines);

/*----------------------
 | vmu_irq_ack
 | Description: Clears latched lines, and their request levels except for buttons, COM, RTC and the link, which follow their signals.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: lines -- VMU_LINE_* bits
 | Returns: N/A
 ----------------------*/
void vmu_irq_ack(uint16_t lines);

/*----------------------
 | vmu_irq_pending
 | Description: Latched lines, masked or not.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: VMU_LINE_* bits
 ----------------------*/
uint16_t vmu_irq_pending(void);

/*----------------------
 | vmu_irq_levels
 | Description: Interrupt request levels: buttons held, the dock and the other signal lines.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: VMU_LINE_* bits
 ----------------------*/
uint16_t vmu_irq_levels(void);

/*----------------------
 | vmu_irq_mask
 | Description: The enabled lines.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: VMU_LINE_* bits
 ----------------------*/
uint16_t vmu_irq_mask(void);

/*----------------------
 | vmu_set_handlers
 | Description: Installs the IRQ and FIQ handlers vmu_idle calls; null drops one.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: irq -- IRQ handler; fiq -- FIQ handler
 | Returns: N/A
 ----------------------*/
void vmu_set_handlers(void (*irq)(void), void (*fiq)(void));

/*----------------------
 | vmu_set_timer
 | Description: Sets a timer's reload and divider code (0-3: 2, 32, 512, 2 clock cycles) and runs or stops it; the prescaler restarts, and the count loads when it starts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: which -- timer 0-2; reload -- count; prescale -- divider code; run -- nonzero to run
 | Returns: N/A
 ----------------------*/
void vmu_set_timer(uint8_t which, uint16_t reload, uint8_t prescale, uint8_t run);

/*----------------------
 | vmu_timer_run
 | Description: Runs or stops a timer with its reload and divider kept; the prescaler restarts either way.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: which -- timer 0-2; run -- nonzero to run
 | Returns: N/A
 ----------------------*/
void vmu_timer_run(uint8_t which, uint8_t run);

/*----------------------
 | vmu_set_clock
 | Description: Sets the CPU clock speed (0-8, 32 kHz to 8 MHz); timers count clock cycles, so their periods follow it at once; the RTC's current half-period also finishes at the new speed, and the next is set from it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: level -- speed
 | Returns: the previous speed
 ----------------------*/
uint8_t vmu_set_clock(uint8_t level);

/*----------------------
 | vmu_clock
 | Description: The CPU clock speed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: speed 0-8
 ----------------------*/
uint8_t vmu_clock(void);

/*----------------------
 | vmu_sys_flags
 | Description: What the system asks of the game, as VMU_SYS_* bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: flags
 ----------------------*/
uint8_t vmu_sys_flags(void);

/*----------------------
 | vmu_power
 | Description: Stands by until an enabled line, or leaves the game for the BIOS.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mode -- VMU_POWER_*
 | Returns: N/A (does not return for the exits)
 ----------------------*/
void vmu_power(uint8_t mode);

/*----------------------
 | vmu_file8
 | Description: Reads a byte of the game's own file, the PocketStation header included.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: off -- file offset
 | Returns: the byte
 ----------------------*/
uint8_t vmu_file8(uint16_t off);

/*----------------------
 | vmu_file_write
 | Description: Writes 128 bytes of RAM to one 128-byte sector of the game's file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: vmu_ram
 | Params: sector -- sector number; src -- RAM offset
 | Returns: 0 on success, 1 on failure
 ----------------------*/
uint8_t vmu_file_write(uint16_t sector, uint16_t src);

/*----------------------
 | vmu_io_write
 | Description: Writes a peripheral register.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: reg -- VMU_IO_*; value -- value
 | Returns: N/A
 ----------------------*/
void vmu_io_write(uint8_t reg, uint16_t value);

/*----------------------
 | vmu_io_read
 | Description: Reads a peripheral register.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: reg -- VMU_IO_*
 | Returns: the value
 ----------------------*/
uint16_t vmu_io_read(uint8_t reg);

#endif
