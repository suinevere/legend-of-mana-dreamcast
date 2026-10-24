/*----------------------
 | vmu_hw.h
 | Description: The only way the VMU-shaped Ring Ring Land reaches RAM, the screen, buttons, five interrupt lines and their one handler, its tick, the sound end, the buzzer, the system, its tables and its pet save.
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
 | VMU_LINE_BUTTONS / VMU_LINE_TICK / VMU_LINE_RTC / VMU_LINE_DOCK / VMU_LINE_SOUND
 | Description: The five interrupt lines, numbered as the PocketStation's: buttons, the tick, the RTC's rising edge, the dock and the sound end. All go to the one handler.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_LINE_BUTTONS 0x001Fu
#define VMU_LINE_TICK 0x0100u
#define VMU_LINE_RTC 0x0200u
#define VMU_LINE_DOCK 0x0400u
#define VMU_LINE_SOUND 0x2000u

/*----------------------
 | VMU_TICK_OFF / VMU_TICK_FRAME / VMU_TICK_FAST / VMU_TICK_STANDBY
 | Description: vmu_tick_mode modes: no tick; reload 15,625 counts at clock 5; reload 15,625 at clock 6, the waited-sound rate; reload 1,953 at clock 2. A period is reload + 1 counts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_TICK_OFF 0
#define VMU_TICK_FRAME 1
#define VMU_TICK_FAST 2
#define VMU_TICK_STANDBY 3

/*----------------------
 | VMU_SOUND_SLOW / VMU_SOUND_FAST
 | Description: vmu_sound_play sample reloads in timer counts: 500 (a 501-count sample period) for sounds, 71 (72 counts) for a waited sound.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_SOUND_SLOW 500
#define VMU_SOUND_FAST 71

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
 | Description: vmu_power modes: stop until an enabled button or RTC line, leave to the BIOS, leave to the BIOS because docked.
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
 | Description: The game's only wait point: lets time run to the next tick, sound end or RTC edge, then calls the handler while an enabled line is latched.
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
 | Description: Clears latched lines, and their request levels except for buttons and RTC, which follow their signals.
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
 | Description: Interrupt request levels: buttons held, the RTC square wave and the dock.
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
 | vmu_set_handler
 | Description: Installs the one handler vmu_idle calls for every line; with none, vmu_idle acknowledges what it would have delivered.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: irq -- handler, or null
 | Returns: N/A
 ----------------------*/
void vmu_set_handler(void (*irq)(void));

/*----------------------
 | vmu_tick_mode
 | Description: Sets the tick mode; OFF stops the tick, a running mode from OFF loads a full period, and between running modes the current period finishes and the next is the new mode's.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mode -- VMU_TICK_*
 | Returns: N/A
 ----------------------*/
void vmu_tick_mode(uint8_t mode);

/*----------------------
 | vmu_sound_play
 | Description: Latches VMU_LINE_SOUND after (samples + 1) * (counts_per_sample + 1) counts, when a sample timer would play samples samples and expire once more; replaces any sound already timed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: samples -- samples to play; counts_per_sample -- VMU_SOUND_*
 | Returns: N/A
 ----------------------*/
void vmu_sound_play(uint32_t samples, uint16_t counts_per_sample);

/*----------------------
 | vmu_sound_stop
 | Description: Cancels the sound end and acknowledges VMU_LINE_SOUND.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void vmu_sound_stop(void);

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
 | Description: Stands by, taking events with no handler until an enabled button or RTC line is latched (left for the next vmu_idle), or leaves the game for the BIOS.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mode -- VMU_POWER_*
 | Returns: N/A (does not return for the exits)
 ----------------------*/
void vmu_power(uint8_t mode);

/*----------------------
 | vmu_rom8
 | Description: Reads a byte of the game's tables, at the offsets of the PocketStation file, its header included.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: off -- table offset
 | Returns: the byte
 ----------------------*/
uint8_t vmu_rom8(uint16_t off);

/*----------------------
 | vmu_save_read
 | Description: Reads one 128-byte sector of the pet save BASLUSP01013, laid out as the PocketStation file, into RAM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: vmu_ram
 | Params: sector -- sector number; dst -- RAM offset, 0x100-0x268 (work RAM before the runtime's rt_save)
 | Returns: 0 on success, 1 on failure
 ----------------------*/
uint8_t vmu_save_read(uint16_t sector, uint16_t dst);

/*----------------------
 | vmu_save_write
 | Description: Writes 128 bytes of RAM to one 128-byte sector of the pet save BASLUSP01013; the VMU also refreshes the save package's CRC.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: vmu_ram
 | Params: sector -- sector number; src -- RAM offset, 0x100-0x268 (work RAM before the runtime's rt_save)
 | Returns: 0 on success, 1 on failure
 ----------------------*/
uint8_t vmu_save_write(uint16_t sector, uint16_t src);

/*----------------------
 | vmu_buzz
 | Description: Sets the buzzer: 0 silences it, 1-255 sounds a square wave of that pitch step until the next call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: tone -- 0 off, 1-255 a pitch step
 | Returns: N/A
 ----------------------*/
void vmu_buzz(uint8_t tone);

/*----------------------
 | vmu_lcd
 | Description: Switches the LCD on or off; the frame buffer is kept either way.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: on -- nonzero for on
 | Returns: N/A
 ----------------------*/
void vmu_lcd(uint8_t on);

#endif
