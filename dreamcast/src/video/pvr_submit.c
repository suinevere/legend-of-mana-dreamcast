/*----------------------
 | pvr_submit.c
 | Description: Scene policy for PowerVR submission: clips, textures and blends PS1 primitives into one translucent list at 2x, opening a scene on first use and closing it at the flip. Remembers the finished frame's draw area.
 | Author: suinevere
 | Dependencies: pvr_submit.h, pvr_backend.h, clip.h, blend.h, palette_bank.h, texture_cache.h, frame_probe.h
 | Globals: s_backend, s_scene_open, s_frame, s_scenes_closed, s_origin_x, s_origin_y, s_header, s_header_valid, s_scene_has_prims, s_scene_area, s_scene_area_valid, s_pending_rect, s_pending_rect_valid, s_frame_rect, s_frame_rect_valid
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "pvr_submit.h"
#include "pvr_backend.h"
#include "clip.h"
#include "blend.h"
#include "palette_bank.h"
#include "texture_cache.h"
#include "frame_probe.h"

/*----------------------
 | PVR_SCALE
 | Description: PS1 pixels to Dreamcast pixels: 320x240 draws at 640x480.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PVR_SCALE 2.0f

/*----------------------
 | PVR_PAGE_TEXELS
 | Description: Width and height of every cached texture, one PS1 texture page.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PVR_PAGE_TEXELS 256.0f

/*----------------------
 | HeaderKey
 | Description: The polygon state a header encodes, compared to skip re-sending an identical header.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    int textured;
    void *texture;
    int format;
    int palette_slot;
    uint8_t src, dst;
} HeaderKey;

/*----------------------
 | s_backend
 | Description: The hardware hooks; the Dreamcast build binds the KallistiOS backend in pvr_submit_init.
 | Author: suinevere
 | Dependencies: PvrBackend
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const PvrBackend *s_backend;

/*----------------------
 | s_scene_open
 | Description: Set between the first draw or clear after a flip and the next flip.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_scene_open;

/*----------------------
 | s_frame
 | Description: Scene number passed to the texture and palette caches; increments once per scene.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_frame;

/*----------------------
 | s_scenes_closed
 | Description: Scenes handed to the PowerVR so far.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_scenes_closed;

/*----------------------
 | s_origin_x, s_origin_y
 | Description: Screen position of the draw area's top-left corner in PS1 pixels.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static float s_origin_x, s_origin_y;

/*----------------------
 | s_header
 | Description: The state of the last header sent in this scene.
 | Author: suinevere
 | Dependencies: HeaderKey
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static HeaderKey s_header;

/*----------------------
 | s_header_valid
 | Description: Set once a header has been sent in the open scene.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_header_valid;

/*----------------------
 | s_scene_has_prims
 | Description: Set once a triangle strip has been sent in the open scene.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_scene_has_prims;

/*----------------------
 | FrameSnapshot
 | Description: A draw-area rect plus the screen origin it was placed at, the unit s_scene_area, s_pending_rect and s_frame_rect share.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    RECT rect;
    int16_t origin_x, origin_y;
} FrameSnapshot;

/*----------------------
 | s_scene_area
 | Description: The draw area and origin of the most recent primitive submitted in the open scene.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static FrameSnapshot s_scene_area;

/*----------------------
 | s_scene_area_valid
 | Description: Set once a primitive has recorded s_scene_area in the open scene.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_scene_area_valid;

/*----------------------
 | s_pending_rect
 | Description: The closed scene's snapshot, not yet confirmed to have reached the screen.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static FrameSnapshot s_pending_rect;

/*----------------------
 | s_pending_rect_valid
 | Description: Set once pvr_submit_flip has recorded s_pending_rect, cleared once it is promoted to s_frame_rect.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_pending_rect_valid;

/*----------------------
 | s_frame_rect
 | Description: The snapshot of the last scene confirmed to have reached the screen.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static FrameSnapshot s_frame_rect;

/*----------------------
 | s_frame_rect_valid
 | Description: Set once s_frame_rect holds a real on-screen frame's snapshot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_frame_rect_valid;

/*----------------------
 | ps1_channel
 | Description: Expands one 5-bit PS1 channel to 8 bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- channel 0-31
 | Returns: channel 0-255
 ----------------------*/
static uint8_t ps1_channel(unsigned c)
{
    return (uint8_t)((c << 3) | (c >> 2));
}

/*----------------------
 | promote_pending_rect
 | Description: Moves s_pending_rect into s_frame_rect, the caller having already confirmed that scene reached the screen.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_pending_rect, s_pending_rect_valid, s_frame_rect, s_frame_rect_valid
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void promote_pending_rect(void)
{
    s_frame_rect = s_pending_rect;
    s_frame_rect_valid = 1;
    s_pending_rect_valid = 0;
}

/*----------------------
 | open_scene
 | Description: Starts a scene through the backend if none is open and starts a new cache frame.
 | Skips the pending texture flush, and promoting s_pending_rect, when the backend reports its flip wait expired, leaving both pending for a scene that catches up.
 | Author: suinevere
 | Dependencies: PvrBackend.scene_begin, texture_cache_begin_frame, promote_pending_rect
 | Globals: s_backend, s_scene_open, s_scenes_closed, s_frame, s_header_valid, s_pending_rect_valid
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void open_scene(void)
{
    int flip_ok;

    if (s_scene_open)
    {
        return;
    }
    flip_ok = s_backend->scene_begin(s_scenes_closed);
    s_frame++;
    if (flip_ok)
    {
        texture_cache_begin_frame();
        if (s_pending_rect_valid)
        {
            promote_pending_rect();
        }
    }
    s_header_valid = 0;
    s_scene_has_prims = 0;
    s_scene_open = 1;
}

/*----------------------
 | send_header
 | Description: Sends a polygon header for this texture and blend pass unless the last header sent in the scene was identical.
 | Author: suinevere
 | Dependencies: PvrBackend.header
 | Globals: s_backend, s_header, s_header_valid
 | Params: tex -- texture, or null for untextured, pass -- blend pass
 | Returns: N/A
 ----------------------*/
static void send_header(const DcSurfaceTexture *tex, const BlendPass *pass)
{
    HeaderKey key;

    key.textured = tex != 0;
    key.texture = tex != 0 ? tex->texture : 0;
    key.format = tex != 0 ? tex->format : 0;
    key.palette_slot = tex != 0 ? tex->palette_slot : 0;
    key.src = pass->src;
    key.dst = pass->dst;
    if (s_header_valid && key.textured == s_header.textured && key.texture == s_header.texture &&
        key.format == s_header.format && key.palette_slot == s_header.palette_slot &&
        key.src == s_header.src && key.dst == s_header.dst)
    {
        return;
    }
    s_backend->header(tex, pass);
    s_header = key;
    s_header_valid = 1;
}

/*----------------------
 | send_strip
 | Description: Sends a clipped polygon as one triangle strip at 2x from the draw area's origin, with texel coordinates normalised to the page and a fixed alpha.
 | Vertices are white for a coverage pass, or the tint above 1.0 for an excess pass.
 | Author: suinevere
 | Dependencies: clip_strip_order, blend_vertex_colour, blend_excess_colour, PvrBackend.vertex
 | Globals: s_backend, s_origin_x, s_origin_y, s_scene_has_prims
 | Params: poly -- clipped polygon, alpha -- vertex alpha 0-255, textured -- primitive samples a texture, raw -- texture is untinted, cover -- draw white coverage instead of the vertex colour, excess -- draw the tint blend_vertex_colour clamps away
 | Returns: N/A
 ----------------------*/
static void send_strip(const DcPoly *poly, uint8_t alpha, int textured, int raw, int cover, int excess)
{
    uint8_t (*colour)(uint8_t, int, int) = excess ? blend_excess_colour : blend_vertex_colour;
    uint8_t order[8];
    unsigned n = clip_strip_order(poly->count, order);
    unsigned i;

    s_scene_has_prims = 1;
    for (i = 0; i < n; i++)
    {
        const DcClipVertex *cv = &poly->v[order[i]];
        uint32_t argb = ((uint32_t)alpha << 24) | ((uint32_t)colour(cv->r, textured, raw) << 16) |
                        ((uint32_t)colour(cv->g, textured, raw) << 8) | colour(cv->b, textured, raw);

        if (cover)
        {
            argb |= 0x00FFFFFFu;
        }

        s_backend->vertex((cv->x + s_origin_x) * PVR_SCALE, (cv->y + s_origin_y) * PVR_SCALE,
                          cv->u / PVR_PAGE_TEXELS, cv->v / PVR_PAGE_TEXELS, argb, i + 1 == n);
    }
}

/*----------------------
 | has_excess
 | Description: Whether any vertex of a clipped polygon tints its texture above 1.0, so its colour passes need excess passes.
 | Author: suinevere
 | Dependencies: blend_excess_colour
 | Globals: N/A
 | Params: poly -- clipped polygon, textured -- primitive samples a texture, raw -- texture is untinted
 | Returns: 1 if an excess pass would draw anything, else 0
 ----------------------*/
static int has_excess(const DcPoly *poly, int textured, int raw)
{
    unsigned i;

    for (i = 0; i < poly->count; i++)
    {
        const DcClipVertex *cv = &poly->v[i];

        if (blend_excess_colour(cv->r, textured, raw) | blend_excess_colour(cv->g, textured, raw) |
            blend_excess_colour(cv->b, textured, raw))
        {
            return 1;
        }
    }
    return 0;
}

/*----------------------
 | pvr_submit_set_backend
 | Description: Binds the hardware hooks every later call goes through.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_backend
 | Params: backend -- hooks, kept by pointer
 | Returns: N/A
 ----------------------*/
void pvr_submit_set_backend(const PvrBackend *backend)
{
    s_backend = backend;
}

/*----------------------
 | pvr_submit_prim
 | Description: Clips one primitive to the draw area and writes it as up to four blended triangle strips, opening the scene first if none is open.
 | Each colour pass is followed by an excess pass when the texture tint exceeds 1.0.
 | Author: suinevere
 | Dependencies: clip_prim, palette_clut_stp, open_scene, blend_plan, texture_cache_lookup, send_header, send_strip, has_excess, blend_excess_pass, frame_probe_prim
 | Globals: s_frame, s_origin_x, s_origin_y, s_scene_area, s_scene_area_valid
 | Params: prim -- decoded primitive, state -- GPU state carrying the draw area
 | Returns: N/A
 ----------------------*/
void pvr_submit_prim(const DcPrim *prim, const DcGpuState *state)
{
    DcPoly poly;
    BlendPass passes[BLEND_MAX_PASSES];
    BlendPass excess;
    DcSurfaceTexture tex[BLEND_MAX_PASSES];
    unsigned count;
    unsigned i;
    int stp = STP_ALL;
    int brighten;

    s_scene_area.rect.x = state->area_x1;
    s_scene_area.rect.y = state->area_y1;
    s_scene_area.rect.w = (short)(state->area_x2 - state->area_x1 + 1);
    s_scene_area.rect.h = (short)(state->area_y2 - state->area_y1 + 1);
    s_scene_area.origin_x = (int16_t)s_origin_x;
    s_scene_area.origin_y = (int16_t)s_origin_y;
    s_scene_area_valid = 1;
    if (clip_prim(prim, state, &poly) == 0)
    {
        return;
    }
    if (prim->textured && prim->depth != DC_DEPTH_15)
    {
        stp = palette_clut_stp(prim->clut_x, prim->clut_y, prim->depth);
    }
    open_scene();
    count = blend_plan(prim->textured, prim->semi, prim->abr, stp, prim->depth != DC_DEPTH_15, passes);
    if (prim->textured)
    {
        for (i = 0; i < count; i++)
        {
            if (texture_cache_lookup(prim->tpage_x, prim->tpage_y, prim->clut_x, prim->clut_y, prim->depth,
                                     passes[i].variant, s_frame, &tex[i]) < 0)
            {
                return;
            }
        }
    }
    brighten = has_excess(&poly, prim->textured, prim->raw);
    for (i = 0; i < count; i++)
    {
        send_header(prim->textured ? &tex[i] : 0, &passes[i]);
        send_strip(&poly, passes[i].alpha, prim->textured, prim->raw, passes[i].cover, 0);
        if (brighten && blend_excess_pass(&passes[i], &excess))
        {
            send_header(&tex[i], &excess);
            send_strip(&poly, excess.alpha, prim->textured, prim->raw, 0, 1);
        }
    }
    frame_probe_prim();
}

/*----------------------
 | pvr_submit_flip
 | Description: Finishes the scene if one is open, so the PowerVR renders it, counts the scene for the probe, and hands its snapshot to s_pending_rect to confirm later.
 | Author: suinevere
 | Dependencies: PvrBackend.scene_finish, frame_probe_scene
 | Globals: s_backend, s_scene_open, s_scenes_closed, s_scene_area, s_scene_area_valid, s_pending_rect, s_pending_rect_valid
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void pvr_submit_flip(void)
{
    if (!s_scene_open)
    {
        return;
    }
    s_backend->scene_finish();
    s_scene_open = 0;
    s_scenes_closed++;
    if (s_scene_area_valid)
    {
        s_pending_rect = s_scene_area;
        s_pending_rect_valid = 1;
    }
    s_scene_area_valid = 0;
    frame_probe_scene();
}

/*----------------------
 | pvr_submit_origin
 | Description: Sets where the draw area's top-left corner lands on the 320x240 screen, for every later primitive and rect.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_origin_x, s_origin_y
 | Params: x, y -- screen position in PS1 pixels
 | Returns: N/A
 ----------------------*/
void pvr_submit_origin(int x, int y)
{
    s_origin_x = (float)x;
    s_origin_y = (float)y;
}

/*----------------------
 | pvr_submit_background
 | Description: Opens the scene and sets the colour it clears to, so a frame with only a clear still renders at the next flip.
 | Author: suinevere
 | Dependencies: open_scene, ps1_channel, PvrBackend.background
 | Globals: s_backend
 | Params: ps1_colour -- PS1 15-bit colour
 | Returns: N/A
 ----------------------*/
void pvr_submit_background(uint16_t ps1_colour)
{
    open_scene();
    s_backend->background(ps1_channel(ps1_colour & 0x1Fu), ps1_channel((ps1_colour >> 5) & 0x1Fu),
                          ps1_channel((ps1_colour >> 10) & 0x1Fu));
}

/*----------------------
 | pvr_submit_rect
 | Description: Opens the scene and draws an opaque untextured rectangle, standing in for a ClearImage that only covers part of the draw area.
 | Author: suinevere
 | Dependencies: open_scene, ps1_channel, send_header, send_strip
 | Globals: N/A
 | Params: x, y -- top-left relative to the draw area in PS1 pixels, w, h -- size, ps1_colour -- PS1 15-bit colour
 | Returns: N/A
 ----------------------*/
void pvr_submit_rect(int x, int y, int w, int h, uint16_t ps1_colour)
{
    BlendPass pass = { BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_OPAQUE, 0 };
    DcPoly poly;
    uint8_t r = ps1_channel(ps1_colour & 0x1Fu);
    uint8_t g = ps1_channel((ps1_colour >> 5) & 0x1Fu);
    uint8_t b = ps1_channel((ps1_colour >> 10) & 0x1Fu);
    unsigned i;

    poly.count = 4;
    poly.v[0].x = (float)x;
    poly.v[0].y = (float)y;
    poly.v[1].x = (float)(x + w);
    poly.v[1].y = (float)y;
    poly.v[2].x = (float)(x + w);
    poly.v[2].y = (float)(y + h);
    poly.v[3].x = (float)x;
    poly.v[3].y = (float)(y + h);
    for (i = 0; i < 4; i++)
    {
        poly.v[i].u = 0.0f;
        poly.v[i].v = 0.0f;
        poly.v[i].r = r;
        poly.v[i].g = g;
        poly.v[i].b = b;
    }
    open_scene();
    send_header(0, &pass);
    send_strip(&poly, pass.alpha, 0, 0, 0, 0);
}

/*----------------------
 | RGB24_TEXTURE_WIDTH / RGB24_TEXTURE_HEIGHT
 | Description: Size of the texture a 24-bit display frame is loaded into, for its texel coordinates.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RGB24_TEXTURE_WIDTH 512.0f
#define RGB24_TEXTURE_HEIGHT 256.0f

/*----------------------
 | pvr_submit_rgb24
 | Description: Closes any open scene, loads the frame into its texture, and shows it as one opaque full-screen quad in a scene of its own.
 | Author: suinevere
 | Dependencies: pvr_submit_flip, PvrBackend.frame_texture, open_scene, send_header, PvrBackend.vertex
 | Globals: s_backend, s_scene_has_prims
 | Params: pixels -- RGB565 rows at a 512-pixel pitch, width, height -- frame size in pixels
 | Returns: N/A
 ----------------------*/
void pvr_submit_rgb24(const uint16_t *pixels, unsigned width, unsigned height)
{
    BlendPass pass = { BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_OPAQUE, 0 };
    const DcSurfaceTexture *tex;
    float u = (float)width / RGB24_TEXTURE_WIDTH;
    float v = (float)height / RGB24_TEXTURE_HEIGHT;

    pvr_submit_flip();
    tex = s_backend->frame_texture(pixels, width, height);
    if (tex == 0)
    {
        return;
    }
    open_scene();
    send_header(tex, &pass);
    s_backend->vertex(0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFFu, 0);
    s_backend->vertex(640.0f, 0.0f, u, 0.0f, 0xFFFFFFFFu, 0);
    s_backend->vertex(0.0f, 480.0f, 0.0f, v, 0xFFFFFFFFu, 0);
    s_backend->vertex(640.0f, 480.0f, u, v, 0xFFFFFFFFu, 1);
    s_scene_has_prims = 1;
    pvr_submit_flip();
}

/*----------------------
 | pvr_submit_scene_has_prims
 | Description: Reports whether the open scene has had a triangle strip sent since it opened.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_scene_has_prims
 | Params: N/A
 | Returns: 1 if a strip was sent this scene, else 0
 ----------------------*/
int pvr_submit_scene_has_prims(void)
{
    return s_scene_has_prims;
}

/*----------------------
 | pvr_frame_rect
 | Description: Returns the PS1 draw-area rect and screen origin of the last scene confirmed on screen, as plain fields so this header still compiles under KallistiOS-only translation units.
 | Promotes a still-pending scene first if PvrBackend.scene_shown now reports it reached the screen, since a caller may ask before the next scene's open_scene gets the chance.
 | Author: suinevere
 | Dependencies: PvrBackend.scene_shown, promote_pending_rect
 | Globals: s_backend, s_pending_rect_valid, s_scenes_closed, s_frame_rect, s_frame_rect_valid
 | Params: x, y, w, h, origin_x, origin_y -- outputs, filled when a confirmed frame exists
 | Returns: 1 if a confirmed frame's rect was returned, 0 if there is none yet
 ----------------------*/
int pvr_frame_rect(int16_t *x, int16_t *y, int16_t *w, int16_t *h, int16_t *origin_x, int16_t *origin_y)
{
    if (s_pending_rect_valid && s_backend->scene_shown(s_scenes_closed))
    {
        promote_pending_rect();
    }
    if (!s_frame_rect_valid)
    {
        return 0;
    }
    *x = s_frame_rect.rect.x;
    *y = s_frame_rect.rect.y;
    *w = s_frame_rect.rect.w;
    *h = s_frame_rect.rect.h;
    *origin_x = s_frame_rect.origin_x;
    *origin_y = s_frame_rect.origin_y;
    return 1;
}
