/*----------------------
 | pvr_backend_kos.c
 | Description: KallistiOS PowerVR backend for pvr_submit: one autosort-off translucent list, recorded in RAM and copied to the TA when the scene closes, the texture and palette memory hooks, and the displayed framebuffer for readback.
 | Author: suinevere
 | Dependencies: kos.h, pvr_submit.h, pvr_backend.h, texture_cache.h, palette_bank.h, blend.h, dc_system.h
 | Globals: s_flip_base, s_settle_closed, s_settled, s_dr, s_dr_active, s_blend_factor, s_texture_ops, s_palette_ops, s_kos_backend
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <kos.h>

#include "pvr_submit.h"
#include "pvr_backend.h"
#include "texture_cache.h"
#include "palette_bank.h"
#include "blend.h"
#include "dc_system.h"

/*----------------------
 | PVR_VERTEX_BUFFER_BYTES
 | Description: Size of each TA vertex buffer in video memory.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PVR_VERTEX_BUFFER_BYTES (512 * 1024)

/*----------------------
 | PVR_OPB_OVERFLOW
 | Description: Extra object-pointer blocks so tiles crowded by the single translucent list do not drop geometry.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PVR_OPB_OVERFLOW 2

/*----------------------
 | PVR_TEXTURE_SIZE
 | Description: Width and height of every cached texture, one PS1 texture page.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PVR_TEXTURE_SIZE 256

/*----------------------
 | PVR_FLIP_WAIT_VBLANKS
 | Description: Longest wait for the previous scene to reach the screen before a new scene starts anyway.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PVR_FLIP_WAIT_VBLANKS 4u

/*----------------------
 | s_flip_base
 | Description: The PowerVR's page-flip count at pvr_submit_init, so flips can be matched to scenes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static size_t s_flip_base;

/*----------------------
 | s_settle_closed
 | Description: Scenes closed when the open scene began, the flip count settle_previous_scene waits for.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_settle_closed;

/*----------------------
 | s_settled
 | Description: Set once the open scene has confirmed or given up waiting for the previous flip, so it waits at most once.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_settled = 1;

static void settle_previous_scene(void);

/*----------------------
 | s_dr
 | Description: Store-queue direct-rendering cursor for the open scene.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static pvr_dr_state_t s_dr;

/*----------------------
 | s_dr_active
 | Description: Set while the store queues are locked for direct rendering; texture uploads release them first.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_dr_active;

/*----------------------
 | s_blend_factor
 | Description: PowerVR blend factor for each BL_* value; PVR_BLEND_INVDESTCOLOR means one minus the other colour, the source's as a destination factor and the destination's as a source factor.
 | Author: suinevere
 | Dependencies: blend.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const int s_blend_factor[] = { PVR_BLEND_ZERO, PVR_BLEND_ONE, PVR_BLEND_SRCALPHA, PVR_BLEND_INVSRCALPHA, PVR_BLEND_INVDESTCOLOR,
                                      PVR_BLEND_INVDESTCOLOR };

/*----------------------
 | dr_begin
 | Description: Locks the store queues for direct rendering if a texture upload released them.
 | Author: suinevere
 | Dependencies: pvr_dr_init
 | Globals: s_dr, s_dr_active
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void dr_begin(void)
{
    if (!s_dr_active)
    {
        pvr_dr_init(&s_dr);
        s_dr_active = 1;
    }
}

/*----------------------
 | dr_suspend
 | Description: Releases the store queues so pvr_txr_load can take them; the next write re-locks them.
 | Needed because sq_lock is a non-recursive mutex shared by direct rendering and texture uploads.
 | Author: suinevere
 | Dependencies: pvr_dr_finish
 | Globals: s_dr_active
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void dr_suspend(void)
{
    if (s_dr_active)
    {
        pvr_dr_finish();
        s_dr_active = 0;
    }
}

/*----------------------
 | FRAME_TEXTURE_WIDTH / FRAME_TEXTURE_HEIGHT / s_frame_texture / s_frame_surface
 | Description: The non-twiddled RGB565 texture a 24-bit display frame is shown from, allocated on first use and again after texture memory is reset, and its lookup result.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FRAME_TEXTURE_WIDTH 512
#define FRAME_TEXTURE_HEIGHT 256
static pvr_ptr_t s_frame_texture;
static DcSurfaceTexture s_frame_surface;

/*----------------------
 | SCREEN_CAPTURE_W / SCREEN_CAPTURE_H / s_screen_textures / s_screen_surfaces
 | Description: The two screen slots capture_screen fills with the shown frame at PS1 resolution, in frame-texture layout, one per PS1 draw buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SCREEN_CAPTURE_W 320
#define SCREEN_CAPTURE_H 240
static pvr_ptr_t s_screen_textures[2];
static DcSurfaceTexture s_screen_surfaces[2];

/*----------------------
 | texture_alloc
 | Description: TextureOps.alloc backed by the PowerVR texture allocator.
 | Author: suinevere
 | Dependencies: pvr_mem_malloc
 | Globals: N/A
 | Params: bytes -- size
 | Returns: texture memory, or null when full
 ----------------------*/
static void *texture_alloc(unsigned bytes)
{
    return pvr_mem_malloc(bytes);
}

/*----------------------
 | texture_free_all
 | Description: TextureOps.free_all: forgets every texture allocation at once, the frame texture included, after the previous scene stops reading them.
 | Author: suinevere
 | Dependencies: settle_previous_scene, pvr_mem_reset
 | Globals: s_frame_texture
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void texture_free_all(void)
{
    settle_previous_scene();
    s_frame_texture = 0;
    s_screen_textures[0] = 0;
    s_screen_textures[1] = 0;
    pvr_mem_reset();
}

/*----------------------
 | DIRECT_UPLOAD_MAX_BYTES
 | Description: Largest upload copied with CPU word writes instead of the store queues, sized for a dirty 16x16 ARGB1555 square.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DIRECT_UPLOAD_MAX_BYTES 512u

/*----------------------
 | texture_write
 | Description: TextureOps.write: copies into texture memory no submitted scene reads, through the store queues, or for a run of at most DIRECT_UPLOAD_MAX_BYTES with plain word writes that leave direct rendering open.
 | Releases the direct-rendering store-queue lock before a store-queue copy, because pvr_txr_load takes the same non-recursive sq_lock and would deadlock.
 | Author: suinevere
 | Dependencies: dr_suspend, pvr_txr_load
 | Globals: N/A
 | Params: dst -- texture memory, src -- bytes, bytes -- count, a multiple of 32
 | Returns: N/A
 ----------------------*/
static void texture_write(void *dst, const void *src, unsigned bytes)
{
    if (bytes <= DIRECT_UPLOAD_MAX_BYTES)
    {
        volatile uint32_t *out = (volatile uint32_t *)dst;
        const uint32_t *in = (const uint32_t *)src;
        unsigned i;

        for (i = 0; i < bytes / 4u; i++)
        {
            out[i] = in[i];
        }
        return;
    }
    dr_suspend();
    pvr_txr_load((void *)src, (pvr_ptr_t)dst, bytes);
}

/*----------------------
 | texture_upload
 | Description: TextureOps.upload: waits until the previous scene stops reading texture memory, then copies as texture_write does.
 | Author: suinevere
 | Dependencies: settle_previous_scene, texture_write
 | Globals: N/A
 | Params: dst -- texture memory, src -- bytes, bytes -- count
 | Returns: N/A
 ----------------------*/
static void texture_upload(void *dst, const void *src, unsigned bytes)
{
    settle_previous_scene();
    texture_write(dst, src, bytes);
}

/*----------------------
 | kos_frame_texture
 | Description: PvrBackend.frame_texture: loads a 24-bit display frame into its own texture, flushing the texture cache once if texture memory is full.
 | Author: suinevere
 | Dependencies: pvr_mem_malloc, texture_cache_flush_now, texture_upload
 | Globals: s_frame_texture, s_frame_surface
 | Params: pixels -- RGB565 rows at a 512-pixel pitch, width -- unused, height -- rows to load
 | Returns: the frame surface, or NULL when no texture memory is free
 ----------------------*/
static const DcSurfaceTexture *kos_frame_texture(const uint16_t *pixels, unsigned width, unsigned height)
{
    (void)width;
    if (s_frame_texture == 0)
    {
        s_frame_texture = pvr_mem_malloc(FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT * 2);
        if (s_frame_texture == 0)
        {
            texture_cache_flush_now();
            s_frame_texture = pvr_mem_malloc(FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT * 2);
        }
        if (s_frame_texture == 0)
        {
            return 0;
        }
    }
    texture_upload(s_frame_texture, pixels, FRAME_TEXTURE_WIDTH * 2 * height);
    s_frame_surface.texture = s_frame_texture;
    s_frame_surface.format = TEX_FMT_RGB565_FRAME;
    s_frame_surface.palette_slot = -1;
    return &s_frame_surface;
}

/*----------------------
 | kos_capture_screen
 | Description: PvrBackend.capture_screen: copies every other pixel of every other row of the shown 640x480 frame into a screen slot's texture, row by row.
 | Author: suinevere
 | Dependencies: pvr_mem_malloc, texture_cache_flush_now, texture_upload, vram_l, vid_mode
 | Globals: s_screen_textures, s_screen_surfaces
 | Params: slot -- 0 or 1
 | Returns: 1 once the slot holds the frame, 0 when no texture memory is free
 ----------------------*/
static int kos_capture_screen(unsigned slot)
{
    static uint16_t line[SCREEN_CAPTURE_W] __attribute__((aligned(32)));
    const uint16_t *shown = (const uint16_t *)vram_l;
    unsigned pitch = (unsigned)vid_mode->width;
    uint8_t *dst;
    unsigned x, y;

    if (slot > 1u)
    {
        return 0;
    }
    if (s_screen_textures[slot] == 0)
    {
        s_screen_textures[slot] = pvr_mem_malloc(FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT * 2);
        if (s_screen_textures[slot] == 0)
        {
            texture_cache_flush_now();
            s_screen_textures[slot] = pvr_mem_malloc(FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT * 2);
        }
        if (s_screen_textures[slot] == 0)
        {
            return 0;
        }
    }
    dst = (uint8_t *)s_screen_textures[slot];
    for (y = 0; y < SCREEN_CAPTURE_H; y++)
    {
        const uint16_t *row = shown + (2u * y) * pitch;

        for (x = 0; x < SCREEN_CAPTURE_W; x++)
        {
            line[x] = row[2u * x];
        }
        texture_upload(dst + y * FRAME_TEXTURE_WIDTH * 2u, line, sizeof(line));
    }
    s_screen_surfaces[slot].texture = s_screen_textures[slot];
    s_screen_surfaces[slot].format = TEX_FMT_RGB565_FRAME;
    s_screen_surfaces[slot].palette_slot = -1;
    return 1;
}

/*----------------------
 | kos_screen_texture
 | Description: PvrBackend.screen_texture: the surface of a screen slot's last capture.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_screen_textures, s_screen_surfaces
 | Params: slot -- 0 or 1
 | Returns: the slot's surface, or NULL when it was never captured or texture memory was reset since
 ----------------------*/
static const DcSurfaceTexture *kos_screen_texture(unsigned slot)
{
    if (slot > 1u || s_screen_textures[slot] == 0)
    {
        return 0;
    }
    return &s_screen_surfaces[slot];
}

/*----------------------
 | palette_set_entry
 | Description: PaletteOps.set_entry: writes one ARGB8888 palette RAM entry, after the previous scene stops reading palette RAM.
 | Author: suinevere
 | Dependencies: settle_previous_scene, pvr_set_pal_entry
 | Globals: N/A
 | Params: index -- entry 0-1023, argb8888 -- colour
 | Returns: N/A
 ----------------------*/
static void palette_set_entry(unsigned index, uint32_t argb8888)
{
    settle_previous_scene();
    pvr_set_pal_entry(index, argb8888);
}

/*----------------------
 | s_texture_ops
 | Description: Texture memory backend handed to texture_cache_init.
 | Author: suinevere
 | Dependencies: texture_alloc, texture_free_all, texture_upload, texture_write
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const TextureOps s_texture_ops = { texture_alloc, texture_free_all, texture_upload, texture_write };

/*----------------------
 | s_palette_ops
 | Description: Palette RAM backend handed to palette_bank_init.
 | Author: suinevere
 | Dependencies: palette_set_entry
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const PaletteOps s_palette_ops = { palette_set_entry };

/*----------------------
 | wait_previous_flip
 | Description: Waits until every closed scene has been rendered and shown, so the new scene may reuse its texture memory and palette entries.
 | KallistiOS's pvr_wait_ready returns when a render starts, not when it finishes, so the page-flip count is the safe signal.
 | Author: suinevere
 | Dependencies: pvr_get_stats, thd_pass
 | Globals: s_flip_base
 | Params: scenes_closed -- scenes handed to the PowerVR so far
 | Returns: 1 if the previous scene reached the screen before the vblank guard ran out, 0 if the wait expired
 ----------------------*/
static int wait_previous_flip(unsigned scenes_closed)
{
    pvr_stats_t stats;
    size_t start;

    pvr_get_stats(&stats);
    start = stats.vbl_count;
    while (stats.frame_count - s_flip_base < scenes_closed && stats.vbl_count - start < PVR_FLIP_WAIT_VBLANKS)
    {
        thd_pass();
        pvr_get_stats(&stats);
    }
    return stats.frame_count - s_flip_base >= scenes_closed;
}

/*----------------------
 | kos_scene_shown
 | Description: Reports wait_previous_flip's condition without waiting for it, so a caller outside the scene loop can learn a scene reached the screen.
 | Author: suinevere
 | Dependencies: pvr_get_stats
 | Globals: s_flip_base
 | Params: scenes_closed -- scenes handed to the PowerVR so far
 | Returns: 1 if that many scenes have reached the screen, else 0
 ----------------------*/
static int kos_scene_shown(unsigned scenes_closed)
{
    pvr_stats_t stats;

    pvr_get_stats(&stats);
    return stats.frame_count - s_flip_base >= scenes_closed;
}

/*----------------------
 | settle_previous_scene
 | Description: Before the open scene's first write to texture memory or palette RAM, waits once for the previous scene to reach the screen so its render no longer reads them.
 | Author: suinevere
 | Dependencies: wait_previous_flip, dc_log
 | Globals: s_settle_closed, s_settled
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void settle_previous_scene(void)
{
    if (s_settled)
    {
        return;
    }
    s_settled = 1;
    if (!wait_previous_flip(s_settle_closed))
    {
        dc_log("PVR: flip wait expired");
    }
}

/*----------------------
 | s_pvr_wait_us
 | Description: Microseconds spent waiting for the PowerVR to finish the previous scene before opening a new one.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned long long s_pvr_wait_us;

unsigned long long pvr_backend_wait_us(void)
{
    return s_pvr_wait_us;
}

/*----------------------
 | s_render_max_us
 | Description: Longest PowerVR render KallistiOS reported at any scene start since the last pvr_backend_take_render_max_us.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_render_max_us;

unsigned pvr_backend_take_render_max_us(void)
{
    unsigned max = s_render_max_us;

    s_render_max_us = 0;
    return max;
}

/*----------------------
 | CMD_BUFFER_BYTES / s_cmd / s_cmd_words / s_cmd_capacity / s_streaming / s_begin_closed
 | Description: The open scene's TA commands recorded in main RAM and replayed in one store-queue copy when the scene closes; past capacity the scene starts on the hardware and the rest streams straight through.
 |              Recording means only the close waits for the previous render to start, which KallistiOS does at a vblank, instead of the next frame's first primitive stalling the game up to a vblank.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CMD_BUFFER_BYTES (256u * 1024u)
static uint32_t *s_cmd;
static unsigned s_cmd_words;
static unsigned s_cmd_capacity;
static int s_streaming;
static unsigned s_begin_closed;

/*----------------------
 | s_bg_pending / s_bg
 | Description: A background colour set while recording, applied once the scene starts on the hardware so it cannot reach a scene still waiting to render.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_bg_pending;
static float s_bg[3];

/*----------------------
 | kos_scene_begin
 | Description: Opens a scene for recording without touching the PowerVR, waiting for the previous flip only when a texture flush is pending; later texture and palette writes settle lazily.
 | Author: suinevere
 | Dependencies: texture_cache_flush_pending, settle_previous_scene, kos_scene_shown
 | Globals: s_settle_closed, s_settled, s_cmd_words, s_streaming, s_begin_closed
 | Params: scenes_closed -- scenes handed to the PowerVR so far
 | Returns: 1 if every closed scene is confirmed on screen, else 0
 ----------------------*/
static int kos_scene_begin(unsigned scenes_closed)
{
    int flip_ok = 1;

    s_settle_closed = scenes_closed;
    s_settled = 1;
    s_begin_closed = scenes_closed;
    s_cmd_words = 0;
    s_streaming = 0;
    if (scenes_closed != 0)
    {
        s_settled = 0;
        if (texture_cache_flush_pending())
        {
            settle_previous_scene();
        }
        flip_ok = kos_scene_shown(scenes_closed);
        s_settled = s_settled || flip_ok;
    }
    return flip_ok;
}

/*----------------------
 | start_hardware_scene
 | Description: Waits for the TA buffer (the previous scene's render to start), applies a pending background, begins the scene and its translucent list and replays the recorded commands.
 |              The first scene skips pvr_wait_ready, whose semaphore starts at zero.
 | Author: suinevere
 | Dependencies: pvr_wait_ready, pvr_get_stats, pvr_set_bg_color, pvr_scene_begin, pvr_list_begin, pvr_prim
 | Globals: s_begin_closed, s_pvr_wait_us, s_render_max_us, s_bg_pending, s_bg, s_dr_active, s_cmd, s_cmd_words, s_streaming
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void start_hardware_scene(void)
{
    if (s_begin_closed != 0)
    {
        uint64 t0 = timer_us_gettime64();
        pvr_stats_t stats;

        pvr_wait_ready();
        s_pvr_wait_us += timer_us_gettime64() - t0;
        pvr_get_stats(&stats);
        if (stats.rnd_last_time / 1000u > s_render_max_us)
        {
            s_render_max_us = (unsigned)(stats.rnd_last_time / 1000u);
        }
    }
    if (s_bg_pending)
    {
        pvr_set_bg_color(s_bg[0], s_bg[1], s_bg[2]);
        s_bg_pending = 0;
    }
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);
    s_dr_active = 0;
    if (s_cmd_words != 0)
    {
        pvr_prim(s_cmd, (int)(s_cmd_words * 4u));
        s_cmd_words = 0;
    }
    s_streaming = 1;
}

/*----------------------
 | emit
 | Description: Appends one 32-byte TA command to the recording, or writes it through the store queues once the scene streams.
 | Author: suinevere
 | Dependencies: start_hardware_scene, dr_begin, pvr_dr_target, pvr_dr_commit
 | Globals: s_cmd, s_cmd_words, s_cmd_capacity, s_streaming, s_dr
 | Params: words -- the command's eight words
 | Returns: N/A
 ----------------------*/
static void emit(const uint32_t *words)
{
    uint32_t *dst;
    int i;

    if (!s_streaming && s_cmd_words + 8u > s_cmd_capacity)
    {
        start_hardware_scene();
    }
    if (s_streaming)
    {
        dr_begin();
        dst = (uint32_t *)pvr_dr_target(s_dr);
        for (i = 0; i < 8; i++)
        {
            dst[i] = words[i];
        }
        pvr_dr_commit(dst);
        return;
    }
    dst = &s_cmd[s_cmd_words];
    for (i = 0; i < 8; i++)
    {
        dst[i] = words[i];
    }
    s_cmd_words += 8u;
}

/*----------------------
 | kos_strip_mark
 | Description: PvrBackend.strip_mark: the recording slot the next command lands in, or NULL once the scene streams.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_cmd, s_cmd_words, s_cmd_capacity, s_streaming
 | Params: N/A
 | Returns: the slot, or NULL
 ----------------------*/
static void *kos_strip_mark(void)
{
    if (s_streaming || s_cmd_words + 8u > s_cmd_capacity)
    {
        return NULL;
    }
    return &s_cmd[s_cmd_words];
}

/*----------------------
 | kos_patch_strip
 | Description: PvrBackend.patch_strip: rewrites the x and y of the recorded vertices starting at a mark, each a pvr_vertex_t of eight words.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mark -- from kos_strip_mark, verts -- new positions, count -- vertices
 | Returns: N/A
 ----------------------*/
static void kos_patch_strip(void *mark, const PvrVertex *verts, unsigned count)
{
    pvr_vertex_t *vert = (pvr_vertex_t *)mark;
    unsigned i;

    for (i = 0; i < count; i++)
    {
        vert[i].x = verts[i].x;
        vert[i].y = verts[i].y;
    }
}

/*----------------------
 | kos_scene_finish
 | Description: Starts the recorded scene on the hardware unless it already streams, then finishes the translucent list and the scene, which also releases the store queues.
 | Author: suinevere
 | Dependencies: start_hardware_scene, pvr_list_finish, pvr_scene_finish
 | Globals: s_streaming, s_dr_active
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void kos_scene_finish(void)
{
    if (!s_streaming)
    {
        start_hardware_scene();
    }
    pvr_list_finish();
    pvr_scene_finish();
    s_dr_active = 0;
    s_streaming = 0;
}

/*----------------------
 | kos_background
 | Description: Sets the colour the PowerVR clears the scene to, at once while the scene streams, else when it starts on the hardware.
 | Author: suinevere
 | Dependencies: pvr_set_bg_color
 | Globals: s_bg, s_bg_pending, s_streaming
 | Params: r, g, b -- 8-bit channels
 | Returns: N/A
 ----------------------*/
static void kos_background(uint8_t r, uint8_t g, uint8_t b)
{
    s_bg[0] = r / 255.0f;
    s_bg[1] = g / 255.0f;
    s_bg[2] = b / 255.0f;
    if (s_streaming)
    {
        pvr_set_bg_color(s_bg[0], s_bg[1], s_bg[2]);
        return;
    }
    s_bg_pending = 1;
}

/*----------------------
 | texture_format
 | Description: Builds the PowerVR texture format word for a cached texture, including its palette selector.
 | Author: suinevere
 | Dependencies: TEX_FMT_*
 | Globals: N/A
 | Params: tex -- lookup result
 | Returns: format word for pvr_poly_cxt_txr
 ----------------------*/
static int texture_format(const DcSurfaceTexture *tex)
{
    if (tex->format == TEX_FMT_PAL4)
    {
        return PVR_TXRFMT_PAL4BPP | PVR_TXRFMT_TWIDDLED | PVR_TXRFMT_4BPP_PAL(tex->palette_slot);
    }
    if (tex->format == TEX_FMT_PAL8)
    {
        return PVR_TXRFMT_PAL8BPP | PVR_TXRFMT_TWIDDLED | PVR_TXRFMT_8BPP_PAL(tex->palette_slot >> 4);
    }
    if (tex->format == TEX_FMT_RGB565_FRAME)
    {
        return PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED;
    }
    return PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_TWIDDLED;
}

/*----------------------
 | HEADER_CACHE_ENTRIES / HeaderCacheEntry / s_header_cache
 | Description: Compiled polygon headers kept by texture, format, palette slot and blend factors, a header being fully determined by them; direct-mapped, a clash recompiles.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define HEADER_CACHE_ENTRIES 256u

typedef struct
{
    void *texture;
    int format;
    int palette_slot;
    uint8_t src;
    uint8_t dst;
    uint8_t textured;
    uint8_t valid;
    pvr_poly_hdr_t hdr;
} HeaderCacheEntry;

static HeaderCacheEntry s_header_cache[HEADER_CACHE_ENTRIES] __attribute__((aligned(32)));

/*----------------------
 | header_cache_entry
 | Description: The cache slot for a texture and blend pass, claimed and marked invalid when it held a different key.
 | Author: suinevere
 | Dependencies: s_header_cache
 | Globals: s_header_cache
 | Params: tex -- texture, or null for untextured, pass -- blend pass
 | Returns: the slot
 ----------------------*/
static HeaderCacheEntry *header_cache_entry(const DcSurfaceTexture *tex, const BlendPass *pass)
{
    void *texture = tex != 0 ? tex->texture : 0;
    int format = tex != 0 ? tex->format : 0;
    int slot = tex != 0 ? tex->palette_slot : 0;
    uintptr_t hash = ((uintptr_t)texture >> 5) ^ ((uintptr_t)format * 31u) ^ ((uintptr_t)slot * 7u) ^ ((uintptr_t)pass->src << 3) ^ ((uintptr_t)pass->dst << 6);
    HeaderCacheEntry *entry = &s_header_cache[hash & (HEADER_CACHE_ENTRIES - 1u)];

    if (entry->texture != texture || entry->format != format || entry->palette_slot != slot || entry->src != (uint8_t)pass->src ||
        entry->dst != (uint8_t)pass->dst || entry->textured != (tex != 0))
    {
        entry->texture = texture;
        entry->format = format;
        entry->palette_slot = slot;
        entry->src = (uint8_t)pass->src;
        entry->dst = (uint8_t)pass->dst;
        entry->textured = (uint8_t)(tex != 0);
        entry->valid = 0;
    }
    return entry;
}

/*----------------------
 | kos_header
 | Description: Sends a point-sampled, depth-always, unculled polygon header with the pass's blend factors, compiling it only the first time its texture and blend are seen.
 | Author: suinevere
 | Dependencies: header_cache_entry, pvr_poly_cxt_txr, pvr_poly_cxt_col, pvr_poly_compile, texture_format, emit
 | Globals: s_dr, s_blend_factor
 | Params: tex -- texture, or null for untextured, pass -- blend pass
 | Returns: N/A
 ----------------------*/
static void kos_header(const DcSurfaceTexture *tex, const BlendPass *pass)
{
    pvr_poly_cxt_t cxt;
    HeaderCacheEntry *entry = header_cache_entry(tex, pass);
    const uint32_t *src = (const uint32_t *)&entry->hdr;

    if (entry->valid)
    {
        emit(src);
        return;
    }
    if (tex != 0)
    {
        int frame = tex->format == TEX_FMT_RGB565_FRAME;

        pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, texture_format(tex), frame ? FRAME_TEXTURE_WIDTH : PVR_TEXTURE_SIZE,
                         frame ? FRAME_TEXTURE_HEIGHT : PVR_TEXTURE_SIZE, (pvr_ptr_t)tex->texture, PVR_FILTER_NONE);
        cxt.txr.env = PVR_TXRENV_MODULATEALPHA;
    }
    else
    {
        pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    }
    cxt.gen.culling = PVR_CULLING_NONE;
    cxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
    cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
    cxt.blend.src = s_blend_factor[pass->src];
    cxt.blend.dst = s_blend_factor[pass->dst];
    pvr_poly_compile(&entry->hdr, &cxt);
    entry->valid = 1;
    emit(src);
}

/*----------------------
 | kos_strip
 | Description: Sends a triangle strip's vertices at z = 1, ending the strip on the last.
 | Author: suinevere
 | Dependencies: emit
 | Globals: s_dr
 | Params: verts -- strip vertices, count -- how many
 | Returns: N/A
 ----------------------*/
static void kos_strip(const PvrVertex *verts, unsigned count)
{
    pvr_vertex_t vert __attribute__((aligned(32)));
    unsigned i;

    vert.z = 1.0f;
    vert.oargb = 0;
    for (i = 0; i < count; i++)
    {
        vert.flags = i + 1 == count ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
        vert.x = verts[i].x;
        vert.y = verts[i].y;
        vert.u = verts[i].u;
        vert.v = verts[i].v;
        vert.argb = verts[i].argb;
        emit((const uint32_t *)&vert);
    }
}

/*----------------------
 | s_kos_backend
 | Description: The KallistiOS hooks pvr_submit_init binds.
 | Author: suinevere
 | Dependencies: kos_scene_begin, kos_scene_finish, kos_background, kos_header, kos_strip, kos_scene_shown, wait_previous_flip, kos_frame_texture, kos_strip_mark, kos_patch_strip, kos_capture_screen, kos_screen_texture
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const PvrBackend s_kos_backend = { kos_scene_begin, kos_scene_finish, kos_background, kos_header, kos_strip, kos_scene_shown, wait_previous_flip, kos_frame_texture,
                                          kos_strip_mark, kos_patch_strip, kos_capture_screen, kos_screen_texture };

/*----------------------
 | pvr_backend_frame_pixels
 | Description: Returns the displayed 640x480 RGB565 buffer through the 32-bit VRAM view; vid_set_start moves it only at vblank, so it always points at an already-rendered frame.
 | Author: suinevere
 | Dependencies: vram_l, vid_mode
 | Globals: N/A
 | Params: pitch_px -- output, pixels per row
 | Returns: pointer to the displayed buffer
 ----------------------*/
const uint16_t *pvr_backend_frame_pixels(unsigned *pitch_px)
{
    *pitch_px = (unsigned)vid_mode->width;
    return (const uint16_t *)vram_l;
}

/*----------------------
 | pvr_submit_init
 | Description: Initialises the PowerVR with only the translucent list sized and autosort off, ARGB8888 palettes and a black background, and binds the caches and the backend.
 | Author: suinevere
 | Dependencies: pvr_init, memalign, pvr_set_pal_format, pvr_set_bg_color, pvr_get_stats, texture_cache_init, palette_bank_init, pvr_submit_set_backend
 | Globals: s_flip_base, s_cmd, s_cmd_capacity, s_texture_ops, s_palette_ops, s_kos_backend
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void pvr_submit_init(void)
{
    pvr_init_params_t params = {
        .opb_sizes = { PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_0 },
        .vertex_buf_size = PVR_VERTEX_BUFFER_BYTES,
        .dma_enabled = 0,
        .fsaa_enabled = 0,
        .autosort_disabled = 1,
        .opb_overflow_count = PVR_OPB_OVERFLOW,
    };
    pvr_stats_t stats;

    pvr_init(&params);
    s_cmd = memalign(32, CMD_BUFFER_BYTES);
    s_cmd_capacity = s_cmd != NULL ? CMD_BUFFER_BYTES / 4u : 0u;
    pvr_set_pal_format(PVR_PAL_ARGB8888);
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);
    texture_cache_init(&s_texture_ops);
    palette_bank_init(&s_palette_ops);
    pvr_get_stats(&stats);
    s_flip_base = stats.frame_count;
    pvr_submit_set_backend(&s_kos_backend);
}
