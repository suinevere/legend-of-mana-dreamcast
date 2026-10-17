/*----------------------
 | akao_tick_dc.h
 | Description: The Dreamcast's stand-in for AKAO's root counter 2: a 240 Hz TMU1 interrupt that runs AKAO's tick and the SPU voice flush.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_AKAO_TICK_DC_H
#define LOM_AKAO_TICK_DC_H

/*----------------------
 | akao_tick_dc_init
 | Description: Brings up the AICA backend and hands it to the SPU shim; AKAO's own init later arms the tick.
 | Author: suinevere
 | Dependencies: aica_init, spu_shim_set_backend
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void akao_tick_dc_init(void);

/*----------------------
 | akao_tick_dc_count
 | Description: AKAO ticks run since the tick was armed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: tick count
 ----------------------*/
unsigned akao_tick_dc_count(void);

/*----------------------
 | lom_SetRCnt / lom_StartRCnt / lom_GetRCnt / lom_StopRCnt / lom_sound_OpenEvent / lom_sound_EnableEvent / lom_sound_DisableEvent / lom_sound_UnDeliverEvent / lom_sound_CdMix
 | Description: The root-counter, event and CD-mix calls AKAO's driver makes, renamed at compile time; EnableEvent starts the 240 Hz TMU1 tick and later unmasks it, DisableEvent masks it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: as libapi.h
 | Returns: as libapi.h
 ----------------------*/
long lom_SetRCnt(unsigned long counter, unsigned short target, long mode);
long lom_StartRCnt(unsigned long counter);
long lom_GetRCnt(unsigned long counter);
long lom_sound_OpenEvent(unsigned long desc, long spec, long mode, long (*func)());
long lom_sound_EnableEvent(long event);
long lom_sound_DisableEvent(long event);
long lom_StopRCnt(unsigned long counter);
void lom_sound_UnDeliverEvent(unsigned long event, unsigned long spec);
int lom_sound_CdMix(void *volume);

#endif
