/*----------------------
 | libgpu_dc.c
 | Description: Dreamcast backend for the Psy-Q libgpu calls the ported game code makes: mirrors VRAM, decodes ordering tables into the PowerVR submitter, and paces frames on the shared VSync pacer. Also reads back the finished PowerVR frame for StoreImage and MoveImage.
 | Author: suinevere
 | Dependencies: lom_prefix.h, gpu_tag.h, vram_mirror.h, gpu_decode.h, pvr_submit.h, pvr_backend.h, frame_readback.h, vsync_pacer.h, libgpu_dc.h, heap_probe.h
 | Globals: s_draw_env, s_disp_env, s_disp_seen, s_disp_mask, s_frame_primitives, s_vsync_hook, s_gpu_state, s_readback_line, s_draw_matches_disp, s_immediate_scene
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "gpu_tag.h"
#include "vram_mirror.h"
#include "gpu_decode.h"
#include "pvr_submit.h"
#include "pvr_backend.h"
#include "frame_readback.h"
#include "vsync_pacer.h"
#include "libgpu_dc.h"
#include "cdrom_dc.h"
#include "rgb24_display.h"
#include "dc_system.h"
#include "heap_probe.h"

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
 | s_otag_us
 | Description: Microseconds spent in DrawOTag walking ordering tables and submitting their primitives, for the speed readout.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned long long s_otag_us;

/*----------------------
 | s_vsync_idle
 | Description: Called on every pass of VSync's vblank wait, or null.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static DcVsyncHook s_vsync_idle;

/*----------------------
 | s_async_poll
 | Description: Called from DrawSync and VSync to deliver work a worker finished, such as an MDEC decode, or null.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static DcVsyncHook s_async_poll;

/*----------------------
 | idle_count
 | Description: VSync's wait clock: runs the idle hook, then reads the vblank count.
 | Author: suinevere
 | Dependencies: vsync_count
 | Globals: s_vsync_idle
 | Params: N/A
 | Returns: the vblank count
 ----------------------*/
static unsigned idle_count(void)
{
    if (s_vsync_idle != 0)
    {
        s_vsync_idle();
    }
    return vsync_count();
}

/*----------------------
 | s_vsync_wait_us
 | Description: Microseconds VSync has spent waiting for vblanks since start-up, the main thread's idle time.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned long long s_vsync_wait_us;

/*----------------------
 | s_draw_sync_callback
 | Description: The handler DrawSyncCallback installed, run when a DrawOTag finishes; NULL for none.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void (*s_draw_sync_callback)(void);

/*----------------------
 | RGB24_FRAME_PITCH / RGB24_FRAME_ROWS / s_rgb24_frame
 | Description: The RGB565 staging frame a 24-bit display area converts into, sized as the PowerVR texture it is loaded into.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RGB24_FRAME_PITCH 512
#define RGB24_FRAME_ROWS 256
static uint16_t s_rgb24_frame[RGB24_FRAME_PITCH * RGB24_FRAME_ROWS] __attribute__((aligned(32)));

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

/*----------------------
 | s_draw_matches_disp
 | Description: Whether the last PutDrawEnv's area is pixel-identical to the last PutDispEnv's area, so primitives land in the buffer the CRTC is currently scanning.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_draw_matches_disp;

/*----------------------
 | s_immediate_scene
 | Description: Whether the open scene's first primitive came from DrawPrim and no DrawOTag has added to it since, the mark of a loading card drawn outside the frame loop.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_immediate_scene;

static void update_origin(void);
static void update_draw_matches_disp(void);
static void sync_flip(void);
static void present_immediate_scene(void);

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
 | update_draw_matches_disp
 | Description: Recomputes s_draw_matches_disp by comparing the current draw area against the current display area.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_draw_env, s_disp_env, s_draw_matches_disp
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void update_draw_matches_disp(void)
{
    const RECT *draw = &s_draw_env.clip;
    const RECT *disp = &s_disp_env.disp;

    s_draw_matches_disp = draw->x == disp->x && draw->y == disp->y && draw->w == disp->w && draw->h == disp->h;
}

/*----------------------
 | sync_flip
 | Description: Presents the open scene now, outside the normal VSync flip, when it drew into the buffer currently on screen -- real PS1 hardware shows such a draw the instant it lands.
 | Author: suinevere
 | Dependencies: pvr_submit_scene_has_prims, pvr_submit_flip
 | Globals: s_draw_matches_disp
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void sync_flip(void)
{
    if (s_draw_matches_disp && pvr_submit_scene_has_prims())
    {
        pvr_submit_flip();
    }
}

/*----------------------
 | DrawOTag
 | Description: Decodes the chain in OT order and submits every primitive to the PowerVR, then runs the DrawSync callback as the PS1 does when drawing ends.
 | Author: suinevere
 | Dependencies: gpu_decode_ot, submit_sink, dc_time_us
 | Globals: s_gpu_state, s_draw_sync_callback, s_otag_us
 | Params: ot -- first packet of the chain
 | Returns: N/A
 ----------------------*/
void DrawOTag(u_long *ot)
{
    unsigned long long t0 = dc_time_us();

    s_immediate_scene = 0;

    gpu_decode_ot(&s_gpu_state, ot, submit_sink, 0);
    s_otag_us += dc_time_us() - t0;
    if (s_draw_sync_callback != 0)
    {
        s_draw_sync_callback();
    }
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

    if (!pvr_submit_scene_has_prims())
    {
        s_immediate_scene = 1;
    }

    gpu_decode_packet(&s_gpu_state, (const uint32_t *)(packet + 1), gpu_tag_len((uint32_t)packet[0]), submit_sink, 0);
}

/*----------------------
 | SetPolyFT4
 | Description: Marks a packet as a flat textured quad, nine words long with GPU code 0x2C, leaving its link address and other fields untouched.
 | Author: suinevere
 | Dependencies: setPolyFT4, lom_prefix.h setlen
 | Globals: N/A
 | Params: prim -- packet to initialise
 | Returns: N/A
 ----------------------*/
void SetPolyFT4(POLY_FT4 *prim)
{
    setPolyFT4(prim);
}

/*----------------------
 | SetPolyG4
 | Description: Marks a packet as a Gouraud-shaded quad, eight words long with GPU code 0x38, leaving its link address and other fields untouched.
 | Author: suinevere
 | Dependencies: lom_prefix.h setlen, setcode
 | Globals: N/A
 | Params: prim -- packet to initialise
 | Returns: N/A
 ----------------------*/
void SetPolyG4(POLY_G4 *prim)
{
    setlen(prim, 8);
    setcode(prim, 0x38);
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
 | Description: No queued GPU work to wait for, but presents a scene drawn into the currently displayed area now instead of waiting for the next VSync flip.
 | Author: suinevere
 | Dependencies: s_async_poll, sync_flip
 | Globals: s_async_poll
 | Params: mode -- ignored
 | Returns: 0
 ----------------------*/
int DrawSync(int mode)
{
    (void)mode;
    if (s_async_poll != 0)
    {
        s_async_poll();
    }
    sync_flip();
    return 0;
}

/*----------------------
 | VSync
 | Description: Flips an open PowerVR scene on the waiting modes (0 and above 1) and paces the game on the shared vblank counter.
 | VSync(0) under a 24-bit display returns at once when a vblank has passed since the last wait, since a movie frame's software decode runs close to its four vblanks and the extra wait pushed most frames to five.
 | Then delivers due CD stream sectors and runs the hook unless this was a counter read; VSync(1) returns at once on the PS1 and games call it mid-frame, so it never flips.
 | Author: suinevere
 | Dependencies: heap_probe, pvr_submit_flip, vsync_wait, vsync_wait_after, idle_count, s_async_poll, cdrom_dc_vsync_tick
 | Globals: s_vsync_hook, s_async_poll, s_disp_env
 | Params: mode -- PS1 VSync mode
 | Returns: vblank count for mode < 0, else 0
 ----------------------*/
int VSync(int mode)
{
    int result;
    unsigned long long t0;

    heap_probe("vsync");
    if (mode == 0 || mode > 1)
    {
        pvr_submit_flip();
    }
    t0 = dc_time_us();
    result = 0;
    if (mode == 0 && s_disp_env.isrgb24)
    {
        vsync_wait_after(idle_count);
    }
    else
    {
        result = vsync_wait(mode, idle_count);
    }
    s_vsync_wait_us += dc_time_us() - t0;

    if (s_async_poll != 0)
    {
        s_async_poll();
    }
    if (mode >= 0)
    {
        cdrom_dc_vsync_tick();
    }
    if (mode >= 0 && s_vsync_hook != 0)
    {
        s_vsync_hook();
    }
    return result;
}

/*----------------------
 | vsync_wait_us
 | Description: Microseconds VSync has spent waiting for vblanks since start-up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_vsync_wait_us
 | Params: N/A
 | Returns: the total
 ----------------------*/
unsigned long long vsync_wait_us(void)
{
    return s_vsync_wait_us;
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

unsigned long long dc_otag_us(void)
{
    return s_otag_us;
}

void dc_set_vsync_idle(DcVsyncHook idle)
{
    s_vsync_idle = idle;
}

void dc_set_async_poll(DcVsyncHook poll)
{
    s_async_poll = poll;
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
 | Dependencies: gpu_decode_reset, update_origin, update_draw_matches_disp, pvr_submit_keep_screen, pvr_submit_background, pack_colour
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
    update_draw_matches_disp();
    pvr_submit_keep_screen(!env->isbg);
    if (env->isbg)
    {
        pvr_submit_background(pack_colour(env->r0, env->g0, env->b0));
    }
    return env;
}

/*----------------------
 | PutDispEnv
 | Description: Presents the outgoing frame, a scene drawn into the display area or, for a 24-bit area, its VRAM as an RGB565 frame; then records the new display environment and remembers its area alongside the previous distinct one.
 | Author: suinevere
 | Dependencies: sync_flip, rgb24_to_rgb565, pvr_submit_rgb24, update_draw_matches_disp
 | Globals: s_disp_env, s_disp_seen
 | Params: env -- display environment
 | Returns: env
 ----------------------*/
DISPENV *PutDispEnv(DISPENV *env)
{
    const RECT *r = &env->disp;
    unsigned width = r->w > RGB24_FRAME_PITCH ? RGB24_FRAME_PITCH : (unsigned)r->w;
    unsigned height = r->h > RGB24_FRAME_ROWS ? RGB24_FRAME_ROWS : (unsigned)r->h;

    if (env->isrgb24)
    {
        rgb24_to_rgb565(vram_mirror_at(0, 0), 1024, (unsigned)r->x, (unsigned)r->y, width, height, s_rgb24_frame, RGB24_FRAME_PITCH);
        pvr_submit_rgb24(s_rgb24_frame, width, height);
    }
    else
    {
        sync_flip();
    }
    s_disp_env = *env;
    if (r->x != s_disp_seen[0].x || r->y != s_disp_seen[0].y || r->w != s_disp_seen[0].w || r->h != s_disp_seen[0].h)
    {
        s_disp_seen[1] = s_disp_seen[0];
        s_disp_seen[0] = *r;
    }
    update_draw_matches_disp();
    if (!env->isrgb24)
    {
        present_immediate_scene();
    }
    return env;
}

/*----------------------
 | present_immediate_scene
 | Description: When the display moves onto an area overlapping the one an open DrawPrim-only scene was drawn in, flips it and holds it for DC_IMMEDIATE_SCENE_HOLD_VBLANKS.
 |              WMAP's loading globe brightens in three such scenes between CD loads that finish at once here, so each would otherwise never reach the screen.
 | Author: suinevere
 | Dependencies: pvr_submit_scene_has_prims, pvr_submit_flip, VSync
 | Globals: s_immediate_scene, s_draw_env, s_disp_env
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void present_immediate_scene(void)
{
    const RECT *draw = &s_draw_env.clip;
    const RECT *disp = &s_disp_env.disp;
    int i;

    if (!s_immediate_scene || !pvr_submit_scene_has_prims())
    {
        return;
    }
    if (draw->x >= disp->x + disp->w || disp->x >= draw->x + draw->w || draw->y >= disp->y + disp->h || disp->y >= draw->y + draw->h)
    {
        return;
    }
    s_immediate_scene = 0;
    pvr_submit_flip();
    for (i = 0; i < DC_IMMEDIATE_SCENE_HOLD_VBLANKS; i++)
    {
        VSync(0);
    }
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
 | A full-area clear also draws a full-area opaque rect when the scene already holds prims, since the PS1 erases them but the background alone would not.
 | Author: suinevere
 | Dependencies: vram_mirror_clear, pvr_submit_background, pvr_submit_rect, pvr_submit_scene_has_prims, pack_colour
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
        if (pvr_submit_scene_has_prims())
        {
            pvr_submit_rect(0, 0, s_gpu_state.area_x2 - s_gpu_state.area_x1 + 1, s_gpu_state.area_y2 - s_gpu_state.area_y1 + 1, colour);
        }
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
 | FRAME_READBACK_LINE_MAX
 | Description: Widest row consult_frame reads back in one frame_readback_copy call, the full mirror width.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FRAME_READBACK_LINE_MAX 1024

/*----------------------
 | s_readback_line
 | Description: One mirror row of scratch for consult_frame's readback, reused row by row to keep the call stackless.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint16_t s_readback_line[FRAME_READBACK_LINE_MAX];

/*----------------------
 | FRAME_READBACK_SCREEN_W, FRAME_READBACK_SCREEN_H
 | Description: The draw buffer's PS1-pixel size: a frame's pixels only cover VRAM [fx, fx + FRAME_READBACK_SCREEN_W) by [fy, fy + FRAME_READBACK_SCREEN_H).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FRAME_READBACK_SCREEN_W 320
#define FRAME_READBACK_SCREEN_H 240

/*----------------------
 | consult_frame
 | Description: Where rect overlaps both the last confirmed PowerVR frame and the screen it was placed on, reads those pixels back into the mirror row by row, first letting a just-closed scene that drew there reach the screen.
 | So StoreImage, MoveImage and MoveImage2 see the screen, mapping each VRAM pixel through the frame's screen origin before sampling it.
 | Author: suinevere
 | Dependencies: pvr_frame_settle, pvr_frame_rect, DrawSync, pvr_backend_frame_pixels, frame_readback_copy, vram_mirror_invalidate
 | Globals: s_readback_line
 | Params: rect -- VRAM rectangle about to be read or moved
 | Returns: N/A
 ----------------------*/
static void consult_frame(const RECT *rect)
{
    RECT frame;
    int16_t origin_x, origin_y;
    int fx, fy;
    int x1, y1, x2, y2;
    unsigned pitch;
    const uint16_t *pixels;
    int row;
    int w;

    pvr_frame_settle(rect->x, rect->y, rect->w, rect->h);
    if (!pvr_frame_rect(&frame.x, &frame.y, &frame.w, &frame.h, &origin_x, &origin_y))
    {
        return;
    }
    fx = frame.x - origin_x;
    fy = frame.y - origin_y;
    x1 = rect->x > frame.x ? rect->x : frame.x;
    x1 = fx > x1 ? fx : x1;
    y1 = rect->y > frame.y ? rect->y : frame.y;
    y1 = fy > y1 ? fy : y1;
    x2 = rect->x + rect->w - 1;
    x2 = (frame.x + frame.w - 1) < x2 ? (frame.x + frame.w - 1) : x2;
    x2 = (fx + FRAME_READBACK_SCREEN_W - 1) < x2 ? (fx + FRAME_READBACK_SCREEN_W - 1) : x2;
    y2 = rect->y + rect->h - 1;
    y2 = (frame.y + frame.h - 1) < y2 ? (frame.y + frame.h - 1) : y2;
    y2 = (fy + FRAME_READBACK_SCREEN_H - 1) < y2 ? (fy + FRAME_READBACK_SCREEN_H - 1) : y2;
    if (x1 > x2 || y1 > y2)
    {
        return;
    }
    DrawSync(0);
    pixels = pvr_backend_frame_pixels(&pitch);
    if (pixels == 0)
    {
        return;
    }
    w = x2 - x1 + 1;
    if (w > FRAME_READBACK_LINE_MAX)
    {
        w = FRAME_READBACK_LINE_MAX;
    }
    for (row = y1; row <= y2; row++)
    {
        frame_readback_copy(pixels, pitch, fx, fy, s_readback_line, x1, row, w, 1);
        vram_mirror_invalidate(x1, row, w, 1, s_readback_line);
    }
}

/*----------------------
 | StoreImage
 | Description: Reads VRAM mirror pixels out of a rectangle, first reading back any overlap with the last finished PowerVR frame.
 | Author: suinevere
 | Dependencies: consult_frame, vram_mirror_store
 | Globals: N/A
 | Params: rect -- VRAM rectangle, data -- destination buffer
 | Returns: 0
 ----------------------*/
int StoreImage(RECT *rect, u_long *data)
{
    consult_frame(rect);
    vram_mirror_store(rect, data);
    return 0;
}

/*----------------------
 | MoveImage
 | Description: Copies a VRAM mirror rectangle to a new position, first reading back any overlap of the source with the last finished PowerVR frame.
 | Author: suinevere
 | Dependencies: consult_frame, vram_mirror_move
 | Globals: N/A
 | Params: rect -- source rectangle, x, y -- destination origin
 | Returns: 0
 ----------------------*/
int MoveImage(RECT *rect, int x, int y)
{
    consult_frame(rect);
    vram_mirror_move(rect, x, y);
    return 0;
}

/*----------------------
 | MoveImage2
 | Description: MoveImage followed by DrawSync(0), for callers that need the move visible before continuing.
 | Author: suinevere
 | Dependencies: MoveImage, DrawSync
 | Globals: N/A
 | Params: rect -- source rectangle, x, y -- destination origin
 | Returns: 0
 ----------------------*/
int MoveImage2(RECT *rect, int x, int y)
{
    MoveImage(rect, x, y);
    DrawSync(0);
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

/*----------------------
 | DrawSyncCallback
 | Description: Installs the handler run when a DrawOTag finishes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_draw_sync_callback
 | Params: func -- handler or NULL
 | Returns: the previous handler
 ----------------------*/
u_long DrawSyncCallback(void (*func)(void))
{
    u_long previous = (u_long)(uintptr_t)s_draw_sync_callback;

    s_draw_sync_callback = func;
    return previous;
}

/*----------------------
 | BreakDraw
 | Description: Drawing finishes inside DrawOTag, so there is never a chain to interrupt.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: NULL, nothing interrupted
 ----------------------*/
u_long *BreakDraw(void)
{
    return 0;
}

/*----------------------
 | LoadImage2
 | Description: The PS1's immediate upload, which on the Dreamcast is the same mirror write as LoadImage.
 | Author: suinevere
 | Dependencies: LoadImage
 | Globals: N/A
 | Params: rect -- VRAM rectangle in words, src -- pixels
 | Returns: 0
 ----------------------*/
int LoadImage2(RECT *rect, u_long *src)
{
    return LoadImage(rect, src);
}
