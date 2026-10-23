/*----------------------
 | aica.h
 | Description: The Dreamcast's SPU backend: AICA channel register writes over G2 with the ARM7 held in reset.
 | Author: suinevere
 | Dependencies: spu_backend.h
 | Globals: aica_backend
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_AICA_H
#define LOM_AICA_H

#include "spu_backend.h"

/*----------------------
 | aica_backend
 | Description: SpuBackend answered by the AICA.
 | Author: suinevere
 | Dependencies: aica.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern const SpuBackend aica_backend;

/*----------------------
 | aica_init
 | Description: Holds the ARM7 in reset, keys every channel off and sets the master volume to full stereo.
 | Author: suinevere
 | Dependencies: spu_disable, spu_reset_chans, spu_master_mixer
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void aica_init(void);

/*----------------------
 | aica_monitor_service
 | Description: Once per tick: refreshes the cached EG level of the next few keyed channels, then preselects the following one so the next tick's first read needs no settling wait.
 | Author: suinevere
 | Dependencies: refresh_level, monitor_select, irq_disable, irq_restore
 | Globals: s_keyed, s_monitor_next
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void aica_monitor_service(void);

#endif
