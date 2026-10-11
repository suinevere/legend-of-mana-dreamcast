/*----------------------
 | pvr_backend_kos.c
 | Description: KallistiOS PowerVR backend for pvr_submit: one autosort-off translucent list written through the store queues, plus the texture and palette memory hooks.
 | Author: suinevere
 | Dependencies: kos.h, pvr_submit.h, pvr_backend.h, texture_cache.h, palette_bank.h, blend.h
 | Globals: s_flip_base, s_dr, s_dr_active, s_blend_factor, s_texture_ops, s_palette_ops, s_kos_backend
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <kos.h>

#include "pvr_submit.h"
#include "pvr_backend.h"
#include "texture_cache.h"
#include "palette_bank.h"
#include "blend.h"

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
 | Description: TextureOps.free_all: forgets every texture allocation at once.
 | Author: suinevere
 | Dependencies: pvr_mem_reset
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void texture_free_all(void)
{
    pvr_mem_reset();
}

/*----------------------
 | texture_upload
 | Description: TextureOps.upload: copies a built texture into texture memory through the store queues.
 | Releases the direct-rendering store-queue lock first, because pvr_txr_load takes the same non-recursive sq_lock and would deadlock.
 | Author: suinevere
 | Dependencies: dr_suspend, pvr_txr_load
 | Globals: N/A
 | Params: dst -- texture memory, src -- built texture, bytes -- size
 | Returns: N/A
 ----------------------*/
static void texture_upload(void *dst, const void *src, unsigned bytes)
{
    dr_suspend();
    pvr_txr_load((void *)src, (pvr_ptr_t)dst, bytes);
}

/*----------------------
 | palette_set_entry
 | Description: PaletteOps.set_entry: writes one ARGB8888 palette RAM entry.
 | Author: suinevere
 | Dependencies: pvr_set_pal_entry
 | Globals: N/A
 | Params: index -- entry 0-1023, argb8888 -- colour
 | Returns: N/A
 ----------------------*/
static void palette_set_entry(unsigned index, uint32_t argb8888)
{
    pvr_set_pal_entry(index, argb8888);
}

/*----------------------
 | s_texture_ops
 | Description: Texture memory backend handed to texture_cache_init.
 | Author: suinevere
 | Dependencies: texture_alloc, texture_free_all, texture_upload
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const TextureOps s_texture_ops = { texture_alloc, texture_free_all, texture_upload };

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
 | Returns: N/A
 ----------------------*/
static void wait_previous_flip(unsigned scenes_closed)
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
}

/*----------------------
 | kos_scene_begin
 | Description: Starts a scene and its translucent list once the previous scene is on screen; the first scene skips pvr_wait_ready, whose semaphore starts at zero.
 | Author: suinevere
 | Dependencies: pvr_wait_ready, wait_previous_flip, pvr_scene_begin, pvr_list_begin
 | Globals: s_dr_active
 | Params: scenes_closed -- scenes handed to the PowerVR so far
 | Returns: N/A
 ----------------------*/
static void kos_scene_begin(unsigned scenes_closed)
{
    if (scenes_closed != 0)
    {
        pvr_wait_ready();
        wait_previous_flip(scenes_closed);
    }
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);
    s_dr_active = 0;
}

/*----------------------
 | kos_scene_finish
 | Description: Finishes the translucent list and the scene, which also releases the store queues.
 | Author: suinevere
 | Dependencies: pvr_list_finish, pvr_scene_finish
 | Globals: s_dr_active
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void kos_scene_finish(void)
{
    pvr_list_finish();
    pvr_scene_finish();
    s_dr_active = 0;
}

/*----------------------
 | kos_background
 | Description: Sets the colour the PowerVR clears the scene to.
 | Author: suinevere
 | Dependencies: pvr_set_bg_color
 | Globals: N/A
 | Params: r, g, b -- 8-bit channels
 | Returns: N/A
 ----------------------*/
static void kos_background(uint8_t r, uint8_t g, uint8_t b)
{
    pvr_set_bg_color(r / 255.0f, g / 255.0f, b / 255.0f);
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
    return PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_TWIDDLED;
}

/*----------------------
 | kos_header
 | Description: Compiles and sends a point-sampled, depth-always, unculled polygon header with the pass's blend factors.
 | Author: suinevere
 | Dependencies: pvr_poly_cxt_txr, pvr_poly_cxt_col, pvr_poly_compile, texture_format, dr_begin, pvr_dr_target, pvr_dr_commit
 | Globals: s_dr, s_blend_factor
 | Params: tex -- texture, or null for untextured, pass -- blend pass
 | Returns: N/A
 ----------------------*/
static void kos_header(const DcSurfaceTexture *tex, const BlendPass *pass)
{
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    const uint32_t *src = (const uint32_t *)&hdr;
    uint32_t *dst;
    int i;

    if (tex != 0)
    {
        pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, texture_format(tex), PVR_TEXTURE_SIZE, PVR_TEXTURE_SIZE,
                         (pvr_ptr_t)tex->texture, PVR_FILTER_NONE);
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
    pvr_poly_compile(&hdr, &cxt);
    dr_begin();
    dst = (uint32_t *)pvr_dr_target(s_dr);
    for (i = 0; i < 8; i++)
    {
        dst[i] = src[i];
    }
    pvr_dr_commit(dst);
}

/*----------------------
 | kos_vertex
 | Description: Sends one strip vertex at z = 1 through the store queues, ending the strip when last is set.
 | Author: suinevere
 | Dependencies: dr_begin, pvr_dr_target, pvr_dr_commit
 | Globals: s_dr
 | Params: x, y -- screen pixels, u, v -- page-normalised texel coordinates, argb -- colour, last -- end of strip
 | Returns: N/A
 ----------------------*/
static void kos_vertex(float x, float y, float u, float v, uint32_t argb, int last)
{
    pvr_vertex_t *vert;

    dr_begin();
    vert = pvr_dr_target(s_dr);
    vert->flags = last ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
    vert->x = x;
    vert->y = y;
    vert->z = 1.0f;
    vert->u = u;
    vert->v = v;
    vert->argb = argb;
    vert->oargb = 0;
    pvr_dr_commit(vert);
}

/*----------------------
 | s_kos_backend
 | Description: The KallistiOS hooks pvr_submit_init binds.
 | Author: suinevere
 | Dependencies: kos_scene_begin, kos_scene_finish, kos_background, kos_header, kos_vertex
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const PvrBackend s_kos_backend = { kos_scene_begin, kos_scene_finish, kos_background, kos_header, kos_vertex };

/*----------------------
 | pvr_submit_init
 | Description: Initialises the PowerVR with only the translucent list sized and autosort off, ARGB8888 palettes and a black background, and binds the caches and the backend.
 | Author: suinevere
 | Dependencies: pvr_init, pvr_set_pal_format, pvr_set_bg_color, pvr_get_stats, texture_cache_init, palette_bank_init, pvr_submit_set_backend
 | Globals: s_flip_base, s_texture_ops, s_palette_ops, s_kos_backend
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
    pvr_set_pal_format(PVR_PAL_ARGB8888);
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);
    texture_cache_init(&s_texture_ops);
    palette_bank_init(&s_palette_ops);
    pvr_get_stats(&stats);
    s_flip_base = stats.frame_count;
    pvr_submit_set_backend(&s_kos_backend);
}
