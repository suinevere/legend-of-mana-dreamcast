/*----------------------
 | dc_exception.c
 | Description: Installs an SH-4 exception trap that reports the fault and halts, for address-error and illegal-instruction exceptions.
 | Author: suinevere
 | Dependencies: dc_exception.h, dc_system.h, dc_exception_format.h, arch/irq.h, stdio.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <arch/irq.h>
#include "dc_exception.h"
#include "dc_exception_format.h"
#include "dc_system.h"

/*----------------------
 | DC_EXCEPTION_TEA_ADDR
 | Description: Memory-mapped address of the SH-4 MMU's TEA register, holding the faulting address for address-error exceptions.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DC_EXCEPTION_TEA_ADDR 0xFF00000Cul

/*----------------------
 | dc_exception_handler
 | Description: Formats the faulting exception's name, PC, PR and address, the build's date and time, and r0..r15, then halts.
 | Author: suinevere
 | Dependencies: dc_exception_format_halt, dc_exception_format_registers, dc_halt
 | Globals: N/A
 | Params: source -- exception code that fired, context -- CPU state at the fault, data -- unused
 | Returns: N/A
 ----------------------*/
static void dc_exception_handler(irq_t source, irq_context_t *context, void *data)
{
    const char *name;
    static char head[64];
    static char registers[256];
    static char message[400];
    unsigned long r[16];
    int i;
    unsigned long address = *(volatile unsigned long *)DC_EXCEPTION_TEA_ADDR;

    (void)data;

    switch (source) {
        case EXC_DATA_ADDRESS_READ:
            name = "ADDR READ";
            break;
        case EXC_DATA_ADDRESS_WRITE:
            name = "ADDR WRITE";
            break;
        case EXC_ILLEGAL_INSTR:
            name = "ILLEGAL";
            break;
        case EXC_SLOT_ILLEGAL_INSTR:
            name = "SLOT ILLEGAL";
            break;
        default:
            name = "EXCEPTION";
            break;
    }

    for (i = 0; i < 16; i++)
        r[i] = context->r[i];
    dc_exception_format_halt(head, sizeof head, name, context->pc, context->pr, address);
    dc_exception_format_registers(registers, sizeof registers, r);
    snprintf(message, sizeof message, "%s\nBUILD " __DATE__ " " __TIME__ "\n%s", head, registers);
    dc_halt(message);
}

/*----------------------
 | dc_exception_install
 | Description: Installs handlers for the SH-4 address-error, illegal-instruction and slot-illegal exceptions.
 | Author: suinevere
 | Dependencies: irq_set_handler
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void dc_exception_install(void)
{
    irq_set_handler(EXC_DATA_ADDRESS_READ, dc_exception_handler, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_WRITE, dc_exception_handler, NULL);
    irq_set_handler(EXC_ILLEGAL_INSTR, dc_exception_handler, NULL);
    irq_set_handler(EXC_SLOT_ILLEGAL_INSTR, dc_exception_handler, NULL);
}
