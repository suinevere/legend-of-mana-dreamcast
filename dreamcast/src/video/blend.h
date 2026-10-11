/*----------------------
 | blend.h
 | Description: Pure-C table mapping a PS1 primitive's semi-transparency mode to up to four PowerVR blend passes, plus vertex-colour scaling and the pass that adds a texture tint above 0x80.
 | Author: suinevere
 | Dependencies: stdint.h, palette_bank.h (PAL_*, STP_*)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_DC_BLEND_H
#define LOM_DC_BLEND_H

#include <stdint.h>

/*----------------------
 | BL_ZERO, BL_ONE, BL_SRCALPHA, BL_INVSRCALPHA, BL_INVSRCCOLOR, BL_INVDSTCOLOR
 | Description: Blend factors a BlendPass chooses between, named after their PowerVR/D3D meaning; BL_INVSRCCOLOR is a destination factor and BL_INVDSTCOLOR a source factor.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum { BL_ZERO = 0, BL_ONE, BL_SRCALPHA, BL_INVSRCALPHA, BL_INVSRCCOLOR, BL_INVDSTCOLOR };

/*----------------------
 | BLEND_MAX_PASSES
 | Description: Most passes blend_plan writes: an opaque pass for non-STP texels plus subtraction's invert, add, invert.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BLEND_MAX_PASSES 4

/*----------------------
 | BlendPass
 | Description: One blend pass: source and destination factors, a fixed alpha weight, the palette variant, and whether it draws the primitive's coverage in white instead of its colour.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct { uint8_t src, dst, alpha, variant, cover; } BlendPass;

/*----------------------
 | blend_plan
 | Description: Picks the draw-order blend passes for a PS1 primitive's semi-transparency mode and ABR code; subtraction inverts the covered pixels, adds, and inverts back.
 | Author: suinevere
 | Dependencies: BlendPass, PAL_*, STP_*
 | Globals: N/A
 | Params: textured -- primitive samples a texture, semi -- semi-transparency flag, abr -- PS1 ABR code 0-3, clut_stp -- STP_NONE/STP_ALL/STP_MIXED of the CLUT (ignored if untextured), can_cover -- the texture honours PAL_COVER (false for 15bpp), out -- receives up to BLEND_MAX_PASSES passes
 | Returns: number of passes written to out, 1 to BLEND_MAX_PASSES
 ----------------------*/
unsigned blend_plan(int textured, int semi, int abr, int clut_stp, int can_cover, BlendPass out[BLEND_MAX_PASSES]);

/*----------------------
 | blend_vertex_colour
 | Description: Scales one vertex colour channel for a textured primitive, doubling it unless the texture is raw (no tinting).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- channel value 0-255, textured -- primitive samples a texture, raw -- texture is a raw (untinted) direct texture
 | Returns: scaled channel value 0-255
 ----------------------*/
uint8_t blend_vertex_colour(uint8_t c, int textured, int raw);

/*----------------------
 | blend_excess_colour
 | Description: The part of a textured channel's PS1 tint above 1.0 that blend_vertex_colour clamps away, as an extra-pass vertex colour; zero when nothing is clamped.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- channel value 0-255, textured -- primitive samples a texture, raw -- texture is a raw (untinted) direct texture
 | Returns: excess channel value 0-254
 ----------------------*/
uint8_t blend_excess_colour(uint8_t c, int textured, int raw);

/*----------------------
 | blend_excess_pass
 | Description: The pass that adds a primitive's excess tint after one of its colour passes: the same factors and variant, but adding where the base pass overwrote.
 | Author: suinevere
 | Dependencies: BlendPass
 | Globals: N/A
 | Params: base -- a pass from blend_plan, out -- receives the excess pass
 | Returns: 1 if base draws the primitive's colour, 0 for a coverage pass
 ----------------------*/
int blend_excess_pass(const BlendPass *base, BlendPass *out);

#endif
