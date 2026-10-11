/*----------------------
 | clip.h
 | Description: Pure-C CPU clipping of decoded PS1 primitives to the draw area, producing screen-space polygons plus triangle-strip vertex ordering.
 | Author: suinevere
 | Dependencies: stdint.h, gpu_decode.h (DcPrim, DcGpuState)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_DC_CLIP_H
#define LOM_DC_CLIP_H

#include <stdint.h>

#include "gpu_decode.h"

/*----------------------
 | DC_CLIP_MAX_VERTS
 | Description: Largest vertex count clip_prim can produce: a quad clipped against all four draw-area edges.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DC_CLIP_MAX_VERTS 8

/*----------------------
 | DcClipVertex
 | Description: One clipped vertex: float screen position (relative to the draw area) and texel coordinates, plus a tint colour.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    float x, y, u, v;
    uint8_t r, g, b;
} DcClipVertex;

/*----------------------
 | DcPoly
 | Description: A clipped screen-space polygon: vertex count and up to DC_CLIP_MAX_VERTS vertices in perimeter order.
 | Author: suinevere
 | Dependencies: DcClipVertex
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    unsigned count;
    DcClipVertex v[DC_CLIP_MAX_VERTS];
} DcPoly;

/*----------------------
 | clip_prim
 | Description: Clips one decoded primitive to state's draw area, writing a screen-space polygon relative to the area's top-left corner.
 | Author: suinevere
 | Dependencies: DcPrim, DcGpuState, DcPoly
 | Globals: N/A
 | Params: prim -- decoded primitive, state -- GPU state carrying the draw area, out -- clipped polygon output
 | Returns: out->count; 0 when nothing is visible
 ----------------------*/
int clip_prim(const DcPrim *prim, const DcGpuState *state, DcPoly *out);

/*----------------------
 | clip_strip_order
 | Description: Turns a convex polygon's perimeter vertex order into a triangle-strip order (0, 1, n-1, 2, n-2, ...).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: count -- polygon vertex count (at most 8), order -- output strip-order indices, at least 8 entries
 | Returns: count
 ----------------------*/
unsigned clip_strip_order(unsigned count, uint8_t order[8]);

#endif
