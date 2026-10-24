//tis file is mostly from MAME, just cleaned up and converted to a sane programing language








#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cpu.h"

//***************************************************************************
//    DEBUGGING
//***************************************************************************

#define LOG_TIMERS          0
#define LOG_IRQ             0


//**************************************************************************
//  PREDEFINITIONS
//**************************************************************************

static void set_irq_flag(int source);
static int decode_op(uint8_t op);
static void check_p_flag();
static void check_p3int();
static void change_clock_source();
static uint16_t get_addr();
static uint8_t get_data();
static void write_data_latch(uint16_t offset, uint8_t data);
static void update_port1(uint8_t data);
static void set_pc(uint16_t new_pc);
static void push(uint8_t data);
static uint8_t pop();
static uint16_t get_addr();
static uint8_t fetch();
static uint8_t read_data(uint16_t offset);
static void write_data(uint16_t offset, uint8_t data);
static uint8_t read_data_latch(uint16_t offset);
static void check_irqs();
static void base_timer_tick();
static void timer0_prescaler_tick();
static void timer0_tick(bool ext_line);
static void timer1_tick();
static uint8_t mem_io_mram_r(uint32_t offset);
static void mem_io_mram_w(uint32_t offset, uint8_t data);
static uint8_t mem_io_xram_r(uint32_t offset);
static void mem_io_xram_w(uint32_t offset, uint8_t data);
static uint8_t mem_io_regs_r(uint32_t offset);
static void mem_io_regs_w(uint32_t offset, uint8_t data);
static uint8_t mem_io_r(uint32_t offset);
static void mem_io_w(uint32_t offset, uint8_t data);



static const lc8670_cpu_device_op_handler s_opcode_table[];




//**************************************************************************
//  DATA STRUCTURES
//**************************************************************************
static uint8_t               m_sfr[0x80];            // special function registers
static uint8_t               m_op;
static uint16_t              m_pc;
static uint8_t               m_mram[0x200];          // main RAM
static uint16_t              m_irq_flag;
static uint8_t               m_irq_lev;
static bool                  m_after_reti;
static uint32_t              m_clocks[3];            // clock sources
static bool                  m_clock_changed;
static uint8_t               m_p1_data;
static uint8_t               m_vtrbf[0x200];         // work RAM
static uint8_t               m_xram[0xc6];           // XRAM
static uint8_t               m_timer0_prescaler;
static uint8_t               m_timer0[2];
static uint8_t               m_timer1[2];
static uint8_t               m_timer1_comparator[2];
static uint8_t               m_base_timer[2];
static int                   m_input_lines[4];
static int                   m_flash_unlock_seq;


//**************************************************************************
//  TYPES
//**************************************************************************



//**************************************************************************
//  MACROS
//**************************************************************************

#define fatalerror(...)						fprintf(stderr, __VA_ARGS__)
#define logerror(...)						fprintf(stderr, __VA_ARGS__)
#define tag()								"ERR"
#define BIT(_r, _b)							(!!((_r) & (1 << (_b))))
#define machine_side_effect_disabled()		false
#define machine_describe_context()			"FATAL"
#define standard_irq_callback(...)

// registers
#define     REG_A       m_sfr[0x00]
#define     REG_PSW     m_sfr[0x01]
#define     REG_B       m_sfr[0x02]
#define     REG_C       m_sfr[0x03]
#define     REG_TRL     m_sfr[0x04]
#define     REG_TRH     m_sfr[0x05]
#define     REG_SP      m_sfr[0x06]
#define     REG_PCON    m_sfr[0x07]
#define     REG_IE      m_sfr[0x08]
#define     REG_IP      m_sfr[0x09]
#define     REG_EXT     m_sfr[0x0d]
#define     REG_OCR     m_sfr[0x0e]
#define     REG_T0CNT   m_sfr[0x10]
#define     REG_T0PRR   m_sfr[0x11]
#define     REG_T0LR    m_sfr[0x13]
#define     REG_T0HR    m_sfr[0x15]
#define     REG_T1CNT   m_sfr[0x18]
#define     REG_T1LC    m_sfr[0x1a]
#define     REG_T1LR    m_sfr[0x1b]
#define     REG_T1HC    m_sfr[0x1c]
#define     REG_T1HR    m_sfr[0x1d]
#define     REG_MCR     m_sfr[0x20]
#define     REG_STAD    m_sfr[0x22]
#define     REG_CNR     m_sfr[0x23]
#define     REG_TDR     m_sfr[0x24]
#define     REG_XBNK    m_sfr[0x25]
#define     REG_VCCR    m_sfr[0x27]
#define     REG_SCON0   m_sfr[0x30]
#define     REG_SBUF0   m_sfr[0x31]
#define     REG_SBR     m_sfr[0x32]
#define     REG_SCON1   m_sfr[0x34]
#define     REG_SBUF1   m_sfr[0x35]
#define     REG_P1      m_sfr[0x44]
#define     REG_P1DDR   m_sfr[0x45]
#define     REG_P1FCR   m_sfr[0x46]
#define     REG_P3      m_sfr[0x4c]
#define     REG_P3DDR   m_sfr[0x4d]
#define     REG_P3INT   m_sfr[0x4e]
#define     REG_FPR     m_sfr[0x54]
#define     REG_I01CR   m_sfr[0x5d]
#define     REG_I23CR   m_sfr[0x5e]
#define     REG_ISL     m_sfr[0x5f]
#define     REG_VSEL    m_sfr[0x63]
#define     REG_VRMAD1  m_sfr[0x64]
#define     REG_VRMAD2  m_sfr[0x65]
#define     REG_BTCR    m_sfr[0x7f]

// addressing modes
#define     GET_D9      (((m_op & 0x01)<<8) | fetch())
#define     GET_D9B3    (((m_op & 0x10)<<4) | fetch())
#define     GET_I8      fetch()
#define     GET_R8      fetch()
#define     GET_RI      (m_op & 0x03)
#define     GET_B3      (m_op & 0x07)
#define     GET_A12     (((m_op & 0x10)<<7) | ((m_op & 0x07)<<8) | fetch())
#define     SIGNED(v)   ((v) - (BIT(v,7) ? 0x100 : 0))

// flags
#define     FLAG_CY     0x80
#define     FLAG_AC     0x40
#define     FLAG_OV     0x04
#define     FLAG_P      0x01
#define     GET_CY      BIT(REG_PSW,7)
#define     GET_AC      BIT(REG_PSW,6)
#define     GET_OV      BIT(REG_PSW,2)
#define     GET_P       BIT(REG_PSW,0)
#define     SET_CY(v)   do { if (v) REG_PSW |= FLAG_CY; else REG_PSW &= ~FLAG_CY; } while(0)
#define     SET_AC(v)   do { if (v) REG_PSW |= FLAG_AC; else REG_PSW &= ~FLAG_AC; } while(0)
#define     SET_OV(v)   do { if (v) REG_PSW |= FLAG_OV; else REG_PSW &= ~FLAG_OV; } while(0)
#define     CHECK_P()   check_p_flag()

// CPU state
#define     HALT_MODE   0x01
#define     HOLD_MODE   0x02




//**************************************************************************
//  IRQ vectors
//**************************************************************************

static const uint16_t s_irq_vectors[] =
{
	0x0000, 0x0003, 0x000b, 0x0013, 0x001b, 0x0023, 0x002b, 0x0033,
	0x003b, 0x0043, 0x004b, 0x004f, 0x0052, 0x0055, 0x005a, 0x005d
};


//**************************************************************************
//  LC8670 DEVICE
//**************************************************************************

*/
//-------------------------------------------------
//  device_start - start up the device
//-------------------------------------------------

void device_start()
{
	// setup timers
	timer_adust(BASE_TIMER, 32768);
}


//-------------------------------------------------
//  device_reset - reset up the device
//-------------------------------------------------

void device_reset()
{
	m_pc = s_irq_vectors[0];
//DGV	m_ppc = m_pc;
	m_op = 0;
	m_irq_flag = 0;
	m_irq_lev = 0;
	m_after_reti = false;
	m_p1_data = 0;
	m_timer0_prescaler = 0;
	m_timer0[0] = m_timer0[1] = 0;
	m_timer1[0] = m_timer1[1] = 0;
	m_timer1_comparator[0] = m_timer1_comparator[1] = 0;
	m_base_timer[0] = m_base_timer[1] = 0;
	m_clock_changed = false;
	memset(m_sfr, 0, 0x80);
	memset(m_mram, 0, 0x200);
	memset(m_xram, 0, 0xc6);
	memset(m_vtrbf, 0, 0x200);
	m_flash_unlock_seq = 0;

	// default values from VMU hardware manual
	REG_P1FCR = 0xbf;
	REG_P3INT = 0xfd;
	REG_ISL = 0xc0;
	REG_VSEL = 0xfc;
	REG_BTCR = 0x41;

	// reset bankswitch and clock source
	ext_code_bankswitch(0);
	change_clock_source();
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void device_timer(int id)
{
	switch(id)
	{
		case BASE_TIMER:
			if (!(REG_ISL & 0x10))
				base_timer_tick();
			break;

		case CLOCK_TIMER:
			timer0_prescaler_tick();
			timer1_tick();

			if ((REG_ISL & 0x30) == 0x10)
				base_timer_tick();
			break;
	}
}

//-------------------------------------------------
//DGV: mem access
//-------------------------------------------------

static uint8_t mem_io_r(uint32_t offset) {
	if (offset < 0x100)
		return mem_io_mram_r(offset);
	else if (offset < 0x180)
		return mem_io_regs_r(offset - 0x100);
	else if (offset < 0x200)
		return mem_io_xram_r(offset - 0x180);
	else {
		///ERROR
		fprintf(stderr, "MEM READ ERR: [0x%08X]\n", offset);
		return 0;
	}
}

static void mem_io_w(uint32_t offset, uint8_t data) {
	if (offset < 0x100)
		mem_io_mram_w(offset, data);
	else if (offset < 0x180)
		mem_io_regs_w(offset - 0x100, data);
	else if (offset < 0x200)
		mem_io_xram_w(offset - 0x180, data);
	else {
		///ERROR
		fprintf(stderr, "MEM WRITE ERR: [0x%08X] <- 0x%02X\n", offset, data);
	}
}


*/
//-------------------------------------------------
//  execute - execute for the provided number of
//  countcles
//-------------------------------------------------

int execute_run(int numInstrsToEecute)
{
	int instrsExecuted = 0;
	
	if (m_clock_changed)
	{
		change_clock_source();
		return 0;
	}

	do
	{
		check_irqs();

		int cycles;

		if (REG_PCON & HALT_MODE)
		{
			// in HALT state the timers are still updated
			cycles = 1;
		}
		else
		{
			
//			fprintf(stderr, "[0x%04X] = ", m_pc);
			
			// instruction fetch
			m_op = fetch();
			
//			fprintf(stderr,"0x%02X\n", m_op);
			
			int op_idx = decode_op(m_op);

			// execute the instruction
			cycles = (s_opcode_table[op_idx])();
		}

		// update the instruction counter
		instrsExecuted += cycles;
		numInstrsToEecute -= cycles;
	}
	while (numInstrsToEecute > 0 && !m_clock_changed);
	
	return instrsExecuted;
}


//-------------------------------------------------
//  execute_set_input
//-------------------------------------------------

void execute_set_input(enum ExtIntNum inputnum, int state)
{
	switch(inputnum)
	{
		case LC8670_EXT_INT0:
			if ((REG_I01CR & 0x0c) == 0x00 && m_input_lines[inputnum] && !state)        // falling edge
			{
				REG_I01CR |= 0x02;
				if (REG_I01CR & 0x01)
					set_irq_flag(1);
			}
			else if ((REG_I01CR & 0x0c) == 0x04 && !state)      // low level
			{
				REG_I01CR |= 0x02;
				if (REG_I01CR & 0x01)
					set_irq_flag(1);
			}
			else if ((REG_I01CR & 0x0c) == 0x08 && !m_input_lines[inputnum] && state)       // rising edge
			{
				REG_I01CR |= 0x02;
				if (REG_I01CR & 0x01)
					set_irq_flag(1);
			}
			else if ((REG_I01CR & 0x0c) == 0x0c && state)       // high level
			{
				REG_I01CR |= 0x02;
				if (REG_I01CR & 0x01)
					set_irq_flag(1);
			}
			break;
		case LC8670_EXT_INT1:
			if ((REG_I01CR & 0xc0) == 0x00 && m_input_lines[inputnum] && !state)        // falling edge
			{
				REG_I01CR |= 0x20;
				if (REG_I01CR & 0x10)
					set_irq_flag(2);
			}
			else if ((REG_I01CR & 0xc0) == 0x40 && !state)      // low level
			{
				REG_I01CR |= 0x20;
				if (REG_I01CR & 0x10)
					set_irq_flag(2);
			}
			else if ((REG_I01CR & 0xc0) == 0x80 && !m_input_lines[inputnum] && state)       // rising edge
			{
				REG_I01CR |= 0x20;
				if (REG_I01CR & 0x10)
					set_irq_flag(2);
			}
			else if ((REG_I01CR & 0xc0) == 0xc0 && state)       // high level
			{
				REG_I01CR |= 0x20;
				if (REG_I01CR & 0x10)
					set_irq_flag(2);
			}
			break;
		case LC8670_EXT_INT2:
			if ((REG_I23CR & 0x04) && m_input_lines[inputnum] && !state)    // falling edge
			{
				if (!(REG_ISL & 0x01))
					timer0_tick(true);

				REG_I23CR |= 0x02;
				if (REG_I23CR & 0x01)
					set_irq_flag(3);
			}
			if ((REG_I23CR & 0x08) && !m_input_lines[inputnum] && state)    // rising edge
			{
				if (!(REG_ISL & 0x01))
					timer0_tick(true);

				REG_I23CR |= 0x02;
				if (REG_I23CR & 0x01)
					set_irq_flag(3);
			}
			break;
		case LC8670_EXT_INT3:
			if ((REG_I23CR & 0x40) && m_input_lines[inputnum] && !state)    // falling edge
			{
				if (REG_ISL & 0x01)
					timer0_tick(true);

				REG_I23CR |= 0x20;
				if (REG_I23CR & 0x10)
					set_irq_flag(4);
			}
			if ((REG_I23CR & 0x80) && !m_input_lines[inputnum] && state)    // rising edge
			{
				if (REG_ISL & 0x01)
					timer0_tick(true);

				REG_I23CR |= 0x20;
				if (REG_I23CR & 0x10)
					set_irq_flag(4);
			}
			break;
	}

	m_input_lines[inputnum] = state;
}


//-------------------------------------------------
//  screen_update - handle updating the screen
//-------------------------------------------------

void screen_update(uint8_t *data, uint8_t *icons)
{
	if ((REG_MCR & 0x08) && (REG_VCCR & 0x80)) {	//screen on
		
		if (data)
			memcpy(data, m_xram, 192);
		if (icons) {
			
			*icons = 0;
			
			if (m_xram[0xc1] & (1 << 6))
				*icons |= ICON_FILE;
			if (m_xram[0xc2] & (1 << 4))
				*icons |= ICON_GAME;
			if (m_xram[0xc3] & (1 << 2))
				*icons |= ICON_CLOCK;
			if (m_xram[0xc4] & (1 << 0))
				*icons |= ICON_FLASH;
		}
	}
	else {
		if (icons)
			*icons = 0;
		if (data)
			memset(data, 0, 32 * 48 / 8);
	}
}


//-------------------------------------------------
//  check_irqs - check for interrupts request
//-------------------------------------------------

static void check_irqs()
{
	// update P3 interrupt
	check_p3int();

	if (m_irq_flag && !m_after_reti)
	{
		int irq = 0;
		uint8_t priority = 0;

		// highest priority IRQ
		if (!(REG_IE & 0x01) && (m_irq_flag & 0x02))
		{
			irq = 0x01;
			priority = 2;
		}
		else if (!(REG_IE & 0x02) && (m_irq_flag & 0x04))
		{
			irq = 0x02;
			priority = 2;
		}

		// high priority IRQ
		else if ((REG_IE & 0x80) && ((REG_IP<<3) & m_irq_flag))
		{
			for(int i=3; i<=10; i++)
				if ((m_irq_flag & (REG_IP<<3)) & (1<<i))
				{
					irq = i;
					priority = 1;
					break;
				}
		}

		// low priority IRQ
		else if ((REG_IE & 0x80) && (m_irq_flag & 0x02))
		{
			irq = 0x01;
			priority = 0;
		}
		else if ((REG_IE & 0x80) && (m_irq_flag & 0x04))
		{
			irq = 0x02;
			priority = 0;
		}
		else if (REG_IE & 0x80)
		{
			for(int i=3; i<=10; i++)
				if (m_irq_flag & (1<<i))
				{
					irq = i;
					priority = 0;
					break;
				}
		}

		// IRQ with less priority of current interrupt are not executed until the end of the current interrupt routine
		if (irq != 0 && ((m_irq_lev & (1<<priority)) || (priority == 0 && (m_irq_lev & 0x06)) || (priority == 1 && (m_irq_lev & 0x04))))
		{
			if (LOG_IRQ)    logerror("%s: interrupt %d (Priority=%d, Level=%d) delayed\n", tag(), irq, priority, m_irq_lev);
			irq = 0;
		}

		if (irq != 0)
		{
			if (LOG_IRQ)    logerror("%s: interrupt %d (Priority=%d, Level=%d) executed\n", tag(), irq, priority, m_irq_lev);

			m_irq_lev |= (1<<priority);

			push((m_pc>>0) & 0xff);
			push((m_pc>>8) & 0xff);

			set_pc(s_irq_vectors[irq]);

			REG_PCON &= ~HALT_MODE;     // interrupts resume from HALT state

			// clear the IRQ flag
			m_irq_flag &= ~(1<<irq);

			standard_irq_callback(irq);
		}
	}

	// at least one opcode need to be executed after a RETI before another IRQ can be accepted
	m_after_reti = false;
}


//-------------------------------------------------
//  base_timer_tick - update base timer
//-------------------------------------------------

static void base_timer_tick()
{
	if (REG_BTCR & 0x40)
	{
		uint16_t base_counter_l = m_base_timer[0] + 1;
		uint16_t base_counter_h = m_base_timer[1];

		if (REG_BTCR & 0x80)    // 6-bit mode
			base_counter_h++;
		else if (base_counter_l & 0x100)
			base_counter_h++;

		if (base_counter_h & 0x40)
		{
			if (LOG_TIMERS) logerror("%s: base timer 0 overflow, IRQ: %d\n", tag(), BIT(REG_BTCR,0));
			REG_BTCR |= 0x02;
			if (REG_BTCR & 0x01)
				set_irq_flag(4);
		}

		bool bt1_req = false;
		switch(REG_BTCR & 0x30)
		{
			case 0x00:
				if (base_counter_l & 0x20)
					bt1_req = true;
				break;
			case 0x10:
				if (base_counter_l & 0x80)
					bt1_req = true;
				break;
			case 0x20:
				if (base_counter_h & 0x01)
					bt1_req = true;
				break;
			case 0x30:
				if (base_counter_h & 0x04)
					bt1_req = true;
				break;
		}

		if (bt1_req)
		{
			if (LOG_TIMERS) logerror("%s: base timer 1 overflow, IRQ: %d\n", tag(), BIT(REG_BTCR,3));
			REG_BTCR |= 0x08;
			if (REG_BTCR & 0x04)
				set_irq_flag(4);
		}

		if (((base_counter_l & 0x04) && (REG_ISL & 0x08)) || ((base_counter_l & 0x08) && !(REG_ISL & 0x08)))
			update_port1(m_p1_data | 0x40);
		else
			update_port1(m_p1_data & 0xbf);

		m_base_timer[0] = base_counter_l & 0xff;
		m_base_timer[1] = base_counter_h & 0x3f;
	}
}

//-------------------------------------------------
//  update_to_prescaler - update timer 0 prescaler
//-------------------------------------------------

static void timer0_prescaler_tick()
{
	uint16_t prescaler = m_timer0_prescaler + 1;
	if (prescaler & 0x100)
	{
		if (LOG_TIMERS) logerror("%s: timer0 prescaler overflow\n", tag());

		if ((REG_ISL & 0x30) == 0x30)
			base_timer_tick();

		timer0_tick(false);

		m_timer0_prescaler = REG_T0PRR;
	}
	else
	{
		m_timer0_prescaler = prescaler & 0xff;
	}
}

//-------------------------------------------------
//  timer0_tick - update timer 0
//-------------------------------------------------

static void timer0_tick(bool ext_line)
{
	if (REG_T0CNT & 0xc0)
	{
		if (REG_T0CNT & 0x20)
		{
			// 16-bit timer/counter mode
			if ((REG_T0CNT & 0xc0) == 0xc0 && (((REG_T0CNT & 0x10) && ext_line) || (!(REG_T0CNT & 0x10) && !ext_line)))
			{
				uint32_t timer0 = ((m_timer0[1] << 8) | m_timer0[0]) + 1;

				if (timer0 & 0x10000)
				{
					if (LOG_TIMERS) logerror("%s: timer0 long overflow, IRQ: %d\n", tag(), BIT(REG_T0CNT,3));
					m_timer0[0] = REG_T0LR;
					m_timer0[1] = REG_T0HR;
					REG_T0CNT |= 0x0a;
					if (REG_T0CNT & 0x04)
						set_irq_flag(5);
				}
				else
				{
					m_timer0[0] = (timer0>>0) & 0xff;
					m_timer0[1] = (timer0>>8) & 0xff;
				}
			}
		}
		else
		{
			// 8-bit timer/counter mode
			if ((REG_T0CNT & 0x40) && (((REG_T0CNT & 0x10) && ext_line) || (!(REG_T0CNT & 0x10) && !ext_line)))
			{
				uint16_t timer0l = m_timer0[0] + 1;

				if (timer0l & 0x100)
				{
					if (LOG_TIMERS) logerror("%s: timer0 low overflow, IRQ: %d\n", tag(), BIT(REG_T0CNT,0));
					m_timer0[0] = REG_T0LR;
					REG_T0CNT |= 0x02;
					if (REG_T0CNT & 0x01)
						set_irq_flag(3);
				}
				else
				{
					m_timer0[0]  = timer0l & 0xff;
				}
			}
			if ((REG_T0CNT & 0x80)  && !ext_line)
			{
				uint16_t timer0h = m_timer0[1] + 1;
				if (timer0h & 0x100)
				{
					if (LOG_TIMERS) logerror("%s: timer0 high overflow, IRQ: %d\n", tag(), BIT(REG_T0CNT,3));
					m_timer0[1] = REG_T0HR;
					REG_T0CNT |= 0x08;
					if (REG_T0CNT & 0x04)
						set_irq_flag(5);
				}
				else
				{
					m_timer0[1]  = timer0h & 0xff;
				}
			}
		}
	}
}

//-------------------------------------------------
//  timer1_tick - update timer 1
//-------------------------------------------------

static void timer1_tick()
{
	if (REG_T1CNT & 0xc0)
	{
		if (REG_T1CNT & 0x20)
		{
			if (REG_T1CNT & 0x40)
			{
				// 16-bit timer mode
				uint16_t timer1l = m_timer1[0] + (REG_T1CNT & 0x80 ? 1 : 2);
				if (timer1l & 0x100)
				{
					uint16_t timer1h = m_timer1[1] + 1;
					m_timer1[0] = REG_T1LR;
					REG_T1CNT |= 0x04;

					if (timer1h & 0x100)
					{
						if (LOG_TIMERS) logerror("%s: timer1 long overflow, IRQ: %d\n", tag(), BIT(REG_T1CNT,3));
						m_timer1[1] = REG_T1HR;
						REG_T1CNT |= 0x08;
						if (REG_T1CNT & 0x05)
							set_irq_flag(6);
					}
					else
					{
						m_timer1[1] = timer1h & 0xff;
					}
				}
				else
				{
					m_timer1[0]  = timer1l & 0xff;
				}
			}
		}
		else
		{
			// 8-bit timer/pulse generator mode
			if (REG_T1CNT & 0x40)
			{
				uint16_t timer1l = m_timer1[0] + 1;

				if (timer1l == m_timer1_comparator[0])
					update_port1(m_p1_data | 0x80);

				if (timer1l & 0x100)
				{
					if (LOG_TIMERS) logerror("%s: timer1 low overflow, IRQ: %d\n", tag(), BIT(REG_T1CNT,0));
					m_timer1[0] = REG_T1LR;
					update_port1(m_p1_data & 0x7f);
					REG_T1CNT |= 0x02;
					if (REG_T1CNT & 0x01)
						set_irq_flag(6);
				}
				else
				{
					m_timer1[0] = timer1l & 0xff;
				}
			}
			if (REG_T1CNT & 0x80)
			{
				uint16_t timer1h = m_timer1[1] + 1;

				if (timer1h & 0x100)
				{
					if (LOG_TIMERS) logerror("%s: timer1 high overflow, IRQ: %d\n", tag(), BIT(REG_T1CNT,3));
					m_timer1[1] = REG_T1HR;
					REG_T1CNT |= 0x08;
					if (REG_T1CNT & 0x04)
						set_irq_flag(6);
				}
				else
				{
					m_timer1[1] = timer1h & 0xff;
				}
			}
		}
	}
}


//**************************************************************************
//  internal map handlers
//**************************************************************************

static uint8_t mem_io_mram_r(uint32_t offset)
{
	return m_mram[BIT(REG_PSW,1)*0x100 + offset];
}

static void mem_io_mram_w(uint32_t offset, uint8_t data)
{
	m_mram[BIT(REG_PSW,1)*0x100 + offset] = data;
}

static uint8_t mem_io_xram_r(uint32_t offset)
{
	if (!(REG_VCCR & 0x40) || machine_side_effect_disabled())  // XRAM access enabled
	{
		uint8_t * xram_bank = m_xram + (REG_XBNK & 0x03) * 0x60;

		switch(REG_XBNK & 0x03)
		{
			case 0:
			case 1:
				if ((offset & 0x0f) < 0x0c)
					return xram_bank[(offset>>4) * 0x0c + (offset & 0x0f)];
				break;
			case 2:
				if (offset < 0x06)
					return xram_bank[offset];
				break;
		}
	}

	return 0xff;
}

static void mem_io_xram_w(uint32_t offset, uint8_t data)
{
	if (!(REG_VCCR & 0x40) || machine_side_effect_disabled())  // XRAM access enabled
	{
		uint8_t * xram_bank = m_xram + (REG_XBNK & 0x03) * 0x60;

		switch(REG_XBNK & 0x03)
		{
			case 0:
			case 1:
				if ((offset & 0x0f) < 0x0c)
					xram_bank[(offset>>4) * 0x0c + (offset & 0x0f)] = data;
				break;
			case 2:
				if (offset < 0x06)
					xram_bank[offset] = data;
				break;
		}
	}
}

static uint8_t mem_io_regs_r(uint32_t offset)
{
	switch(offset)
	{
		case 0x12:
			return m_timer0[0];
		case 0x14:
			return m_timer0[1];
		case 0x1b:
			return m_timer1[0];
		case 0x1d:
			return m_timer1[1];
		case 0x44:
			return (REG_P1 & REG_P1DDR) | (ext_io_r(LC8670_PORT1) & (REG_P1DDR ^ 0xff));
		case 0x4c:
			return (REG_P3 & REG_P3DDR) | (ext_io_r(LC8670_PORT3) & (REG_P3DDR ^ 0xff));
		case 0x5c:
			return ext_io_r(LC8670_PORT7) | 0xf0;    // 4-bit read-only port
		case 0x66:
		{
			uint8_t data = m_vtrbf[((REG_VRMAD2<<8) | REG_VRMAD1) & 0x1ff];
			if (!machine_side_effect_disabled() && (REG_VSEL & 0x10))
			{
				uint16_t vrmad = (REG_VRMAD1 | (REG_VRMAD2<<8)) + 1;
				REG_VRMAD1 = vrmad & 0xff;
				REG_VRMAD2 = (vrmad >> 8) & 0x01;
			}
			return data;
		}

		// write-only registers
		case 0x20: case 0x23: case 0x24: case 0x27:
		case 0x45: case 0x46: case 0x4d:
			if(!machine_side_effect_disabled())    logerror("%s: read write-only SFR %04x\n", machine_describe_context(), offset);
			return 0xff;
	}
	return m_sfr[offset];
}

static void mem_io_regs_w(uint32_t offset, uint8_t data)
{
	switch(offset)
	{
		case 0x00:
			REG_A = data;
			CHECK_P();
			break;
		case 0x07:
			if (data & HOLD_MODE)
				fatalerror("%s: unemulated HOLD mode\n", machine_describe_context());
			break;
		case 0x10:
			if (!(data & 0x80))
				m_timer0[1] = REG_T0HR;
			if (!(data & 0x40))
				m_timer0[0] = REG_T0LR;
			break;
		case 0x18:
			if ((data & 0x10) && !(REG_T1CNT & 0x10))
			{
				m_timer1_comparator[0] = REG_T1LC;
				m_timer1_comparator[1] = REG_T1HC;
			}
			if (!(data & 0x80))
				m_timer1[1] = REG_T1HR;
			if (!(data & 0x40))
				m_timer1[0] = REG_T1LR;
			break;
		case 0x1a:
			if ((REG_T1CNT & 0x10) || !(REG_T1CNT & 0x40))
				m_timer1_comparator[0] = data;
			break;
		case 0x1c:
			if ((REG_T1CNT & 0x10) || !(REG_T1CNT & 0x80))
				m_timer1_comparator[1] = data;
			break;
		case 0x0e:
			if ((data ^ REG_OCR) & 0xb0)
				m_clock_changed = true;
			break;
		case 0x44:
			ext_io_w(LC8670_PORT1, ((data | (m_p1_data & REG_P1FCR)) & REG_P1DDR) | (ext_io_r(LC8670_PORT1) & (REG_P1DDR ^ 0xff)));
			break;
		case 0x4c:
			ext_io_w(LC8670_PORT3, (data & REG_P3DDR) | (ext_io_r(LC8670_PORT3) & (REG_P3DDR ^ 0xff)));
			break;
		case 0x66:
			m_vtrbf[((REG_VRMAD2<<8) | REG_VRMAD1) & 0x1ff] = data;
			if (!machine_side_effect_disabled() && (REG_VSEL & 0x10))
			{
				uint16_t vrmad = (REG_VRMAD1 | (REG_VRMAD2<<8)) + 1;
				REG_VRMAD1 = vrmad & 0xff;
				REG_VRMAD2 = (vrmad >> 8) & 0x01;
			}
			break;
		case 0x7f:
			if (!(data & 0x40))
				m_base_timer[0] = m_base_timer[1] = 0;  // stop the timer clear the counter
			break;

		// read-only registers
		case 0x12: case 0x14: case 0x5c:
			if(!machine_side_effect_disabled())    logerror("%s: write read-only SFR %04x = %02x\n", machine_describe_context(), offset, data);
			return;
	}

	m_sfr[offset] = data;
}


//**************************************************************************
//  HELPERS
//**************************************************************************

static uint8_t fetch()
{
	uint8_t data = ext_read_code(m_pc);

	set_pc(m_pc + 1);

	return data;
}

static uint8_t read_data(uint16_t offset)
{
	return mem_io_r(offset);
}

static void write_data(uint16_t offset, uint8_t data)
{
	mem_io_w(offset, data);
}

static uint8_t read_data_latch(uint16_t offset)
{
	if (offset == 0x144)
		return REG_P1;
	else if (offset == 0x14c)
		return REG_P3;
	else
		return read_data(offset);
}

static void write_data_latch(uint16_t offset, uint8_t data)
{
	if (offset == 0x144)
		REG_P1 = data;
	else if (offset == 0x14c)
		REG_P3 = data;
	else
		write_data(offset, data);
}

static void update_port1(uint8_t data)
{
	m_p1_data = data;
	ext_io_w(LC8670_PORT1, ((REG_P1 | (m_p1_data & REG_P1FCR)) & REG_P1DDR) | (ext_io_r(LC8670_PORT1) & (REG_P1DDR ^ 0xff)));
}

static void set_pc(uint16_t new_pc)
{
	m_pc = new_pc;
}

static void push(uint8_t data)
{
	REG_SP++;
	m_mram[REG_SP] = data;
}

static uint8_t pop()
{
	uint8_t data =  m_mram[REG_SP];
	REG_SP--;
	return data;
}

static uint16_t get_addr()
{
	int mode = m_op & 0x0f;
	uint16_t addr;

	if (mode > 0x01 && mode <= 0x03)
		addr = GET_D9;
	else if (mode > 0x03 && mode <= 0x07)
		addr = read_data(GET_RI | ((REG_PSW>>1) & 0x0c)) | ((GET_RI & 0x02) ? 0x100 : 0x00);
	else
		fatalerror("%s: invalid get_addr in mode %x\n", machine_describe_context(), mode);

	return addr;
}

static uint8_t get_data()
{
	int mode = m_op & 0x0f;
	uint8_t data;

	if (mode == 0x01)
		data = GET_I8;
	else
		data = read_data(get_addr());

	return data;
}

static void change_clock_source()
{
	uint32_t new_clock_rate;

	switch(REG_OCR & 0x30)
	{
		case 0x00:
			new_clock_rate = 600*1000;
			break;
		case 0x20:
			new_clock_rate = 32768;
			break;
		case 0x10:
		case 0x30:
			new_clock_rate = 6*1000*1000;
			break;
	}
	if (REG_OCR & 0x80)
		new_clock_rate /= 6;
	else
		new_clock_rate /= 12;
	
	ext_set_cpu_freq(new_clock_rate);
	timer_adust(CLOCK_TIMER, new_clock_rate);
	m_clock_changed = false;
}

static void check_p_flag()
{
	uint8_t p_plag = 0;
	for(int i=0; i<8; i++)
		p_plag ^= BIT(REG_A, i);

	if (p_plag)
		REG_PSW |= FLAG_P;
	else
		REG_PSW &= ~FLAG_P;
}

static void check_p3int()
{
	if (REG_P3INT & 0x04)
	{
		if ((ext_io_r(LC8670_PORT3) ^ 0xff) & (REG_P3DDR ^ 0xff) & REG_P3)
		{
			REG_P3INT |= 0x02;
			if (REG_P3INT & 0x01)
				set_irq_flag(10);
		}
	}
}

static void set_irq_flag(int source)
{
	if (LOG_IRQ)    logerror("%s: set interrupt flag: %d\n", tag(), source);
	m_irq_flag |= 1<<source;
}

static int decode_op(uint8_t op)
{
	int idx;
	switch (op & 0x0f)
	{
		case 0: case 1:
			idx =  op & 0x0f;
			break;
		case 2: case 3:
			idx = 2;
			break;
		case 4: case 5: case 6: case 7:
			idx = 3;
			break;
		default:
			idx = 4;
			break;
	}

	return ((op>>4) & 0x0f) * 5 + idx;
}

//**************************************************************************
//  Opcodes
//**************************************************************************

int lc8670_cpu_device_op_nop()
{
	return 1;
}

int lc8670_cpu_device_op_br()
{
	uint8_t r8 = GET_R8;
	set_pc(m_pc + SIGNED(r8));

	return 2;
}

int lc8670_cpu_device_op_ld()
{
	REG_A = get_data();
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_call()
{
	uint16_t new_pc = GET_A12;

	push((m_pc>>0) & 0xff);
	push((m_pc>>8) & 0xff);

	set_pc((m_pc & 0xf000) | new_pc);

	return 2;
}


int lc8670_cpu_device_op_callr()
{
	uint16_t r16 = fetch();
	r16 |= fetch()<<8;

	push((m_pc>>0) & 0xff);
	push((m_pc>>8) & 0xff);
	set_pc(m_pc - 1 + r16);

	return 4;
}

int lc8670_cpu_device_op_brf()
{
	uint16_t r16 = fetch();
	r16 |= fetch()<<8;
	set_pc(m_pc - 1 + r16);

	return 4;
}

int lc8670_cpu_device_op_st()
{
	write_data(get_addr(), REG_A);

	return 1;
}

int lc8670_cpu_device_op_callf()
{
	uint16_t a16 = fetch()<<8;
	a16 |= fetch();

	push((m_pc>>0) & 0xff);
	push((m_pc>>8) & 0xff);
	set_pc(a16);

	return 2;
}

int lc8670_cpu_device_op_jmpf()
{
	uint16_t a16 = fetch()<<8;
	a16 |= fetch();
	set_pc(a16);

	ext_code_bankswitch(((REG_EXT & 0x01) ? 1 : (REG_EXT & 0x08) ? 0 : 2));

	return 2;
}

int lc8670_cpu_device_op_mov()
{
	uint16_t addr = get_addr();
	uint8_t i8 = GET_I8;
	write_data(addr, i8);

	return 1;
}

int lc8670_cpu_device_op_jmp()
{
	uint16_t new_pc = GET_A12;
	set_pc((m_pc & 0xf000) | new_pc);

	return 2;
}

int lc8670_cpu_device_op_mul()
{
	uint32_t res = REG_B * ((REG_A<<8) | REG_C);

	REG_A = (res>>8) & 0xff;
	REG_B = (res>>16) & 0xff;
	REG_C = (res>>0) & 0xff;

	SET_OV(REG_B != 0 ? 1 : 0);
	SET_CY(0);
	CHECK_P();

	return 7;
}

int lc8670_cpu_device_op_be()
{
	uint8_t data = get_data();
	uint8_t r8 = GET_R8;

	if (REG_A == data)
		set_pc(m_pc + SIGNED(r8));

	SET_CY((REG_A < data) ? 1 : 0);

	return 2;
}

int lc8670_cpu_device_op_be_ri()
{
	uint8_t data = get_data();
	uint8_t i8 = GET_I8;
	uint8_t r8 = GET_R8;

	if (i8 == data)
		set_pc(m_pc + SIGNED(r8));

	SET_CY((data < i8) ? 1 : 0);

	return 2;
}


int lc8670_cpu_device_op_div()
{
	uint32_t res, mod;

	if (REG_B != 0)
	{
		uint16_t v = ((REG_A<<8) | REG_C);
		res = v / REG_B;
		mod = v % REG_B;

		REG_A = (res>>8) & 0xff;
		REG_C = (res>>0) & 0xff;
		REG_B = mod & 0xff;
		SET_OV(0);
	}
	else
	{
		REG_A = 0xff;
		SET_OV(1);
	}

	SET_CY(0);
	CHECK_P();

	return 7;
}

int lc8670_cpu_device_op_bne()
{
	uint8_t data = get_data();
	uint8_t r8 = GET_R8;

	if (REG_A != data)
		set_pc(m_pc + SIGNED(r8));

	SET_CY((REG_A < data) ? 1 : 0);

	return 2;
}

int lc8670_cpu_device_op_bne_ri()
{
	uint8_t data = get_data();
	uint8_t i8 = GET_I8;
	uint8_t r8 = GET_R8;

	if (i8 != data)
		set_pc(m_pc + SIGNED(r8));

	SET_CY((data < i8) ? 1 : 0);

	return 2;
}

#define QUIET		01
#define DEBUG		01

int lc8670_cpu_device_op_ldf()
{
	uint16_t addr = REG_TRL | (REG_TRH<<8);

//	if (!QUIET)
//		fprintf(stderr, "LDF 0x%04X with FPR $%02X\n", addr, REG_FPR);

	ext_code_bankswitch(REG_FPR & 0x01 ? 2 : 1);
	REG_A = ext_read_code(addr);
	CHECK_P();
	ext_code_bankswitch(((REG_EXT & 0x01) ? 1 : (REG_EXT & 0x08) ? 0 : 2));

	return 2;
}

int lc8670_cpu_device_op_stf()
{
	//STF can only be used form ROM, so we only support it there
	
	if (ext_cpu_is_running_in_rom()) {
		
		uint16_t addr = REG_TRL | (REG_TRH<<8);
		uint32_t addrLong = ((((uint32_t)(REG_FPR)) & 1) << 16) + addr;
		uint8_t val = REG_A;
		
		if (!QUIET)
			fprintf(stderr, "**TRH IN ROM ** $%02x -> [$%04X]     FLASHCTRL=$%02x seq=%3d\n", val, addr, REG_FPR, m_flash_unlock_seq);
		
		if (REG_FPR & 0x02) {		//unlock sequence only
			
			switch (m_flash_unlock_seq) {
				case 0:
					m_flash_unlock_seq = (addr == 0x5555 && val == 0xAA) ? 1 : 0;
					break;
				case 1:
					m_flash_unlock_seq = (addr == 0x2AAA && val == 0x55) ? 2 : 0;
					break;
				case 2:
					m_flash_unlock_seq = (addr == 0x5555 && val == 0xA0) ? 3 : 0;
					break;
				default:
					m_flash_unlock_seq = 0;
					break;
			}
		}
		else if (m_flash_unlock_seq < 3) {	//no full unlock -> not unlocked
			
			fprintf(stderr, "unlock never finished for STF\n");
			m_flash_unlock_seq = 0;
		}
		else if (REG_FPR & 0x02) {			//another STF after unlock with bit1 set -> sequence fail
				
			fprintf(stderr, "post-unlock write with FLASHCTRL & 2\n");
			m_flash_unlock_seq = 0;
		}
		else if (m_flash_unlock_seq == 3 && (addrLong & 127)) {
			
			fprintf(stderr, "not row-aligned flash write @ 0x%05X\n", addrLong);
			m_flash_unlock_seq = 0;
		}
		else  {								//probably valid STF
			
			if (m_flash_unlock_seq == 3) {
				
				fprintf(stderr, "*FLASH WRITE to 0x%05X\n", addrLong);
			}
			
			ext_write_flash_direct(addrLong, &val, 1);
			m_flash_unlock_seq++;
			
			if (m_flash_unlock_seq == 128 + 3)
				m_flash_unlock_seq = 0;
		}
		
		return 2;
	}
	
	//in flash, stf is for debugging for us
	
	if (DEBUG) {			//DEBUG code for C-M23 emulator
	
		static bool firstInstrAfterInit = true;
		static uint8_t maxStack = 0, lastStack;
		const uint8_t stackSentinel = 0x5A, stackStart = 0xC3;
		uint8_t typ = fetch();
		uint32_t i;
		
		switch(typ) {
			case 1:		//before fetch
				
				if (QUIET)
					break;
				
				//analyze stack
				if (REG_SP != stackStart - 1)
					fprintf(stderr, "STACK POINTER INVALID. Expected $%02X, seen $%02X\n", stackStart - 1, REG_SP);
				
				for (i = stackStart; i <= 0x100 && m_mram[i] != stackSentinel; i++);
				lastStack = i - stackStart;
				if (i > 0x100)
					lastStack = 0xFF;		//overflow!
			
				if (firstInstrAfterInit)
					lastStack = 0;
				else if (lastStack > maxStack)
					maxStack = lastStack;
			
				firstInstrAfterInit = false;
				fprintf(stderr, "\n");
				for (i = 0; i < 8; i++) {
					fprintf(stderr, "R%02u = 0x%08X   R%02u = 0x%08X\n",
						i + 0, *(uint32_t*)(m_mram + 0x80 + (i + 0) * 4),
						i + 8, *(uint32_t*)(m_mram + 0x80 + (i + 8) * 4));
				}
				fprintf(stderr, "ARM SR: %c%c%c%c  SP {now=$%02X last=$%02X max=$%02X}\n",
					(m_mram[0xC0] & 0x40) ? 'N' : ' ',
					(m_mram[0xC0] & 0x20) ? 'Z' : ' ',
					(m_mram[0xC0] & 0x80) ? 'C' : ' ',
					(m_mram[0xC0] & 0x10) ? 'V' : ' ',
					REG_SP, lastStack, maxStack);
				fprintf(stderr, "IRQC: S=0x%04X E=0x%04X\n",
					*(uint16_t*)(m_mram + 0xFE),
					*(uint16_t*)(m_mram + 0xC1));
				
				//reset stack sentinel
				for (i = stackStart; i < 0x100; i++)
					m_mram[i] = stackSentinel;
				
				break;
			case 2:		//after 16-bit fetch
			
				if (QUIET)
					break;
				
				fprintf(stderr, "instr 0x%04X	[ENTER]\n", *(uint16_t*)(m_mram + 4));
				getchar();
				break;
			case 3:		//after 32-bit fetch
				
				if (QUIET)
					break;
				
				fprintf(stderr, "instr 0x%04X 0x%04X	[ENTER]\n", *(uint16_t*)(m_mram + 4), *(uint16_t*)(m_mram + 6));
				getchar();
				break;
			
			case 4:		//print R0 as char
				fprintf(stderr, "%c", m_mram[0x80]);
				break;
			
			case 5:		//print R0 as hex
				fprintf(stderr, "0x%08X\n", *(uint32_t*)(m_mram + 0x80));
				break;
			
			case 0xF0:	//annotate readmem
				
				if (QUIET)
					break;
				
				fprintf(stderr, "READMEM ADDR=*$%02X", REG_A);
				if (!(REG_A & 3) && (REG_A >= 0x80 && REG_A < 0xC0))
					fprintf(stderr,"(ARM R%02u)", (REG_A - 0x80) / 4);
				else if (REG_A == 4)
					fprintf(stderr,"(WORDVAL)");
				fprintf(stderr, "=0x%08X, %u bytes, DST=$%02X", *(uint32_t*)(m_mram + REG_A), REG_B, REG_C);
				if (!(REG_C & 3) && (REG_C >= 0x80 && REG_C < 0xC0))
					fprintf(stderr,"(ARM R%02u)", (REG_C - 0x80) / 4);
				else if (REG_C == 4)
					fprintf(stderr,"(WORDVAL)");
				
				fprintf(stderr, "\n");
				break;
			
			case 0xF1:	//annotate writemem
				
				if (QUIET)
					break;
				
				
				fprintf(stderr, "WRITEMEM ADDR=*$%02X", REG_A);
				if (!(REG_A & 3) && (REG_A >= 0x80 && REG_A < 0xC0))
					fprintf(stderr,"(ARM R%02u)", (REG_A - 0x80) / 4);
				else if (REG_A == 4)
					fprintf(stderr,"(WORDVAL)");
				fprintf(stderr, "=0x%08X, %u bytes, SRC=$%02X", *(uint32_t*)(m_mram + REG_A), REG_B, REG_C);
				if (!(REG_C & 3) && (REG_C >= 0x80 && REG_C < 0xC0))
					fprintf(stderr,"(ARM R%02u)", (REG_C - 0x80) / 4);
				else if (REG_C == 4)
					fprintf(stderr,"(WORDVAL)");
				
				switch(REG_B) {
					case 1:
						fprintf(stderr, " (0x%02X)", *(uint8_t*)(m_mram + REG_C));
						break;
					case 2:
						fprintf(stderr, " (0x%04X)", *(uint16_t*)(m_mram + REG_C));
						break;
					case 4:
						fprintf(stderr, " (0x%08X)", *(uint32_t*)(m_mram + REG_C));
						break;
				}
				
				fprintf(stderr, "\n");
				break;
			
			case 0xFE:	//named tracepoi t
				fprintf(stderr, "TRACEPOINT $%02X\n", fetch());
				//fallthrough
			
			case 0xFF:	//generic TRACEPOINT show wordval, A, B, C, Rx
				fprintf(stderr, "WORDVAL=0x%08X A=$%02X B=$%02X C=$%02X\n",
					*(uint32_t*)(m_mram + 4), REG_A, REG_B, REG_C);
				fprintf(stderr, "bank 0 ptrs: R0=$%02X R1=$%02X R2=$%02X R3=$%02X\n",
					m_mram[0], m_mram[1], m_mram[2], m_mram[3]);
				fprintf(stderr, "bank 2 ptrs: R0=$%02X R1=$%02X R2=$%02X R3=$%02X\n",
					m_mram[0x100], m_mram[0x101], m_mram[0x102], m_mram[0x103]);
				fprintf(stderr, "FLASHCTRL=$%02X TRH=$%02X TRL=$%02X\n", REG_FPR, REG_TRH, REG_TRL);
				for (i = 0; i < 8; i++) {
					fprintf(stderr, "R%02u = 0x%08X   R%02u = 0x%08X\n",
						i + 0, *(uint32_t*)(m_mram + 0x80 + (i + 0) * 4),
						i + 8, *(uint32_t*)(m_mram + 0x80 + (i + 8) * 4));
				}
				break;
			
			default:
				fprintf(stderr, "unknown debug code 0x%02X   [ENTER]\n", typ);
				getchar();
				break;
		}
				
		return 0;
	}
	
	return 2;
}

int lc8670_cpu_device_op_dbnz()
{
	uint16_t addr = get_addr();
	uint8_t r8 = GET_R8;
	uint8_t data = read_data_latch(addr) - 1;

	write_data_latch(addr, data);

	if (data != 0)
		set_pc(m_pc + SIGNED(r8));

	return 2;
}

int lc8670_cpu_device_op_bpc()
{
	uint8_t b3 = GET_B3;
	uint16_t d9 = GET_D9B3;
	uint8_t r8 = GET_R8;
	uint8_t data = read_data_latch(d9);

	if (data & (1<<b3))
	{
		write_data_latch(d9, data & ~(1<<b3));
		set_pc(m_pc + SIGNED(r8));
	}

	return 2;
}

int lc8670_cpu_device_op_push()
{
	uint16_t d9 = GET_D9;
	push(read_data(d9));

	return 2;
}

int lc8670_cpu_device_op_inc()
{
	uint16_t addr = get_addr();
	uint8_t data = read_data_latch(addr);

	write_data_latch(addr, data + 1);

	return 1;
}

int lc8670_cpu_device_op_bp()
{
	uint8_t b3 = GET_B3;
	uint16_t d9 = GET_D9B3;
	uint8_t r8 = GET_R8;

	if (read_data(d9) & (1<<b3))
		set_pc(m_pc + SIGNED(r8));

	return 2;
}

int lc8670_cpu_device_op_pop()
{
	uint16_t d9 = GET_D9;
	write_data(d9, pop());

	return 2;
}

int lc8670_cpu_device_op_dec()
{
	uint16_t addr = get_addr();
	uint8_t data = read_data_latch(addr);

	write_data_latch(addr, data - 1);

	return 1;
}

int lc8670_cpu_device_op_bz()
{
	uint8_t r8 = GET_R8;

	if (REG_A == 0)
		set_pc(m_pc + SIGNED(r8));

	return 2;
}

int lc8670_cpu_device_op_add()
{
	uint8_t data = get_data();
	int32_t res = (REG_A + data);

	SET_CY(res > 0xff ? 1 : 0);
	SET_AC(((REG_A & 0x0f) + (data & 0x0f)) > 0x0f ? 1 : 0);
	SET_OV((REG_A & data) & (data ^ res) & 0x80 ? 1 : 0);

	REG_A = res & 0xff;
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_bn()
{
	uint8_t b3 = GET_B3;
	uint16_t d9 = GET_D9B3;
	uint8_t r8 = GET_R8;

	if (!(read_data(d9) & (1<<b3)))
		set_pc(m_pc + SIGNED(r8));

	return 2;
}

int lc8670_cpu_device_op_bnz()
{
	uint8_t r8 = GET_R8;

	if (REG_A != 0)
		set_pc(m_pc + SIGNED(r8));

	return 2;
}

int lc8670_cpu_device_op_addc()
{
	uint8_t data = get_data();
	int32_t res = (REG_A + data + GET_CY);

	SET_CY(res > 0xff ? 1 : 0);
	SET_AC(((REG_A & 0x0f) + (data & 0x0f) + GET_CY) > 0x0f ? 1 : 0);
	SET_OV(((REG_A+GET_CY) & data) & (data ^ res) & 0x80 ? 1 : 0);

	REG_A = res & 0xff;
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_ret()
{
	uint16_t new_pc = pop()<<8;
	new_pc |= pop();
	set_pc(new_pc);

	return 2;
}

int lc8670_cpu_device_op_sub()
{
	uint8_t data = get_data();
	int32_t res = (REG_A - data);

	SET_CY(res < 0x00 ? 1 : 0);
	SET_AC(((REG_A & 0x0f) - (data & 0x0f)) < 0x00 ? 1 : 0);
	SET_OV((REG_A ^ data) & (data & res) & 0x80 ? 1 : 0);

	REG_A = res & 0xff;
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_not1()
{
	uint16_t d9 = GET_D9B3;
	uint8_t data = read_data_latch(d9);

	data ^= (1<<GET_B3);
	write_data_latch(d9, data);

	return 1;
}

int lc8670_cpu_device_op_reti()
{
	uint16_t new_pc = pop()<<8;
	new_pc |= pop();
	set_pc(new_pc);

	if (LOG_IRQ)    logerror("%s: RETI from level %d\n", machine_describe_context(), m_irq_lev);
	for(int i=2; i>=0; i--)
		if (m_irq_lev & (1<<i))
		{
			m_irq_lev &= ~(1<<i);
			break;
		}

	m_after_reti = true;

	return 2;
}

int lc8670_cpu_device_op_subc()
{
	uint8_t data = get_data();
	int32_t res = (REG_A - data - GET_CY);

	SET_CY(res < 0x00 ? 1 : 0);
	SET_AC(((REG_A & 0x0f) - (data & 0x0f) - GET_CY) < 0x00 ? 1 : 0);
	SET_OV((REG_A ^ (data + GET_CY)) & (data & res) & 0x80 ? 1 : 0);

	REG_A = res & 0xff;
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_ror()
{
	REG_A = ((REG_A & 0x01) << 7) | (REG_A>>1);
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_ldc()
{
	REG_A = ext_read_code(((REG_TRH<<8) | REG_TRL) + REG_A);
	CHECK_P();

	return 2;
}

int lc8670_cpu_device_op_xch()
{
	uint16_t addr = get_addr();
	uint8_t data = read_data(addr);

	write_data(addr, REG_A);
	REG_A = data;
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_clr1()
{
	uint16_t d9 = GET_D9B3;
	uint8_t data = read_data_latch(d9);

	data &= ~(1<<GET_B3);
	write_data_latch(d9, data);

	return 1;
}

int lc8670_cpu_device_op_rorc()
{
	uint8_t a = (REG_A>>1) | (GET_CY ? 0x80 : 0x00);

	SET_CY(BIT(REG_A,0));
	REG_A = a;
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_or()
{
	REG_A = REG_A | get_data();
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_rol()
{
	REG_A = ((REG_A & 0x80) >> 7) | (REG_A<<1);
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_and()
{
	REG_A = REG_A & get_data();
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_set1()
{
	uint16_t d9 = GET_D9B3;
	uint8_t data = read_data_latch(d9);

	data |= (1<<GET_B3);
	write_data_latch(d9, data);

	return 1;
}

int lc8670_cpu_device_op_rolc()
{
	uint8_t a = (REG_A<<1) | (GET_CY ? 0x01 : 0x00);

	SET_CY(BIT(REG_A,7));
	REG_A = a;
	CHECK_P();

	return 1;
}

int lc8670_cpu_device_op_xor()
{
	REG_A = REG_A ^ get_data();
	CHECK_P();

	return 1;
}


//**************************************************************************
//  Opcodes Table
//**************************************************************************

static const lc8670_cpu_device_op_handler s_opcode_table[] =
{
	&lc8670_cpu_device_op_nop  , &lc8670_cpu_device_op_br  , &lc8670_cpu_device_op_ld  , &lc8670_cpu_device_op_ld  , &lc8670_cpu_device_op_call,   // 0x0*
	&lc8670_cpu_device_op_callr, &lc8670_cpu_device_op_brf , &lc8670_cpu_device_op_st  , &lc8670_cpu_device_op_st  , &lc8670_cpu_device_op_call,   // 0x1*
	&lc8670_cpu_device_op_callf, &lc8670_cpu_device_op_jmpf, &lc8670_cpu_device_op_mov , &lc8670_cpu_device_op_mov , &lc8670_cpu_device_op_jmp,    // 0x2*
	&lc8670_cpu_device_op_mul  , &lc8670_cpu_device_op_be  , &lc8670_cpu_device_op_be  , &lc8670_cpu_device_op_be_ri, &lc8670_cpu_device_op_jmp,   // 0x3*
	&lc8670_cpu_device_op_div  , &lc8670_cpu_device_op_bne , &lc8670_cpu_device_op_bne , &lc8670_cpu_device_op_bne_ri, &lc8670_cpu_device_op_bpc,  // 0x4*
	&lc8670_cpu_device_op_ldf  , &lc8670_cpu_device_op_stf , &lc8670_cpu_device_op_dbnz, &lc8670_cpu_device_op_dbnz, &lc8670_cpu_device_op_bpc,    // 0x5*
	&lc8670_cpu_device_op_push , &lc8670_cpu_device_op_push, &lc8670_cpu_device_op_inc , &lc8670_cpu_device_op_inc , &lc8670_cpu_device_op_bp,     // 0x6*
	&lc8670_cpu_device_op_pop  , &lc8670_cpu_device_op_pop , &lc8670_cpu_device_op_dec , &lc8670_cpu_device_op_dec , &lc8670_cpu_device_op_bp,     // 0x7*
	&lc8670_cpu_device_op_bz   , &lc8670_cpu_device_op_add , &lc8670_cpu_device_op_add , &lc8670_cpu_device_op_add , &lc8670_cpu_device_op_bn,     // 0x8*
	&lc8670_cpu_device_op_bnz  , &lc8670_cpu_device_op_addc, &lc8670_cpu_device_op_addc, &lc8670_cpu_device_op_addc, &lc8670_cpu_device_op_bn,     // 0x9*
	&lc8670_cpu_device_op_ret  , &lc8670_cpu_device_op_sub , &lc8670_cpu_device_op_sub , &lc8670_cpu_device_op_sub , &lc8670_cpu_device_op_not1,   // 0xa*
	&lc8670_cpu_device_op_reti , &lc8670_cpu_device_op_subc, &lc8670_cpu_device_op_subc, &lc8670_cpu_device_op_subc, &lc8670_cpu_device_op_not1,   // 0xb*
	&lc8670_cpu_device_op_ror  , &lc8670_cpu_device_op_ldc , &lc8670_cpu_device_op_xch , &lc8670_cpu_device_op_xch , &lc8670_cpu_device_op_clr1,   // 0xc*
	&lc8670_cpu_device_op_rorc , &lc8670_cpu_device_op_or  , &lc8670_cpu_device_op_or  , &lc8670_cpu_device_op_or  , &lc8670_cpu_device_op_clr1,   // 0xd*
	&lc8670_cpu_device_op_rol  , &lc8670_cpu_device_op_and , &lc8670_cpu_device_op_and , &lc8670_cpu_device_op_and , &lc8670_cpu_device_op_set1,   // 0xe*
	&lc8670_cpu_device_op_rolc , &lc8670_cpu_device_op_xor , &lc8670_cpu_device_op_xor , &lc8670_cpu_device_op_xor , &lc8670_cpu_device_op_set1,   // 0xf*
};


