/*----------------------
 | launch.c
 | Description: OpenPSK main replacement that maps the first PocketStation file on the card as FLASH1 and enters it in user mode.
 | Author: suinevere
 | Dependencies: openpsk/hardware.h, openpsk/kernel_state.h, openpsk/com.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/

#include "com.h"
#include "hardware.h"
#include "kernel_state.h"

/*----------------------
 | LAUNCH_FIRST_BLOCK
 | Description: Directory block holding the application file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LAUNCH_FIRST_BLOCK 1u

/*----------------------
 | LAUNCH_BANK_MAP
 | Description: Base of the 16 physical-to-virtual flash bank mapping registers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LAUNCH_BANK_MAP 0x06000100u

/*----------------------
 | LAUNCH_BANK_FLG
 | Description: Flash bank enable register (one bit per physical bank).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LAUNCH_BANK_FLG 0x06000008u

/*----------------------
 | LAUNCH_FIQ_SLOT
 | Description: WRAM word the FIQ vector trampoline loads its handler address from.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LAUNCH_FIQ_SLOT 0x3Cu

void launch_enter(unsigned entry) __attribute__((noreturn));
void launch_fiq_service(void);
void launch_swi_service(void);

/*----------------------
 | LAUNCH_SWI_SLOT
 | Description: WRAM word the SWI vector trampoline loads its service routine address from.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LAUNCH_SWI_SLOT 0x28u

void swi_handler_reset(void);
void swi_handler_set_user_callback(void);
void swi_handler_invoke_callback(void);
void swi_handler_set_clock_freq(void);
void swi_handler_default(void);
void swi_handler_get_pda_status(void);
void swi_handler_set_comflags_internal(void);
void swi_handler_app_block_control(void);
void swi_handler_read_serial(void);
void swi_handler_terminate_file_transfer(void);
void swi_handler_set_bcd_date_time(void);
void swi_handler_get_bcd_date(void);
void swi_handler_get_bcd_time(void);
void swi_handler_write_serial(void);
void swi_handler_control_ps_comm(void);
void swi_handler_get_ui_status(void);
void swi_handler_get_syscall_table(void);
void swi_handler_gated_select_byte(void);
void swi_handler_get_app_block(void);
void swi_handler_get_status_ptr(void);
void swi_handler_get_app_flag(void);
void swi_handler_magic(void);
void swi_handler_xform(void);
unsigned swi_handler_write_flash_absolute(unsigned dest, unsigned src);
unsigned launch_swi_flash_write_virtual(unsigned sector, unsigned src);

/*----------------------
 | launch_swi_table
 | Description: OpenPSK's SWI table with SWI 03h replaced by the psx-spx FlashWriteVirtual(sector, src) the app calls.
 | Author: suinevere
 | Dependencies: openpsk swi.S, swi_handlers.c, syscalls.c, com.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void (*const launch_swi_table[32])(void) = {
    swi_handler_reset,
    swi_handler_set_user_callback,
    swi_handler_invoke_callback,
    (void (*)(void))launch_swi_flash_write_virtual,
    swi_handler_set_clock_freq,
    swi_handler_default,
    swi_handler_get_pda_status,
    swi_handler_set_comflags_internal,
    swi_handler_app_block_control,
    swi_handler_default,
    swi_handler_read_serial,
    swi_handler_terminate_file_transfer,
    swi_handler_set_bcd_date_time,
    swi_handler_get_bcd_date,
    swi_handler_get_bcd_time,
    swi_handler_write_serial,
    (void (*)(void))swi_handler_write_flash_absolute,
    swi_handler_control_ps_comm,
    swi_handler_default,
    swi_handler_get_ui_status,
    swi_handler_get_syscall_table,
    swi_handler_gated_select_byte,
    swi_handler_get_app_block,
    swi_handler_get_status_ptr,
    swi_handler_get_app_flag,
    swi_handler_default,
    swi_handler_magic,
    swi_handler_xform,
    swi_handler_default,
    swi_handler_default,
    swi_handler_default,
    swi_handler_default,
};

/*----------------------
 | launch_swi_flash_write_virtual
 | Description: SWI 03h: writes 128 bytes at src to virtual sector number sector of FLASH1, translated through the bank mapping.
 | Author: suinevere
 | Dependencies: swi_handlers.c
 | Globals: N/A
 | Params: sector -- virtual sector 0..3FFh from 0x02000000; src -- halfword-aligned RAM source
 | Returns: 0 okay, 1 failed or sector outside the mapped file
 ----------------------*/
unsigned launch_swi_flash_write_virtual(unsigned sector, unsigned src) {
    unsigned off = sector * 0x80u;
    unsigned enabled = PSK_MMIO(LAUNCH_BANK_FLG);
    if (sector > 0x3FFu) return 1;
    for (unsigned p = 0; p < 16; p++) {
        if (((enabled >> p) & 1u) && (PSK_MMIO(LAUNCH_BANK_MAP + p * 4u) & 0xFu) == off / 0x2000u) {
            return swi_handler_write_flash_absolute(FLASH_BASE_ABS + p * 0x2000u + (off & 0x1FFFu), src);
        }
    }
    return 1;
}

__asm__(
    "    .text\n"
    "    .arm\n"
    "    .global launch_enter\n"
    "launch_enter:\n"
    "    mov     sp, #0x800\n"
    "    msr     cpsr_c, #0x10\n"
    "    bx      r0\n"
    "    .global launch_swi_service\n"
    "launch_swi_service:\n"
    "    stmfd   sp!, {r1, r2, r3, r12, lr}\n"
    "    mrs     r12, spsr\n"
    "    tst     r12, #0x20\n"
    "    subne   r12, lr, #2\n"
    "    subeq   r12, lr, #4\n"
    "    ldrh    r12, [r12]\n"
    "    and     r12, r12, #0xFF\n"
    "    mov     lr, #0xE0\n"
    "    ldr     lr, [lr]\n"
    "    ldr     r12, [lr, r12, lsl #2]\n"
    "    adr     lr, 3f\n"
    "    bx      r12\n"
    "3:  ldmfd   sp!, {r1, r2, r3, r12, pc}^\n"
    "    .global launch_fiq_service\n"
    "launch_fiq_service:\n"
    "    sub     lr, lr, #4\n"
    "    stmfd   sp!, {r0, r1, r2, r3, r12, lr}\n"
    "    ldr     r0, =0x0A000000\n"
    "    ldr     r8, [r0]\n"
    "    ldr     r0, =0x0A000008\n"
    "    ldr     r0, [r0]\n"
    "    and     r8, r8, r0\n"
    "    tst     r8, #0x40\n"
    "    blne    openpsk_com_service\n"
    "    bics    r8, r8, #0x40\n"
    "    beq     2f\n"
    "    mov     r12, #0x100\n"
    "    ldr     r12, [r12]\n"
    "    cmp     r12, #0\n"
    "    bne     1f\n"
    "    ldr     r0, =0x0A000010\n"
    "    str     r8, [r0]\n"
    "    b       2f\n"
    "1:  adr     lr, 2f\n"
    "    bx      r12\n"
    "2:  ldmfd   sp!, {r0, r1, r2, r3, r12, pc}^\n"
    "    .ltorg\n"
    "    .balign 4\n"
    "    .global launch_runner_block\n"
    "launch_runner_block:\n"
    "    .word   0x4B53504C\n"
    "4:  b       4b\n"
    "    .irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31\n"
    "    swi     #\\n\n"
    "    bx      lr\n"
    "    .endr\n");

/*----------------------
 | map_app
 | Description: Follows the directory chain from the app's first block and maps those physical banks to virtual banks 0.. at FLASH1.
 | Author: suinevere
 | Dependencies: hardware.h
 | Globals: N/A
 | Params: first -- first directory block of the file
 | Returns: N/A
 ----------------------*/
static void map_app(unsigned first) {
    unsigned virt[16];
    unsigned used = 0;
    unsigned next = 0;
    for (unsigned i = 0; i < 16; i++) virt[i] = 0xFFu;
    for (unsigned b = first, n = 0; b >= 1 && b <= 15 && n < 15; n++) {
        virt[b] = next++;
        used |= 1u << b;
        unsigned link = PSK_MMIO16(FLASH_BASE_ABS + b * 0x80u + 8u);
        if (link == 0xFFFFu) break;
        b = link + 1u;
    }
    for (unsigned i = 0; i < 16; i++) {
        if (virt[i] == 0xFFu) virt[i] = next++;
        PSK_MMIO(LAUNCH_BANK_MAP + i * 4u) = virt[i];
    }
    PSK_MMIO(LAUNCH_BANK_FLG) = used;
}

/*----------------------
 | main
 | Description: Brings the LCD and wake interrupts up as the demo does, consumes the docking edge as the GUI would, maps the app and enters it.
 | Author: suinevere
 | Dependencies: hardware.h, kernel_state.h, com.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void main(void) {
    psk_lcd_enable();
    PSK_RAM32(LAUNCH_FIQ_SLOT) = (unsigned)launch_fiq_service;
    PSK_RAM32(PSK_SWI_TABLE_PTR) = (unsigned)launch_swi_table;
    PSK_RAM32(LAUNCH_SWI_SLOT) = (unsigned)launch_swi_service;
    psk_int_unmask(INT_RTC | INT_DOCKED);
    openpsk_dock_service();
    psk_comflags_clr(PSK_CF_INSERTED);
    map_app(LAUNCH_FIRST_BLOCK);
    PSK_RAM16(PSK_CUR_DIR_INDEX) = LAUNCH_FIRST_BLOCK;
    PSK_RAM16(PSK_APP_BLOCK) = LAUNCH_FIRST_BLOCK;
    launch_enter(PSK_MMIO(FLASH_BASE_REL + 0x5Cu));
}
