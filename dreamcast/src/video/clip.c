/*----------------------
 | clip.c
 | Description: Clips decoded PS1 primitives to the draw area in float screen space and turns a clipped polygon's vertex order into a triangle strip.
 | Author: suinevere
 | Dependencies: clip.h, gpu_decode.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "clip.h"

/*----------------------
 | ClipVertF
 | Description: One polygon vertex during clipping: full-float position, texel coordinates and colour, for lossless Sutherland-Hodgman interpolation.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    float x, y, u, v, r, g, b;
} ClipVertF;

/*----------------------
 | DC_QUAD_PERIM_ORDER
 | Description: Maps a PS1 quad's top-left/top-right/bottom-left/bottom-right vertex order onto perimeter order for clipping.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const uint8_t DC_QUAD_PERIM_ORDER[4] = { 0, 1, 3, 2 };

/*----------------------
 | round_u8
 | Description: Rounds a float colour component to the nearest byte, clamping to 0..255.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: x -- colour component value
 | Returns: clamped, rounded byte
 ----------------------*/
static uint8_t round_u8(float x)
{
    float r = x + 0.5f;

    if (r < 0.0f)
    {
        return 0;
    }
    if (r > 255.0f)
    {
        return 255;
    }
    return (uint8_t)r;
}

/*----------------------
 | sh_clip_edge
 | Description: Runs one Sutherland-Hodgman pass of a convex polygon against the half-plane a*x + b*y + c >= 0, interpolating u, v, r, g, b at each crossing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: in -- input polygon, n -- input vertex count, out -- output polygon (at least DC_CLIP_MAX_VERTS entries), a, b, c -- half-plane coefficients
 | Returns: output vertex count
 ----------------------*/
static unsigned sh_clip_edge(const ClipVertF *in, unsigned n, ClipVertF *out, float a, float b, float c)
{
    unsigned count = 0;
    unsigned i;

    for (i = 0; i < n; i++)
    {
        const ClipVertF *cur = &in[i];
        const ClipVertF *prev = &in[(i + n - 1) % n];
        float cur_d = a * cur->x + b * cur->y + c;
        float prev_d = a * prev->x + b * prev->y + c;
        int cur_in = cur_d >= 0.0f;
        int prev_in = prev_d >= 0.0f;

        if (cur_in != prev_in)
        {
            float t = prev_d / (prev_d - cur_d);

            if (count < DC_CLIP_MAX_VERTS)
            {
                out[count].x = prev->x + t * (cur->x - prev->x);
                out[count].y = prev->y + t * (cur->y - prev->y);
                out[count].u = prev->u + t * (cur->u - prev->u);
                out[count].v = prev->v + t * (cur->v - prev->v);
                out[count].r = prev->r + t * (cur->r - prev->r);
                out[count].g = prev->g + t * (cur->g - prev->g);
                out[count].b = prev->b + t * (cur->b - prev->b);
                count++;
            }
        }
        if (cur_in && count < DC_CLIP_MAX_VERTS)
        {
            out[count++] = *cur;
        }
    }
    return count;
}

/*----------------------
 | sh_clip
 | Description: Clips a convex polygon against the draw area's four edges in sequence, left, right, top, then bottom.
 | Author: suinevere
 | Dependencies: sh_clip_edge
 | Globals: N/A
 | Params: verts -- input polygon, n -- input vertex count, w, h -- draw area width and height, result -- output polygon (at least DC_CLIP_MAX_VERTS entries)
 | Returns: output vertex count
 ----------------------*/
static unsigned sh_clip(const ClipVertF *verts, unsigned n, float w, float h, ClipVertF *result)
{
    ClipVertF buf_a[DC_CLIP_MAX_VERTS];
    ClipVertF buf_b[DC_CLIP_MAX_VERTS];
    const ClipVertF *src = verts;
    ClipVertF *dst = buf_a;
    unsigned i;

    n = sh_clip_edge(src, n, dst, 1.0f, 0.0f, 0.0f);
    src = dst;
    dst = buf_b;
    n = sh_clip_edge(src, n, dst, -1.0f, 0.0f, w);
    src = dst;
    dst = buf_a;
    n = sh_clip_edge(src, n, dst, 0.0f, 1.0f, 0.0f);
    src = dst;
    dst = buf_b;
    n = sh_clip_edge(src, n, dst, 0.0f, -1.0f, h);
    for (i = 0; i < n; i++)
    {
        result[i] = dst[i];
    }
    return n;
}

/*----------------------
 | finish_poly
 | Description: Converts a clipped float polygon into out's DcClipVertex form, rounding colour to bytes.
 | Author: suinevere
 | Dependencies: round_u8
 | Globals: N/A
 | Params: verts -- clipped float polygon, n -- vertex count, out -- polygon output
 | Returns: out->count (0 when n is 0)
 ----------------------*/
static int finish_poly(const ClipVertF *verts, unsigned n, DcPoly *out)
{
    unsigned i;

    out->count = n;
    for (i = 0; i < n; i++)
    {
        out->v[i].x = verts[i].x;
        out->v[i].y = verts[i].y;
        out->v[i].u = verts[i].u;
        out->v[i].v = verts[i].v;
        out->v[i].r = round_u8(verts[i].r);
        out->v[i].g = round_u8(verts[i].g);
        out->v[i].b = round_u8(verts[i].b);
    }
    return (int)out->count;
}

/*----------------------
 | clip_polygon
 | Description: Clips a triangle or quad to the draw area, reordering a quad's vertices into perimeter order first.
 | Author: suinevere
 | Dependencies: DC_QUAD_PERIM_ORDER, sh_clip, finish_poly
 | Globals: N/A
 | Params: prim -- decoded triangle or quad, state -- GPU state carrying the draw area, out -- clipped polygon output
 | Returns: out->count
 ----------------------*/
static int clip_polygon(const DcPrim *prim, const DcGpuState *state, DcPoly *out)
{
    ClipVertF verts[4];
    ClipVertF result[DC_CLIP_MAX_VERTS];
    float w = (float)(state->area_x2 - state->area_x1 + 1);
    float h = (float)(state->area_y2 - state->area_y1 + 1);
    unsigned n = prim->nverts;
    unsigned i;
    unsigned clipped_n;

    for (i = 0; i < n; i++)
    {
        unsigned src = (prim->shape == DC_SHAPE_QUAD) ? DC_QUAD_PERIM_ORDER[i] : i;
        const DcVertex *sv = &prim->v[src];

        verts[i].x = (float)sv->x - (float)state->area_x1;
        verts[i].y = (float)sv->y - (float)state->area_y1;
        verts[i].u = (float)sv->u;
        verts[i].v = (float)sv->v;
        verts[i].r = (float)sv->r;
        verts[i].g = (float)sv->g;
        verts[i].b = (float)sv->b;
    }
    clipped_n = sh_clip(verts, n, w, h, result);
    return finish_poly(result, clipped_n, out);
}

/*----------------------
 | clip_rect
 | Description: Trims a two-vertex rect directly against the draw area, shifting its UV origin and end by the trimmed amount instead of reading the decoded, possibly wrapped end UV.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: prim -- decoded rect, state -- GPU state carrying the draw area, out -- clipped polygon output
 | Returns: out->count (0 when fully outside)
 ----------------------*/
static int clip_rect(const DcPrim *prim, const DcGpuState *state, DcPoly *out)
{
    float w = (float)(state->area_x2 - state->area_x1 + 1);
    float h = (float)(state->area_y2 - state->area_y1 + 1);
    float x0 = (float)prim->v[0].x - (float)state->area_x1;
    float y0 = (float)prim->v[0].y - (float)state->area_y1;
    float x1 = (float)prim->v[1].x - (float)state->area_x1;
    float y1 = (float)prim->v[1].y - (float)state->area_y1;
    float u0 = (float)prim->v[0].u;
    float v0 = (float)prim->v[0].v;
    float cx0 = x0 < 0.0f ? 0.0f : x0;
    float cy0 = y0 < 0.0f ? 0.0f : y0;
    float cx1 = x1 > w ? w : x1;
    float cy1 = y1 > h ? h : y1;
    float cu0, cu1, cv0, cv1;

    if (cx1 <= cx0 || cy1 <= cy0)
    {
        out->count = 0;
        return 0;
    }
    cu0 = u0 + (cx0 - x0);
    cu1 = u0 + (cx1 - x0);
    cv0 = v0 + (cy0 - y0);
    cv1 = v0 + (cy1 - y0);
    out->count = 4;
    out->v[0].x = cx0;
    out->v[0].y = cy0;
    out->v[0].u = cu0;
    out->v[0].v = cv0;
    out->v[1].x = cx1;
    out->v[1].y = cy0;
    out->v[1].u = cu1;
    out->v[1].v = cv0;
    out->v[2].x = cx1;
    out->v[2].y = cy1;
    out->v[2].u = cu1;
    out->v[2].v = cv1;
    out->v[3].x = cx0;
    out->v[3].y = cy1;
    out->v[3].u = cu0;
    out->v[3].v = cv1;
    out->v[0].r = out->v[1].r = out->v[2].r = out->v[3].r = prim->v[0].r;
    out->v[0].g = out->v[1].g = out->v[2].g = out->v[3].g = prim->v[0].g;
    out->v[0].b = out->v[1].b = out->v[2].b = out->v[3].b = prim->v[0].b;
    return 4;
}

/*----------------------
 | clip_line
 | Description: Thickens a two-vertex line into a one-pixel-wide quad, perpendicular to whichever axis it runs closer to, then clips that quad to the draw area.
 | Author: suinevere
 | Dependencies: sh_clip, finish_poly
 | Globals: N/A
 | Params: prim -- decoded line, state -- GPU state carrying the draw area, out -- clipped polygon output
 | Returns: out->count
 ----------------------*/
static int clip_line(const DcPrim *prim, const DcGpuState *state, DcPoly *out)
{
    ClipVertF verts[4];
    ClipVertF result[DC_CLIP_MAX_VERTS];
    float w = (float)(state->area_x2 - state->area_x1 + 1);
    float h = (float)(state->area_y2 - state->area_y1 + 1);
    float x0 = (float)prim->v[0].x - (float)state->area_x1;
    float y0 = (float)prim->v[0].y - (float)state->area_y1;
    float x1 = (float)prim->v[1].x - (float)state->area_x1;
    float y1 = (float)prim->v[1].y - (float)state->area_y1;
    float dx = x1 - x0;
    float dy = y1 - y0;
    float adx = dx < 0.0f ? -dx : dx;
    float ady = dy < 0.0f ? -dy : dy;
    float ox = 0.0f;
    float oy = 0.0f;
    unsigned clipped_n;

    if (ady <= adx)
    {
        oy = 0.5f;
    }
    else
    {
        ox = 0.5f;
    }
    verts[0].x = x0 - ox;
    verts[0].y = y0 - oy;
    verts[1].x = x1 - ox;
    verts[1].y = y1 - oy;
    verts[2].x = x1 + ox;
    verts[2].y = y1 + oy;
    verts[3].x = x0 + ox;
    verts[3].y = y0 + oy;
    verts[0].u = verts[1].u = verts[2].u = verts[3].u = 0.0f;
    verts[0].v = verts[1].v = verts[2].v = verts[3].v = 0.0f;
    verts[0].r = verts[3].r = (float)prim->v[0].r;
    verts[0].g = verts[3].g = (float)prim->v[0].g;
    verts[0].b = verts[3].b = (float)prim->v[0].b;
    verts[1].r = verts[2].r = (float)prim->v[1].r;
    verts[1].g = verts[2].g = (float)prim->v[1].g;
    verts[1].b = verts[2].b = (float)prim->v[1].b;
    clipped_n = sh_clip(verts, 4, w, h, result);
    return finish_poly(result, clipped_n, out);
}

/*----------------------
 | clip_prim
 | Description: Clips one decoded primitive to state's draw area, writing a screen-space polygon relative to the area's top-left corner.
 | Author: suinevere
 | Dependencies: clip_rect, clip_line, clip_polygon
 | Globals: N/A
 | Params: prim -- decoded primitive, state -- GPU state carrying the draw area, out -- clipped polygon output
 | Returns: out->count; 0 when nothing is visible
 ----------------------*/
int clip_prim(const DcPrim *prim, const DcGpuState *state, DcPoly *out)
{
    switch (prim->shape)
    {
    case DC_SHAPE_RECT:
        return clip_rect(prim, state, out);
    case DC_SHAPE_LINE:
        return clip_line(prim, state, out);
    default:
        return clip_polygon(prim, state, out);
    }
}

/*----------------------
 | clip_strip_order
 | Description: Turns a convex polygon's perimeter vertex order into a triangle-strip order (0, 1, n-1, 2, n-2, ...).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: count -- polygon vertex count (at most 8), order -- output strip-order indices, at least 8 entries
 | Returns: count
 ----------------------*/
unsigned clip_strip_order(unsigned count, uint8_t order[8])
{
    unsigned n = 0;
    unsigned lo;
    unsigned hi;

    if (count == 0)
    {
        return 0;
    }
    order[n++] = 0;
    lo = 1;
    hi = count - 1;
    while (lo <= hi)
    {
        order[n++] = (uint8_t)lo;
        lo++;
        if (lo <= hi)
        {
            order[n++] = (uint8_t)hi;
            hi--;
        }
    }
    return n;
}
