#ifndef _CPU_H_
#define _CPU_H_

#include <stdbool.h>
#include <stdint.h>

typedef int (*lc8670_cpu_device_op_handler)();

enum
{
	LC8670_BANK_ROM,
	LC8670_BANK_FLASH_0,
	LC8670_BANK_FLASH_1,
};

enum IoPortNum
{
	LC8670_PORT1,       // 8-bit I/O port
	LC8670_PORT3,       // 8-bit I/O port
	LC8670_PORT7        // 4-bit I port
};

enum ExtIntNum
{
	LC8670_EXT_INT0 = 0,    // P70
	LC8670_EXT_INT1,        // P71
	LC8670_EXT_INT2,        // P72
	LC8670_EXT_INT3         // P73
};

enum
{
	BASE_TIMER = 1,
	CLOCK_TIMER = 2,
};

#define ICON_FILE	1
#define ICON_GAME	2
#define ICON_CLOCK	4
#define ICON_FLASH	8



//provided
int execute_run(int numInstrsToEecute);
void execute_set_input(enum ExtIntNum inputnum, int state);
void screen_update(uint8_t *data, uint8_t *icons);
void device_start();
void device_reset();
void device_timer(int id);	//should be called to update timers

uint8_t ext_io_r(enum IoPortNum iop);
void ext_io_w(enum IoPortNum iop, uint8_t val);
uint8_t ext_read_code(uint16_t addr);
void ext_code_bankswitch(uint8_t bank);	//LC8670_BANK_*
void ext_set_cpu_freq(uint32_t freq);
bool ext_cpu_is_running_in_rom(void);
void ext_write_flash_direct(uint32_t address, const uint8_t *data, uint32_t len);

//timers
void timer_adust(int id, uint32_t freq);

#endif