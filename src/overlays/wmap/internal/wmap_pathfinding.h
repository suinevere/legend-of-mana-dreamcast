#ifndef WMAP_PATHFINDING_H
#define WMAP_PATHFINDING_H

#include "common.h"

/**
 * @brief Packed world-map slot record: a 32-bit word whose low bits/nibbles hold
 *        flags and grid coordinates, aliased byte-wise and as bitfields.
 * @note func_8005DBB8 and its callers mix .word bit ops with .b and .f field reads, so on
 *       big-endian both views must be declared in reverse to keep the same bits in the
 *       same named fields.
 */
typedef union {
    u32 word;
#if defined(LOM_BIG_ENDIAN)
    struct { u8 unk3; u8 unk2; u8 b1; u8 b0; } b;
    struct {
        u32 hi : 16;
        u32 nibC : 4;
        u32 nib8 : 4;
        u32 pad3 : 5;
        u32 bit2 : 1;
        u32 bit1 : 1;
        u32 bit0 : 1;
    } f;
#else
    struct { u8 b0; u8 b1; u8 unk2; u8 unk3; } b;
    struct {
        u32 bit0 : 1;
        u32 bit1 : 1;
        u32 bit2 : 1;
        u32 pad3 : 5;
        u32 nib8 : 4;
        u32 nibC : 4;
        u32 hi : 16;
    } f;
#endif
} WmapSlot;

void func_8005DBB8(void);
void func_8005DF50(s32 arg0, s32 arg1);
void func_8005EB68(s32 start_x, s32 start_y, s32 end_x, s32 end_y, s32* out_x, s32* out_y);

#endif
