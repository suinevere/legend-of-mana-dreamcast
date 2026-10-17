/*----------------------
 | sdk_boot_dc.c
 | Description: No-op Dreamcast versions of the Psy-Q system, card and BIOS calls the PS1 boot path makes; libgpu_dc.c owns the graphics ones and port/sound/libspu_shim.c the sound ones.
 | Author: suinevere
 | Dependencies: lom_prefix.h, libapi.h, libetc.h, libmcx.h, saturn/src/boot/sdk_boot.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <libapi.h>
#include <libetc.h>
#include <libmcx.h>

/*----------------------
 | SetMem
 | Description: Ignores the PS1 kernel heap size; KallistiOS sizes memory itself.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: size -- ignored
 | Returns: N/A
 ----------------------*/
void SetMem(long size)
{
    (void)size;
}

/*----------------------
 | SetConf
 | Description: Ignores the PS1 kernel event, TCB and stack count config; KallistiOS configures its own threads.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ev -- ignored, tcb -- ignored, sp -- ignored
 | Returns: 0
 ----------------------*/
long SetConf(unsigned long ev, unsigned long tcb, unsigned long sp)
{
    (void)ev;
    (void)tcb;
    (void)sp;
    return 0;
}

/*----------------------
 | _96_remove
 | Description: Ignores the PS1 BIOS shell removal request; the Dreamcast has no such BIOS shell.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void _96_remove(void)
{
}

/*----------------------
 | ResetCallback
 | Description: Ignores the PS1 reset callback registration.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 0
 ----------------------*/
int ResetCallback(void)
{
    return 0;
}

/*----------------------
 | StopCallback
 | Description: Ignores the PS1 stop callback registration.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 0
 ----------------------*/
int StopCallback(void)
{
    return 0;
}

/*----------------------
 | SetVideoMode
 | Description: Ignores the PS1 video mode select; dc_system_init sets the Dreamcast video mode.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mode -- ignored
 | Returns: 0
 ----------------------*/
long SetVideoMode(long mode)
{
    (void)mode;
    return 0;
}

/*----------------------
 | __main
 | Description: No-op; satisfies the PS1 toolchain's runtime entry hook, which KallistiOS's crt0 already covers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void __main(void)
{
}

/*----------------------
 | InitCARD
 | Description: Ignores PS1 memory card initialization.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: pad_enable -- ignored
 | Returns: N/A
 ----------------------*/
void InitCARD(long pad_enable)
{
    (void)pad_enable;
}

/*----------------------
 | StartCARD
 | Description: Ignores PS1 memory card startup.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 0
 ----------------------*/
long StartCARD(void)
{
    return 0;
}

/*----------------------
 | _bu_init
 | Description: Ignores PS1 memory card buffer unit initialization.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void _bu_init(void)
{
}

/*----------------------
 | McxStartCom
 | Description: Ignores PS1 PocketStation communication startup.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void McxStartCom(void)
{
}

/*----------------------
 | ChangeClearPAD
 | Description: Ignores the PS1 pad-clear mode change.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: value -- ignored
 | Returns: N/A
 ----------------------*/
void ChangeClearPAD(long value)
{
    (void)value;
}

/*----------------------
 | Krom2RawAdd
 | Description: The PS1 BIOS kanji ROM does not exist on the Dreamcast; reports every character as missing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: sjis -- Shift-JIS code, ignored
 | Returns: -1
 ----------------------*/
long Krom2RawAdd(unsigned long sjis)
{
    (void)sjis;
    return -1;
}
