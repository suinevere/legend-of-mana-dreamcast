/*----------------------
 | rumble_dc.h
 | Description: Drives the Jump Pack on each controller's Maple port from the game's per-port vibration level.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_RUMBLE_DC_H
#define LOM_RUMBLE_DC_H

void rumble_dc_update(int port, int level);

#endif
