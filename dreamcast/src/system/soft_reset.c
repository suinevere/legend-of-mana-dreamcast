/*----------------------
 | soft_reset.c
 | Description: The Dreamcast soft reset: on A, B, X, Y and Start held together, reads the game's own binary back from the disc, descrambled when it comes from a CD-R, and runs it afresh, which starts again at the Squaresoft logo.
 | Author: suinevere
 | Dependencies: kos/fs.h, arch/cache.h, arch/irq.h, arch/wdt.h, dc/g2bus.h, dc/maple.h, dc/spu.h, dc/video.h, string.h, pad_dc.h, pad_dc_map.h, card_store_kos.h, boot_descramble.h, dc_system.h, hang_watch.h, state_report_dc.h, lom_platform_memory.h
 | Globals: lom_load_buffer, lom_secondary_overlay
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <kos/fs.h>
#include <arch/cache.h>
#include <arch/irq.h>
#include <arch/wdt.h>
#include <dc/g2bus.h>
#include <dc/maple.h>
#include <dc/spu.h>
#include <dc/video.h>
#include <string.h>

#include "soft_reset.h"
#include "lom_platform_memory.h"
#include "card_store_kos.h"
#include "pad_dc.h"
#include "pad_dc_map.h"
#include "boot_descramble.h"
#include "dc_system.h"
#include "hang_watch.h"
#include "state_report_dc.h"

/*----------------------
 | SOFT_RESET_BINARY
 | Description: The program file the BIOS boots, a plain binary on our GD-ROM, loaded at 0x8C010000 like the first boot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SOFT_RESET_BINARY "/cd/1ST_READ.BIN"

/*----------------------
 | SOFT_RESET_LOAD_ADDRESS / SOFT_RESET_MATCH_BYTES
 | Description: Where the BIOS put the running binary, and how many of its first bytes a binary read back must share to be taken as this one.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SOFT_RESET_LOAD_ADDRESS 0x8C010000u
#define SOFT_RESET_MATCH_BYTES 64u

extern unsigned char lom_load_buffer[];

/*----------------------
 | _arch_exec_template / _arch_exec_template_values / _arch_exec_template_end / _arch_old_*
 | Description: KallistiOS's copy-and-jump trampoline and the SR, VBR, FPSCR and stack the BIOS handed it at boot.
 | Author: suinevere
 | Dependencies: KallistiOS execasm.s, startup.s
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern uint32_t _arch_exec_template[];
extern uint32_t _arch_exec_template_values[];
extern uint32_t _arch_exec_template_end[];
extern uint32_t _arch_old_sr, _arch_old_vbr, _arch_old_stack, _arch_old_fpscr;

/*----------------------
 | soft_reset_read_binary
 | Description: Reads the boot binary into lom_secondary_overlay, then puts it into lom_load_buffer, which lies in .bss above the area the binary is copied over so arch_exec can copy it down: as read from a GD-ROM, descrambled from a CD-R. Either way its start must match the running binary's.
 | Author: suinevere
 | Dependencies: fs_open, fs_total, fs_read, fs_close, boot_descramble, dc_log
 | Globals: lom_load_buffer, lom_secondary_overlay
 | Params: N/A
 | Returns: the binary's size, or 0 when it cannot be read whole
 ----------------------*/
static size_t soft_reset_read_binary(void)
{
    file_t f = fs_open(SOFT_RESET_BINARY, O_RDONLY);
    size_t size;
    ssize_t got;

    if (f < 0)
    {
        dc_log("soft reset: no %s", SOFT_RESET_BINARY);
        return 0;
    }
    size = fs_total(f);
    got = size <= LOM_DC_LOAD_BUFFER_SIZE && size <= LOM_SECONDARY_OVERLAY_SIZE ? fs_read(f, lom_secondary_overlay, size) : -1;
    fs_close(f);
    if (got < 0 || (size_t)got != size || size < SOFT_RESET_MATCH_BYTES)
    {
        dc_log("soft reset: read %d of %u", (int)got, (unsigned)size);
        return 0;
    }
    if (memcmp(lom_secondary_overlay, (const void *)SOFT_RESET_LOAD_ADDRESS, SOFT_RESET_MATCH_BYTES) == 0)
    {
        memcpy(lom_load_buffer, lom_secondary_overlay, size);
    }
    else if (boot_descramble(lom_secondary_overlay, lom_load_buffer, size) != 0 ||
             memcmp(lom_load_buffer, (const void *)SOFT_RESET_LOAD_ADDRESS, SOFT_RESET_MATCH_BYTES) != 0)
    {
        dc_log("soft reset: %s is not this game", SOFT_RESET_BINARY);
        return 0;
    }
    return size;
}

/*----------------------
 | soft_reset_exec
 | Description: arch_exec without arch_shutdown, whose frees halt on hardware: quiets the watchdog, Maple, G2 DMA and the sound CPU, then copies the binary to 0x8C010000 and jumps there with the BIOS's registers.
 | Author: suinevere
 | Dependencies: irq_disable, wdt_disable, maple_dma_stop, g2_dma_shutdown, spu_disable, dcache_flush_range, icache_flush_range
 | Globals: _arch_exec_template, _arch_exec_template_values, _arch_exec_template_end, _arch_old_sr, _arch_old_vbr, _arch_old_stack, _arch_old_fpscr
 | Params: image -- the binary; length -- its size in bytes, a multiple of four
 | Returns: never
 ----------------------*/
static void soft_reset_exec(const void *image, uint32_t length)
{
    uint32_t count = (uint32_t)(_arch_exec_template_end - _arch_exec_template);
    uint32_t buffer[count];
    uint32_t *values = buffer + (_arch_exec_template_values - _arch_exec_template);
    uint32_t i;

    irq_disable();
    wdt_disable();
    maple_dma_stop();
    g2_dma_shutdown();
    spu_disable();
    dcache_flush_range((uint32_t)image, length);
    for (i = 0; i < count; i++)
    {
        buffer[i] = _arch_exec_template[i];
    }
    values[0] = (uint32_t)image;
    values[1] = SOFT_RESET_LOAD_ADDRESS;
    values[2] = length / 4u;
    values[3] = _arch_old_stack;
    dcache_flush_range((uint32_t)buffer, count * 4u);
    icache_flush_range((uint32_t)buffer, count * 4u);
    __asm__ __volatile__("ldc %0,sr" : : "z"(_arch_old_sr) : "memory");
    __asm__ __volatile__("ldc %0,vbr" : : "z"(_arch_old_vbr) : "memory");
    __asm__ __volatile__("lds %0,fpscr" : : "z"(_arch_old_fpscr) : "memory");
    ((void (*)(void))buffer)();
    for (;;)
    {
    }
}

/*----------------------
 | soft_reset_poll
 | Description: Once a frame from the VSync hook: on the soft-reset chord, unless a VMU save is being written, silences the sound, reads the binary, waits for the chord to be let go so the restart does not see A held (the 50 Hz request), feeding the hang check and polling the state report as VSync would, and runs the binary.
 | Author: suinevere
 | Dependencies: pad_dc_read, pad_dc_soft_reset_chord, card_store_kos_busy, spu_master_mixer, soft_reset_read_binary, vid_waitvbl, hang_watch_feed, state_report_poll, dc_log, soft_reset_exec
 | Globals: lom_load_buffer
 | Params: N/A
 | Returns: N/A, or never when the reset runs
 ----------------------*/
void soft_reset_poll(void)
{
    DcPadSample sample;
    size_t size;

    if (!pad_dc_read(&sample) || !pad_dc_soft_reset_chord(&sample) || card_store_kos_busy())
    {
        return;
    }
    spu_master_mixer(0, 1);
    size = soft_reset_read_binary();
    if (size == 0)
    {
        spu_master_mixer(15, 1);
        return;
    }
    while (pad_dc_read(&sample) && sample.buttons != 0)
    {
        vid_waitvbl();
        hang_watch_feed();
        state_report_poll();
    }
    dc_log("soft reset");
    soft_reset_exec(lom_load_buffer, (size + 3u) & ~3u);
}
