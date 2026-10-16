/*----------------------
 | overlay_loader_dc.h
 | Description: Loads an overlay's OVLnnC.BIN code and OVLnn.BIN data from the disc into its fixed region of the overlay arena, then zeroes its .bss.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_OVERLAY_LOADER_DC_H
#define LOM_OVERLAY_LOADER_DC_H

/*----------------------
 | lom_overlay_arena
 | Description: The 10 MB NOLOAD overlay arena dreamcast.ld places at 0x8C400000.
 | Author: suinevere
 | Dependencies: dreamcast.ld
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern unsigned char lom_overlay_arena[];

/*----------------------
 | overlay_loader_init
 | Description: Checks that the linked overlay arena sits where the generated regions expect it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void overlay_loader_init(void);

/*----------------------
 | load_overlay
 | Description: Loads an overlay's code and data into its arena regions and zeroes its .bss; halts naming the overlay when either does not fit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: resource_index -- PS1 CD resource index of the overlay
 | Returns: N/A
 ----------------------*/
void load_overlay(int resource_index);

#endif
