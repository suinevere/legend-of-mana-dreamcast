/*----------------------
 | akao_tick_dc.c
 | Description: AKAO's root counter 2 on the Dreamcast: TMU1 interrupts at 240 Hz (0x44E8 counts at 33.8688 MHz / 8 on the PS1) and each runs AKAO's tick, then the SPU voice flush. TMU1 is free after boot: KallistiOS uses it only for timer_spin_sleep, which nothing on our path calls once threads run.
 | Author: suinevere
 | Dependencies: akao_tick_dc.h, aica.h, spu_shim.h, arch/irq.h, arch/timer.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <arch/irq.h>
#include <arch/timer.h>

#include "aica.h"
#include "akao_tick_dc.h"
#include "spu_shim.h"

/*----------------------
 | AKAO_TICK_HZ
 | Description: AKAO's tick rate on the PS1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AKAO_TICK_HZ 240

/*----------------------
 | AKAO_EVENT_HANDLE
 | Description: The handle lom_sound_OpenEvent returns, shaped like the BIOS's root counter 2 descriptor.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AKAO_EVENT_HANDLE 0xF2000002L

/*----------------------
 | s_tick / s_ticks / s_started
 | Description: AKAO's tick callback, the ticks run so far, and whether TMU1 has been started.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static long (*s_tick)();
static volatile unsigned s_ticks;
static int s_started;

/*----------------------
 | tick_irq
 | Description: TMU1 underflow: clears the timer, refreshes some cached AICA envelope levels, runs AKAO's tick and flushes its SPU register changes to the AICA.
 | Author: suinevere
 | Dependencies: timer_clear, aica_monitor_service, spu_voice_flush
 | Globals: s_tick, s_ticks
 | Params: source / context / data -- unused
 | Returns: N/A
 ----------------------*/
static void tick_irq(irq_t source, irq_context_t *context, void *data)
{
    (void)source;
    (void)context;
    (void)data;
    timer_clear(TMU1);
    aica_monitor_service();
    if (s_tick != NULL)
        s_tick();
    spu_voice_flush();
    s_ticks++;
}

/*----------------------
 | akao_tick_dc_init
 | Description: Brings up the AICA backend and hands it to the SPU shim; AKAO's own init later arms the tick.
 | Author: suinevere
 | Dependencies: aica_init, spu_shim_set_backend
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void akao_tick_dc_init(void)
{
    aica_init();
    spu_shim_set_backend(&aica_backend);
}

/*----------------------
 | akao_tick_dc_count
 | Description: AKAO ticks run since the tick was armed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_ticks
 | Params: N/A
 | Returns: tick count
 ----------------------*/
unsigned akao_tick_dc_count(void)
{
    return s_ticks;
}

/*----------------------
 | lom_SetRCnt / lom_StartRCnt / lom_GetRCnt
 | Description: Root counter 2 needs no setup here; GetRCnt's self-timing reads 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: as libapi.h
 | Returns: 1, or 0 for GetRCnt
 ----------------------*/
long lom_SetRCnt(unsigned long counter, unsigned short target, long mode)
{
    (void)counter;
    (void)target;
    (void)mode;
    return 1;
}

long lom_StartRCnt(unsigned long counter)
{
    (void)counter;
    return 1;
}

long lom_GetRCnt(unsigned long counter)
{
    (void)counter;
    return 0;
}

/*----------------------
 | lom_sound_OpenEvent
 | Description: Keeps AKAO's tick callback.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_tick
 | Params: desc / spec / mode -- unused, func -- tick callback
 | Returns: AKAO_EVENT_HANDLE
 ----------------------*/
long lom_sound_OpenEvent(unsigned long desc, long spec, long mode, long (*func)())
{
    (void)desc;
    (void)spec;
    (void)mode;
    s_tick = func;
    return AKAO_EVENT_HANDLE;
}

/*----------------------
 | lom_sound_EnableEvent
 | Description: Starts the 240 Hz TMU1 interrupt the first time; afterwards unmasks it again, keeping the timer's phase, after the DisableEvent that brackets each AKAO command.
 | Author: suinevere
 | Dependencies: irq_set_handler, timer_prime, timer_start, timer_enable_ints, tick_irq
 | Globals: s_started
 | Params: event -- unused
 | Returns: 1
 ----------------------*/
long lom_sound_EnableEvent(long event)
{
    (void)event;
    if (s_started)
    {
        timer_enable_ints(TMU1);
        return 1;
    }
    s_started = 1;
    irq_set_handler(EXC_TMU1_TUNI1, tick_irq, NULL);
    timer_prime(TMU1, AKAO_TICK_HZ, 1);
    timer_start(TMU1);
    return 1;
}

/*----------------------
 | lom_sound_DisableEvent
 | Description: Masks the tick interrupt; AKAO brackets each command with this and EnableEvent so the tick never sees a half-applied command.
 | Author: suinevere
 | Dependencies: timer_disable_ints
 | Globals: N/A
 | Params: event -- unused
 | Returns: 1
 ----------------------*/
long lom_sound_DisableEvent(long event)
{
    (void)event;
    timer_disable_ints(TMU1);
    return 1;
}

/*----------------------
 | lom_StopRCnt / lom_sound_UnDeliverEvent
 | Description: AKAO's driver shutdown: stops the tick timer; there is no pending event to drop.
 | Author: suinevere
 | Dependencies: timer_stop, timer_disable_ints
 | Globals: s_started
 | Params: as libapi.h
 | Returns: 1, or nothing for UnDeliverEvent
 ----------------------*/
long lom_StopRCnt(unsigned long counter)
{
    (void)counter;
    timer_disable_ints(TMU1);
    timer_stop(TMU1);
    s_started = 0;
    return 1;
}

void lom_sound_UnDeliverEvent(unsigned long event, unsigned long spec)
{
    (void)event;
    (void)spec;
}

/*----------------------
 | lom_sound_CdMix
 | Description: AKAO's CD/XA mixing volumes; XA audio plays through the AICA stream channel, whose volume comes from the SPU CD volume registers, so this has nothing to set.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: volume -- unused
 | Returns: 1
 ----------------------*/
int lom_sound_CdMix(void *volume)
{
    (void)volume;
    return 1;
}
