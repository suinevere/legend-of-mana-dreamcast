/*----------------------
 | dc_exception.h
 | Description: Declares installation of the SH-4 exception trap used to halt on address-error and illegal-instruction faults.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef DC_EXCEPTION_H
#define DC_EXCEPTION_H

/*----------------------
 | dc_exception_install
 | Description: Installs handlers for the SH-4 address-error, illegal-instruction and slot-illegal exceptions.
 | Author: suinevere
 | Dependencies: irq_set_handler
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void dc_exception_install(void);

#endif
