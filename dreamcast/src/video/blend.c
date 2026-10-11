/*----------------------
 | blend.c
 | Description: Maps a PS1 primitive's semi-transparency mode and ABR code to PowerVR blend passes, keyed off the CLUT's STP classification, with exact subtraction by inversion and an excess pass for texture tints above 0x80.
 | Author: suinevere
 | Dependencies: blend.h, palette_bank.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "blend.h"
#include "palette_bank.h"

/*----------------------
 | abr_pass
 | Description: Builds the blend factors and alpha weight for PS1 ABR codes 1-3 (additive, subtractive, quarter-add).
 | Author: suinevere
 | Dependencies: BlendPass
 | Globals: N/A
 | Params: abr -- PS1 ABR code 1, 2 or 3, textured -- use SRCALPHA instead of ONE so a masked texture alpha excludes other texels
 | Returns: the blend pass, variant left at 0
 ----------------------*/
static BlendPass abr_pass(int abr, int textured)
{
    BlendPass pass;

    if (abr == 1)
    {
        pass.src = (uint8_t)(textured ? BL_SRCALPHA : BL_ONE);
        pass.dst = BL_ONE;
        pass.alpha = 255;
    }
    else if (abr == 2)
    {
        pass.src = BL_ZERO;
        pass.dst = BL_INVSRCCOLOR;
        pass.alpha = 255;
    }
    else
    {
        pass.src = BL_SRCALPHA;
        pass.dst = BL_ONE;
        pass.alpha = 64;
    }
    pass.variant = 0;
    pass.cover = 0;
    return pass;
}

/*----------------------
 | make_pass
 | Description: Builds a blend pass that draws the primitive's own colour.
 | Author: suinevere
 | Dependencies: BlendPass
 | Globals: N/A
 | Params: src, dst -- BL_* factors, alpha -- vertex alpha, variant -- PAL_* variant
 | Returns: the blend pass
 ----------------------*/
static BlendPass make_pass(int src, int dst, int alpha, int variant)
{
    BlendPass pass;

    pass.src = (uint8_t)src;
    pass.dst = (uint8_t)dst;
    pass.alpha = (uint8_t)alpha;
    pass.variant = (uint8_t)variant;
    pass.cover = 0;
    return pass;
}

/*----------------------
 | subtract_passes
 | Description: Writes exact PS1 subtraction as three passes: invert the covered pixels, add the primitive, invert them back, so dst - src clamps at zero.
 | Author: suinevere
 | Dependencies: make_pass
 | Globals: N/A
 | Params: out -- receives three passes, cover_variant -- PAL_* variant whose texels are white where drawn, add -- the additive pass
 | Returns: 3
 ----------------------*/
static unsigned subtract_passes(BlendPass *out, int cover_variant, BlendPass add)
{
    BlendPass cover = make_pass(BL_INVDSTCOLOR, BL_INVSRCALPHA, 255, cover_variant);

    cover.cover = 1;
    out[0] = cover;
    out[1] = add;
    out[2] = cover;
    return 3;
}

/*----------------------
 | blend_plan
 | Description: Picks the draw-order blend passes for a PS1 primitive's semi-transparency mode and ABR code; subtraction inverts the covered pixels, adds, and inverts back.
 | Author: suinevere
 | Dependencies: BlendPass, PAL_*, STP_*, abr_pass, make_pass, subtract_passes
 | Globals: N/A
 | Params: textured -- primitive samples a texture, semi -- semi-transparency flag, abr -- PS1 ABR code 0-3, clut_stp -- STP_NONE/STP_ALL/STP_MIXED of the CLUT (ignored if untextured), can_cover -- the texture honours PAL_COVER (false for 15bpp), out -- receives up to BLEND_MAX_PASSES passes
 | Returns: number of passes written to out, 1 to BLEND_MAX_PASSES
 ----------------------*/
unsigned blend_plan(int textured, int semi, int abr, int clut_stp, int can_cover, BlendPass out[BLEND_MAX_PASSES])
{
    if (!semi)
    {
        out[0] = make_pass(BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_OPAQUE);
        return 1;
    }
    if (!textured)
    {
        if (abr == 0)
        {
            out[0] = make_pass(BL_SRCALPHA, BL_INVSRCALPHA, 128, PAL_OPAQUE);
            return 1;
        }
        if (abr == 2)
        {
            return subtract_passes(out, PAL_OPAQUE, make_pass(BL_ONE, BL_ONE, 255, PAL_OPAQUE));
        }
        out[0] = abr_pass(abr, 0);
        out[0].variant = PAL_OPAQUE;
        return 1;
    }
    if (clut_stp == STP_NONE)
    {
        out[0] = make_pass(BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_OPAQUE);
        return 1;
    }
    if (abr == 0)
    {
        out[0] = make_pass(BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_HALF);
        return 1;
    }
    {
        BlendPass stp_pass = abr_pass(abr, 1);
        unsigned n = 0;

        stp_pass.variant = PAL_STP_ONLY;
        if (clut_stp != STP_ALL)
        {
            out[n++] = make_pass(BL_SRCALPHA, BL_INVSRCALPHA, 255, PAL_NONSTP_ONLY);
        }
        if (abr == 2 && can_cover)
        {
            return n + subtract_passes(&out[n], PAL_COVER, make_pass(BL_SRCALPHA, BL_ONE, 255, PAL_STP_ONLY));
        }
        out[n++] = stp_pass;
        return n;
    }
}

/*----------------------
 | blend_vertex_colour
 | Description: Scales one vertex colour channel for a textured primitive, doubling it unless the texture is raw (no tinting).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- channel value 0-255, textured -- primitive samples a texture, raw -- texture is a raw (untinted) direct texture
 | Returns: scaled channel value 0-255
 ----------------------*/
uint8_t blend_vertex_colour(uint8_t c, int textured, int raw)
{
    unsigned scaled;

    if (!textured)
    {
        return c;
    }
    if (raw)
    {
        return 255;
    }
    scaled = (unsigned)c * 2u;
    return (uint8_t)(scaled > 255u ? 255u : scaled);
}

/*----------------------
 | blend_excess_colour
 | Description: The part of a textured channel's PS1 tint above 1.0 that blend_vertex_colour clamps away, as an extra-pass vertex colour; zero when nothing is clamped.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- channel value 0-255, textured -- primitive samples a texture, raw -- texture is a raw (untinted) direct texture
 | Returns: excess channel value 0-254
 ----------------------*/
uint8_t blend_excess_colour(uint8_t c, int textured, int raw)
{
    if (!textured || raw || c <= 128u)
    {
        return 0;
    }
    return (uint8_t)((unsigned)c * 2u - 256u);
}

/*----------------------
 | blend_excess_pass
 | Description: The pass that adds a primitive's excess tint after one of its colour passes: the same factors and variant, but adding where the base pass overwrote.
 | Author: suinevere
 | Dependencies: BlendPass
 | Globals: N/A
 | Params: base -- a pass from blend_plan, out -- receives the excess pass
 | Returns: 1 if base draws the primitive's colour, 0 for a coverage pass
 ----------------------*/
int blend_excess_pass(const BlendPass *base, BlendPass *out)
{
    if (base->cover)
    {
        return 0;
    }
    *out = *base;
    if (out->dst == BL_INVSRCALPHA)
    {
        out->dst = BL_ONE;
    }
    return 1;
}
