/*----------------------
 | libgpu_dc.c
 | Description: Dreamcast backend for the Psy-Q libgpu calls the ported game code makes: mirrors VRAM, decodes ordering tables into the PowerVR submitter and paces frames on the shared VSync pacer.
 | Author: suinevere
 | Dependencies: lom_prefix.h, gpu_tag.h, vram_mirror.h, gpu_decode.h, pvr_submit.h, vsync_pacer.h, libgpu_dc.h
 | Globals: s_draw_env, s_disp_env, s_disp_seen, s_disp_mask, s_frame_primitives, s_vsync_hook, s_gpu_state
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "gpu_tag.h"
#include "vram_mirror.h"
#include "gpu_decode.h"
#include "pvr_submit.h"
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
 | s_disp_seen
 | Description: The last two distinct display areas, newest first: a double-buffered game shows the area it is not drawing to.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static RECT s_disp_seen[2];

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
 | s_gpu_state
 | Description: Decoder state carried across DrawOTag and DrawPrim calls, reset from each PutDrawEnv.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static DcGpuState s_gpu_state;

/*----------------------
 | s_origin_area
 | Description: Draw-area top-left the submitter's origin was last computed for, so a DR_AREA packet inside an ordering table moves it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int16_t s_origin_area[2];

static void update_origin(void);

/*----------------------
 | submit_sink
 | Description: Decoder sink that counts each primitive for the frame, re-places the origin when a packet moved the draw area, and hands it to the PowerVR submitter.
 | Author: suinevere
 | Dependencies: pvr_submit_prim, update_origin
 | Globals: s_frame_primitives, s_origin_area
 | Params: prim -- decoded primitive, state -- GPU state at decode time, ctx -- unused
 | Returns: N/A
 ----------------------*/
static void submit_sink(const DcPrim *prim, const DcGpuState *state, void *ctx)
{
    (void)ctx;
    s_frame_primitives++;
    if (state->area_x1 != s_origin_area[0] || state->area_y1 != s_origin_area[1])
    {
        update_origin();
    }
    pvr_submit_prim(prim, state);
}

/*----------------------
 | pack_colour
 | Description: Packs 8-bit r, g, b into a PS1 15-bit colour with the mask bit clear.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: r, g, b -- 8-bit channels
 | Returns: PS1 15-bit colour
 ----------------------*/
static uint16_t pack_colour(int r, int g, int b)
{
    return (uint16_t)(((r >> 3) & 0x1F) | (((g >> 3) & 0x1F) << 5) | (((b >> 3) & 0x1F) << 10));
}

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
 | update_origin
 | Description: Tells the submitter where the draw area lands on screen: its offset inside whichever recent display area contains its top-left corner, else the screen corner.
 | Author: suinevere
 | Dependencies: pvr_submit_origin
 | Globals: s_disp_seen, s_gpu_state, s_origin_area
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void update_origin(void)
{
    int i;

    s_origin_area[0] = s_gpu_state.area_x1;
    s_origin_area[1] = s_gpu_state.area_y1;

    for (i = 0; i < 2; i++)
    {
        const RECT *d = &s_disp_seen[i];

        if (d->w > 0 && d->h > 0 && s_gpu_state.area_x1 >= d->x && s_gpu_state.area_x1 < d->x + d->w &&
            s_gpu_state.area_y1 >= d->y && s_gpu_state.area_y1 < d->y + d->h)
        {
            pvr_submit_origin(s_gpu_state.area_x1 - d->x, s_gpu_state.area_y1 - d->y);
            return;
        }
    }
    pvr_submit_origin(0, 0);
}

/*----------------------
 | DrawOTag
 | Description: Decodes the chain in OT order and submits every primitive to the PowerVR.
 | Author: suinevere
 | Dependencies: gpu_decode_ot, submit_sink
 | Globals: s_gpu_state
 | Params: ot -- first packet of the chain
 | Returns: N/A
 ----------------------*/
void DrawOTag(u_long *ot)
{
    gpu_decode_ot(&s_gpu_state, ot, submit_sink, 0);
}

/*----------------------
 | DrawPrim
 | Description: Decodes one packet without following its link and submits its primitives to the PowerVR.
 | Author: suinevere
 | Dependencies: gpu_decode_packet, gpu_tag_len, submit_sink
 | Globals: s_gpu_state
 | Params: prim -- packet, tag word first
 | Returns: N/A
 ----------------------*/
void DrawPrim(void *prim)
{
    const u_long *packet = (const u_long *)prim;

    gpu_decode_packet(&s_gpu_state, (const uint32_t *)(packet + 1), gpu_tag_len((uint32_t)packet[0]), submit_sink, 0);
}

/*----------------------
 | dc_frame_primitives
 | Description: Returns the primitives DrawOTag and DrawPrim decoded since the last call and resets the count to zero.
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
 | Description: Flips an open PowerVR scene on the waiting modes (0 and above 1) and paces the game on the shared vblank counter, then runs the hook unless this was a counter read.
 | VSync(1) returns at once on the PS1 and games call it mid-frame, so it never flips.
 | Author: suinevere
 | Dependencies: pvr_submit_flip, vsync_wait, vsync_count
 | Globals: s_vsync_hook
 | Params: mode -- PS1 VSync mode
 | Returns: vblank count for mode < 0, else 0
 ----------------------*/
int VSync(int mode)
{
    int result;

    if (mode == 0 || mode > 1)
    {
        pvr_submit_flip();
    }
    result = vsync_wait(mode, vsync_count);

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
 | Description: Records the draw environment, resets the decoder's area, offset and tpage from it, places it on screen, and sets the PowerVR background when isbg asks for a clear.
 | Author: suinevere
 | Dependencies: gpu_decode_reset, update_origin, pvr_submit_background, pack_colour
 | Globals: s_draw_env, s_gpu_state
 | Params: env -- draw environment
 | Returns: env
 ----------------------*/
DRAWENV *PutDrawEnv(DRAWENV *env)
{
    s_draw_env = *env;
    gpu_decode_reset(&s_gpu_state);
    s_gpu_state.area_x1 = env->clip.x;
    s_gpu_state.area_y1 = env->clip.y;
    s_gpu_state.area_x2 = (int16_t)(env->clip.x + env->clip.w - 1);
    s_gpu_state.area_y2 = (int16_t)(env->clip.y + env->clip.h - 1);
    s_gpu_state.ofs_x = env->ofs[0];
    s_gpu_state.ofs_y = env->ofs[1];
    s_gpu_state.tpage = (uint16_t)((env->tpage & 0x1FF) | (env->dtd ? 0x200 : 0) | (env->dfe ? 0x400 : 0));
    update_origin();
    if (env->isbg)
    {
        pvr_submit_background(pack_colour(env->r0, env->g0, env->b0));
    }
    return env;
}

/*----------------------
 | PutDispEnv
 | Description: Records the display environment and remembers its area alongside the previous distinct one, so draw areas can be placed on screen.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_disp_env, s_disp_seen
 | Params: env -- display environment
 | Returns: env
 ----------------------*/
DISPENV *PutDispEnv(DISPENV *env)
{
    const RECT *r = &env->disp;

    s_disp_env = *env;
    if (r->x != s_disp_seen[0].x || r->y != s_disp_seen[0].y || r->w != s_disp_seen[0].w || r->h != s_disp_seen[0].h)
    {
        s_disp_seen[1] = s_disp_seen[0];
        s_disp_seen[0] = *r;
    }
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
 | Description: Fills a VRAM mirror rectangle, then clears the PowerVR scene: the background when the rect covers the draw area, else an opaque rect for the part inside it.
 | Author: suinevere
 | Dependencies: vram_mirror_clear, pvr_submit_background, pvr_submit_rect, pack_colour
 | Globals: s_gpu_state
 | Params: rect -- area, r, g, b -- fill colour
 | Returns: 0
 ----------------------*/
int ClearImage(RECT *rect, u_char r, u_char g, u_char b)
{
    uint16_t colour = pack_colour(r, g, b);
    int x1 = rect->x;
    int y1 = rect->y;
    int x2 = rect->x + rect->w - 1;
    int y2 = rect->y + rect->h - 1;

    vram_mirror_clear(rect, r, g, b);
    if (x1 <= s_gpu_state.area_x1 && y1 <= s_gpu_state.area_y1 && x2 >= s_gpu_state.area_x2 && y2 >= s_gpu_state.area_y2)
    {
        pvr_submit_background(colour);
        return 0;
    }
    x1 = x1 > s_gpu_state.area_x1 ? x1 : s_gpu_state.area_x1;
    y1 = y1 > s_gpu_state.area_y1 ? y1 : s_gpu_state.area_y1;
    x2 = x2 < s_gpu_state.area_x2 ? x2 : s_gpu_state.area_x2;
    y2 = y2 < s_gpu_state.area_y2 ? y2 : s_gpu_state.area_y2;
    if (x1 <= x2 && y1 <= y2)
    {
        pvr_submit_rect(x1 - s_gpu_state.area_x1, y1 - s_gpu_state.area_y1, x2 - x1 + 1, y2 - y1 + 1, colour);
    }
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
 | Description: Writes real GP0 0xE1 (tpage/dtd/dfe), 0xE2 (texture window), 0xE3/0xE4 (draw area) and 0xE5 (offset) words into a DR_ENV, so gpu_decode_packet can decode this packet like any other.
 | Author: suinevere
 | Dependencies: setlen
 | Globals: N/A
 | Params: dr_env -- packet to fill, env -- draw environment
 | Returns: N/A
 ----------------------*/
void SetDrawEnv(DR_ENV *dr_env, DRAWENV *env)
{
    unsigned long mode = ((unsigned long)env->tpage & 0x1FFul) | (env->dtd ? 0x200ul : 0ul) | (env->dfe ? 0x400ul : 0ul);
    unsigned long tw_word = 0xE2000000ul |
        (unsigned long)((((env->tw.y & 0xFF) >> 3) << 15) |
                         (((env->tw.x & 0xFF) >> 3) << 10) |
                         (((~(env->tw.h - 1) & 0xFF) >> 3) << 5) |
                         ((~(env->tw.w - 1) & 0xFF) >> 3));

    setlen(dr_env, 6);
    dr_env->code[0] = 0xE1000000ul | mode;
    dr_env->code[1] = tw_word;
    dr_env->code[2] = 0;
    dr_env->code[3] = 0xE3000000ul | ((unsigned long)(u_short)env->clip.x & 0x3FFul) |
        (((unsigned long)(u_short)env->clip.y & 0x1FFul) << 10);
    dr_env->code[4] = 0xE4000000ul | ((unsigned long)(u_short)(env->clip.x + env->clip.w - 1) & 0x3FFul) |
        (((unsigned long)(u_short)(env->clip.y + env->clip.h - 1) & 0x1FFul) << 10);
    dr_env->code[5] = 0xE5000000ul | ((unsigned long)env->ofs[0] & 0x7FFul) |
        (((unsigned long)env->ofs[1] & 0x7FFul) << 11);
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
