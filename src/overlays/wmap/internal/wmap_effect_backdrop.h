#ifndef WMAP_EFFECT_BACKDROP_H
#define WMAP_EFFECT_BACKDROP_H

#include "common.h"

/**
 * @brief Fixed-point coordinate with direct access to its integer halfword.
 * @note value is written as a native s32 (e.g. value = x << 16) and whole is read back
 *       for rendering, so on big-endian whole must be the high halfword, not the low one.
 */
typedef union
{
    s32 value;
    struct
    {
#if defined(LOM_BIG_ENDIAN)
        signed int whole : 16;
        unsigned int fraction : 16;
#else
        unsigned int fraction : 16;
        signed int whole : 16;
#endif
    } parts;
} WmapFixed;

/** @brief Number of frames left in the mesh transition. */
extern s32 g_wmap_mesh_transition_frames;
/** @brief Signed mesh motion multiplier; zero when the transition is idle. */
extern s32 g_wmap_mesh_transition_direction;
/** @brief Current mesh transition selection. */
extern s32 g_wmap_mesh_transition_selection;
/** @brief Mesh selection replaced by the current transition. */
extern s32 g_wmap_mesh_previous_selection;

void wmap_init_transition_mesh(void);
void wmap_draw_transition_mesh(void);
void wmap_start_mesh_transition(s32 selection);
void wmap_select_mesh_motion();

#endif
