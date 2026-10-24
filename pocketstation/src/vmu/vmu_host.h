/*----------------------
 | vmu_host.h
 | Description: Control of the host backend behind vmu_hw.h: the game's tables and pet save, the device state at entry, system flags, buttons with Mode and Sleep, the dock, the frame callback, time and the buzzer log.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef VMU_HOST_H
#define VMU_HOST_H

#include <stdint.h>

#include "vmu_hw.h"

/*----------------------
 | VmuBuzz
 | Description: One logged vmu_buzz or vmu_lcd call: the vmu_idle calls begun before it, which call it was, and its tone or on (0 or 1).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint32_t idle;
    uint8_t lcd;
    uint8_t value;
} VmuBuzz;

/*----------------------
 | vmu_host_init
 | Description: Starts the device with the game's tables and pet save, clear RAM, the tick off, only the RTC line enabled and the PocketStation launcher's RTC phase.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: rom -- the tables; rom_len -- their length; save -- the pet save BASLUSP01013; save_len -- its length; each at most 64 KB
 | Returns: N/A
 ----------------------*/
void vmu_host_init(const uint8_t *rom, uint32_t rom_len, const uint8_t *save, uint32_t save_len);

/*----------------------
 | vmu_host_set_entry
 | Description: Loads the five lines' interrupt state and the RTC phase the reference runner dumped at app entry, stopping on the RTC's /4096 mode; its timers are left to the game.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: w -- device words in psk_host_load_entry order; n -- word count
 | Returns: N/A
 ----------------------*/
void vmu_host_set_entry(const uint32_t *w, uint32_t n);

/*----------------------
 | vmu_host_set_sys
 | Description: Sets the flags vmu_sys_flags returns.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: flags -- VMU_SYS_* bits
 | Returns: N/A
 ----------------------*/
void vmu_host_set_sys(uint8_t flags);

/*----------------------
 | VMU_HOST_MODE / VMU_HOST_SLEEP
 | Description: The VMU's Mode and Sleep buttons in vmu_host_set_buttons masks, beside VMU_BTN_*; neither has a line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_HOST_MODE 0x20
#define VMU_HOST_SLEEP 0x40

/*----------------------
 | vmu_host_set_buttons
 | Description: Holds the buttons; a press raises its line's level and latches it if enabled, a release drops the level. Mode raises VMU_SYS_QUIT until read; Sleep sleeps unseen until pressed again, and buttons held at the wake read as released until let go.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mask -- VMU_BTN_* bits held, with VMU_HOST_MODE and VMU_HOST_SLEEP
 | Returns: N/A
 ----------------------*/
void vmu_host_set_buttons(uint8_t mask);

/*----------------------
 | vmu_host_set_dock
 | Description: Sets port 7's dock bit: docking raises the dock line's level and latches it if enabled, undocking drops the level.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: on -- nonzero when docked
 | Returns: N/A
 ----------------------*/
void vmu_host_set_dock(uint8_t on);

/*----------------------
 | vmu_host_set_frame
 | Description: The function vmu_present calls.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: frame -- callback, or null
 | Returns: N/A
 ----------------------*/
void vmu_host_set_frame(void (*frame)(void));

/*----------------------
 | vmu_host_time
 | Description: Timer counts run since vmu_host_init.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: counts
 ----------------------*/
uint64_t vmu_host_time(void);

/*----------------------
 | vmu_host_save
 | Description: The pet save as it stands, sector writes included.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: the save
 ----------------------*/
const uint8_t *vmu_host_save(void);

#ifdef VMU_COUNT_ACCESS
/*----------------------
 | vmu_host_counts
 | Description: Reads and writes of each vmu_ram offset since start; only in VMU_COUNT_ACCESS builds.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 0x400 counts
 ----------------------*/
const uint32_t *vmu_host_counts(void);
#endif

/*----------------------
 | vmu_host_buzz_log
 | Description: The vmu_buzz and vmu_lcd calls since vmu_host_init, up to the log's capacity.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: count -- set to the number of entries returned
 | Returns: the entries
 ----------------------*/
const VmuBuzz *vmu_host_buzz_log(uint32_t *count);

#endif
