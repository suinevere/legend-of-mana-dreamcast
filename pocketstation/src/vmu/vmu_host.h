/*----------------------
 | vmu_host.h
 | Description: Control of the host backend behind vmu_hw.h: the game's file, the device state at entry, system flags, buttons, the frame callback, time and the peripheral write log.
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
 | VmuIoWrite
 | Description: One logged vmu_io_write: the clock cycle it happened at, the register and the value.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint64_t cycle;
    uint8_t reg;
    uint16_t value;
} VmuIoWrite;

/*----------------------
 | vmu_host_init
 | Description: Starts the device with the game's file, clear RAM and the PocketStation launcher's clock, interrupt mask and LCD mode.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: file -- the game's file; len -- its length, at most 64 KB
 | Returns: N/A
 ----------------------*/
void vmu_host_init(const uint8_t *file, uint32_t len);

/*----------------------
 | vmu_host_set_entry
 | Description: Loads the interrupt, timer, clock, RTC and peripheral state the reference runner dumped at app entry.
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
 | vmu_host_set_buttons
 | Description: Holds the buttons; a press raises its line's level and latches it if enabled, a release drops the level.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mask -- VMU_BTN_* bits held
 | Returns: N/A
 ----------------------*/
void vmu_host_set_buttons(uint8_t mask);

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
 | vmu_host_cycles
 | Description: Clock cycles run since vmu_host_init.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: cycles
 ----------------------*/
uint64_t vmu_host_cycles(void);

/*----------------------
 | vmu_host_file
 | Description: The game's file as it stands, sector writes included.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: the file
 ----------------------*/
const uint8_t *vmu_host_file(void);

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
 | vmu_host_io_log
 | Description: The peripheral writes since vmu_host_init, up to the log's capacity.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: count -- set to the number of entries returned
 | Returns: the entries
 ----------------------*/
const VmuIoWrite *vmu_host_io_log(uint32_t *count);

#endif
