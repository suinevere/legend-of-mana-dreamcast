/*----------------------
 | wmap_controller_ports.c
 | Description: Defines WMAP's controller-port pointer in C so it points at main's controller state, which the PS1 data holds as a raw main-memory address.
 | Author: suinevere
 | Dependencies: lom_prefix.h (forced include), main/controller_internal.h
 | Globals: g_wmap_controller_ports
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "main/controller_internal.h"

/*----------------------
 | g_wmap_controller_ports
 | Description: Port states WMAP reads its buttons from, the same CONTROLLER_STATE_ADDRESS the PS1 data stores.
 | Author: suinevere
 | Dependencies: CONTROLLER_STATE_ADDRESS
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
ControllerPortState *g_wmap_controller_ports = (ControllerPortState *)CONTROLLER_STATE_ADDRESS;
