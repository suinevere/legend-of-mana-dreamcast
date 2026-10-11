/*----------------------
 | gpu_decode.h
 | Description: Decodes PS1 GP0 ordering-table packets into shape-neutral DcPrim primitives for a later PowerVR submitter, tracking state across E1-E5 environment commands.
 | Author: suinevere
 | Dependencies: stdint.h, lom_prefix.h (u_long)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_GPU_DECODE_H
#define LOM_GPU_DECODE_H

#include <stdint.h>

/*----------------------
 | DcVertex
 | Description: One decoded vertex: VRAM-space position, texel coordinates, and tint colour.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    int16_t x, y;
    uint8_t u, v;
    uint8_t r, g, b;
} DcVertex;

/*----------------------
 | DC_SHAPE_TRI, DC_SHAPE_QUAD, DC_SHAPE_RECT, DC_SHAPE_LINE
 | Description: DcPrim.shape values: a 3-vertex, 4-vertex, top-left/bottom-right rectangle, or 2-vertex line primitive.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum { DC_SHAPE_TRI = 0, DC_SHAPE_QUAD = 1, DC_SHAPE_RECT = 2, DC_SHAPE_LINE = 3 };

/*----------------------
 | DC_DEPTH_4, DC_DEPTH_8, DC_DEPTH_15
 | Description: DcPrim.depth values: a textured primitive's texture page colour mode.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum { DC_DEPTH_4 = 0, DC_DEPTH_8 = 1, DC_DEPTH_15 = 2 };

/*----------------------
 | DcPrim
 | Description: One decoded primitive, shape-neutral and ready for a later task's clipping and PowerVR submission.
 | Author: suinevere
 | Dependencies: DcVertex
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    uint8_t shape, nverts, textured, raw, semi, abr, depth, gouraud;
    uint16_t tpage_x, tpage_y, clut_x, clut_y;
    DcVertex v[4];
} DcPrim;

/*----------------------
 | DcGpuState
 | Description: GPU environment carried across packets: draw area, draw offset, texture page and texture window.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    int16_t area_x1, area_y1, area_x2, area_y2, ofs_x, ofs_y;
    uint16_t tpage;
    uint32_t tex_window;
} DcGpuState;

/*----------------------
 | DcPrimSink
 | Description: Callback gpu_decode_packet invokes once per decoded primitive.
 | Author: suinevere
 | Dependencies: DcPrim, DcGpuState
 | Globals: N/A
 | Params: prim -- decoded primitive, state -- GPU state at decode time, ctx -- caller-supplied context
 | Returns: N/A
 ----------------------*/
typedef void (*DcPrimSink)(const DcPrim *prim, const DcGpuState *state, void *ctx);

/*----------------------
 | gpu_decode_reset
 | Description: Zeroes a GPU state: no draw area, no offset, tpage 0 and no texture window.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: state -- state to reset
 | Returns: N/A
 ----------------------*/
void gpu_decode_reset(DcGpuState *state);

/*----------------------
 | gpu_decode_packet
 | Description: Decodes every GP0 command packed into one ordering-table packet's body, updating state and calling sink for each primitive.
 | Author: suinevere
 | Dependencies: DcGpuState, DcPrimSink
 | Globals: N/A
 | Params: state -- GPU state, words -- packet body (no tag word), len -- word count, sink -- primitive callback, ctx -- sink context
 | Returns: words consumed (equal to len on a well-formed packet)
 ----------------------*/
unsigned gpu_decode_packet(DcGpuState *state, const uint32_t *words, unsigned len, DcPrimSink sink, void *ctx);

/*----------------------
 | gpu_decode_ot
 | Description: Walks an ordering-table packet chain and decodes every command in every non-empty packet, in chain order.
 | Author: suinevere
 | Dependencies: gpu_decode_packet, dc_halt
 | Globals: N/A
 | Params: state -- GPU state, ot_head -- first packet of the chain, sink -- primitive callback, ctx -- sink context
 | Returns: N/A
 ----------------------*/
void gpu_decode_ot(DcGpuState *state, const u_long *ot_head, DcPrimSink sink, void *ctx);

#ifdef LOM_HOST_TEST

/*----------------------
 | gpu_decode_set_base
 | Description: Host-test-only hook: treats a tag's 24-bit address field as an offset from base instead of Dreamcast RAM, so a chain can be built from host pointers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: base -- host address the 24-bit field 0 corresponds to
 | Returns: N/A
 ----------------------*/
void gpu_decode_set_base(uintptr_t base);

#endif

#endif
