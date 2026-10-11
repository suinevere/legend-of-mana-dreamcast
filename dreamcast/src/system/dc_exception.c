/*----------------------
 | dc_exception.c
 | Description: Installs an SH-4 exception trap that reports the fault and halts, for address-error and illegal-instruction exceptions.
 | Author: suinevere
 | Dependencies: dc_exception.h, dc_system.h, dc_exception_format.h, arch/irq.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
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
 | Description: Formats the faulting exception's name, PC, PR and address, then halts.
 | Author: suinevere
 | Dependencies: dc_exception_format_halt, dc_halt
 | Globals: N/A
 | Params: source -- exception code that fired, context -- CPU state at the fault, data -- unused
 | Returns: N/A
 ----------------------*/
static void dc_exception_handler(irq_t source, irq_context_t *context, void *data)
{
    const char *name;
    char message[64];
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

    dc_exception_format_halt(message, sizeof message, name, context->pc, context->pr, address);
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
