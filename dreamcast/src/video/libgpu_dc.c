/*----------------------
 | libgpu_dc.c
 | Description: Dreamcast counting backend for the Psy-Q libgpu calls the ported game code makes: mirrors VRAM, counts ordering-table primitives through ot_walk_count and paces frames on the shared VSync pacer.
 | Author: suinevere
 | Dependencies: lom_prefix.h, gpu_tag.h, vram_mirror.h, ot_walk.h, vsync_pacer.h, libgpu_dc.h
 | Globals: s_draw_env, s_disp_env, s_disp_mask, s_frame_primitives, s_vsync_hook
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "gpu_tag.h"
#include "vram_mirror.h"
#include "ot_walk.h"
#include "vsync_pacer.h"
#include "libgpu_dc.h"

/*----------------------
 | s_vsync_hook
 | Description: Called after every VSync that is not a counter read, or null.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static DcVsyncHook s_vsync_hook;

/*----------------------
 | s_draw_env
 | Description: The draw environment from the last PutDrawEnv.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static DRAWENV s_draw_env;

/*----------------------
 | s_disp_env
 | Description: The display environment from the last PutDispEnv.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static DISPENV s_disp_env;

/*----------------------
 | s_disp_mask
 | Description: The display mask from the last SetDispMask.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_disp_mask;

/*----------------------
 | s_frame_primitives
 | Description: Primitives counted by DrawOTag since the last dc_frame_primitives call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_frame_primitives;

/*----------------------
 | ClearOTagR
 | Description: Links an ordering table back to front, ending at entry 0 which holds the terminator.
 | Author: suinevere
 | Dependencies: setaddr, GPU_TAG_END
 | Globals: N/A
 | Params: ot -- table, n -- entry count
 | Returns: ot
 ----------------------*/
u_long *ClearOTagR(u_long *ot, int n)
{
    int i;

    for (i = n - 1; i > 0; i--)
    {
        ot[i] = 0;
        setaddr(&ot[i], &ot[i - 1]);
    }
    ot[0] = GPU_TAG_END;
    return ot;
}

/*----------------------
 | DrawOTag
 | Description: Counts the chain's primitives with ot_walk_count and adds them to the per-frame total; D1 has no renderer to draw them.
 | Author: suinevere
 | Dependencies: ot_walk_count
 | Globals: s_frame_primitives
 | Params: ot -- first packet of the chain
 | Returns: N/A
 ----------------------*/
void DrawOTag(u_long *ot)
{
    s_frame_primitives += ot_walk_count(ot);
}

/*----------------------
 | dc_frame_primitives
 | Description: Returns the primitives DrawOTag has counted since the last call and resets the count to zero.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_frame_primitives
 | Params: N/A
 | Returns: primitives counted this frame
 ----------------------*/
unsigned dc_frame_primitives(void)
{
    unsigned count = s_frame_primitives;

    s_frame_primitives = 0;
    return count;
}

/*----------------------
 | DrawSync
 | Description: There is no queued GPU work to wait for; always idle.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mode -- ignored
 | Returns: 0
 ----------------------*/
int DrawSync(int mode)
{
    (void)mode;
    return 0;
}

/*----------------------
 | VSync
 | Description: Paces the game the PS1 way on the shared vblank counter, then runs the registered hook unless this was a counter read.
 | Author: suinevere
 | Dependencies: vsync_wait, vsync_count
 | Globals: s_vsync_hook
 | Params: mode -- PS1 VSync mode
 | Returns: vblank count for mode < 0, else 0
 ----------------------*/
int VSync(int mode)
{
    int result = vsync_wait(mode, vsync_count);

    if (mode >= 0 && s_vsync_hook != 0)
    {
        s_vsync_hook();
    }
    return result;
}

/*----------------------
 | dc_set_vsync_hook
 | Description: Registers the function VSync calls after each wait, so the platform can poll game state once per frame.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_vsync_hook
 | Params: hook -- function to call, or null for none
 | Returns: N/A
 ----------------------*/
void dc_set_vsync_hook(DcVsyncHook hook)
{
    s_vsync_hook = hook;
}

/*----------------------
 | SetDefDrawEnv
 | Description: Fills a DRAWENV the way libgpu does for a draw area.
 | Author: suinevere
 | Dependencies: setRECT
 | Globals: N/A
 | Params: env -- output, x, y, w, h -- draw area
 | Returns: env
 ----------------------*/
DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h)
{
    setRECT(&env->clip, x, y, w, h);
    env->ofs[0] = (short)x;
    env->ofs[1] = (short)y;
    setRECT(&env->tw, 0, 0, 0, 0);
    env->tpage = 0x0A;
    env->dtd = 1;
    env->dfe = 0;
    env->isbg = 0;
    env->r0 = 0;
    env->g0 = 0;
    env->b0 = 0;
    return env;
}

/*----------------------
 | SetDefDispEnv
 | Description: Fills a DISPENV the way libgpu does for a display area.
 | Author: suinevere
 | Dependencies: setRECT
 | Globals: N/A
 | Params: env -- output, x, y, w, h -- display area
 | Returns: env
 ----------------------*/
DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h)
{
    setRECT(&env->disp, x, y, w, h);
    setRECT(&env->screen, 0, 0, 0, 0);
    env->isinter = 0;
    env->isrgb24 = 0;
    env->pad0 = 0;
    env->pad1 = 0;
    return env;
}

/*----------------------
 | PutDrawEnv
 | Description: Records the draw environment; D1 has no renderer to apply it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_draw_env
 | Params: env -- draw environment
 | Returns: env
 ----------------------*/
DRAWENV *PutDrawEnv(DRAWENV *env)
{
    s_draw_env = *env;
    return env;
}

/*----------------------
 | PutDispEnv
 | Description: Records the display environment; D1 has no renderer to apply it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_disp_env
 | Params: env -- display environment
 | Returns: env
 ----------------------*/
DISPENV *PutDispEnv(DISPENV *env)
{
    s_disp_env = *env;
    return env;
}

/*----------------------
 | SetDispMask
 | Description: Records the display mask; D1 has no renderer to apply it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_disp_mask
 | Params: mask -- display mask
 | Returns: N/A
 ----------------------*/
void SetDispMask(int mask)
{
    s_disp_mask = mask;
}

/*----------------------
 | LoadImage
 | Description: Uploads converted pixel data into the VRAM mirror at a rectangle.
 | Author: suinevere
 | Dependencies: vram_mirror_load
 | Globals: N/A
 | Params: rect -- VRAM rectangle in words, data -- converted source
 | Returns: 0
 ----------------------*/
int LoadImage(RECT *rect, u_long *data)
{
    vram_mirror_load(rect, data);
    return 0;
}

/*----------------------
 | ClearImage
 | Description: Fills a VRAM mirror rectangle with the PS1 15-bit colour packed from r, g, b.
 | Author: suinevere
 | Dependencies: vram_mirror_clear
 | Globals: N/A
 | Params: rect -- area, r, g, b -- fill colour
 | Returns: 0
 ----------------------*/
int ClearImage(RECT *rect, u_char r, u_char g, u_char b)
{
    vram_mirror_clear(rect, r, g, b);
    return 0;
}

/*----------------------
 | StoreImage
 | Description: Reads VRAM mirror pixels out of a rectangle.
 | Author: suinevere
 | Dependencies: vram_mirror_store
 | Globals: N/A
 | Params: rect -- VRAM rectangle, data -- destination buffer
 | Returns: 0
 ----------------------*/
int StoreImage(RECT *rect, u_long *data)
{
    vram_mirror_store(rect, data);
    return 0;
}

/*----------------------
 | MoveImage
 | Description: Copies a VRAM mirror rectangle to a new position.
 | Author: suinevere
 | Dependencies: vram_mirror_move
 | Globals: N/A
 | Params: rect -- source rectangle, x, y -- destination origin
 | Returns: 0
 ----------------------*/
int MoveImage(RECT *rect, int x, int y)
{
    vram_mirror_move(rect, x, y);
    return 0;
}

/*----------------------
 | SetDrawEnv
 | Description: Writes a DR_ENV packet carrying the draw environment's clip rectangle and offset; D1 has no renderer to apply it, so this only marks the packet non-empty for the primitive counter.
 | Author: suinevere
 | Dependencies: setlen
 | Globals: N/A
 | Params: dr_env -- packet to fill, env -- draw environment
 | Returns: N/A
 ----------------------*/
void SetDrawEnv(DR_ENV *dr_env, DRAWENV *env)
{
    setlen(dr_env, 15);
    dr_env->code[0] = (u_long)env->clip.x;
    dr_env->code[1] = (u_long)env->clip.y;
    dr_env->code[2] = (u_long)env->clip.w;
    dr_env->code[3] = (u_long)env->clip.h;
    dr_env->code[4] = (u_long)env->ofs[0];
    dr_env->code[5] = (u_long)env->ofs[1];
}

/*----------------------
 | ResetGraph
 | Description: GPU reset has no Dreamcast equivalent.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mode -- ignored
 | Returns: 0
 ----------------------*/
int ResetGraph(int mode)
{
    (void)mode;
    return 0;
}

/*----------------------
 | SetGraphDebug
 | Description: GPU debug level has no Dreamcast equivalent.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: level -- ignored
 | Returns: 0
 ----------------------*/
int SetGraphDebug(int level)
{
    (void)level;
    return 0;
}
