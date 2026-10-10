/*----------------------
 | wmap_save_views.h
 | Description: WMAP's typed views over its portion of g_saved_game, kept separate so
 |              each struct can carry a LOM_BIG_ENDIAN layout without touching the PS1 build.
 | Author: suinevere
 | Dependencies: common.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef WMAP_SAVE_VIEWS_H
#define WMAP_SAVE_VIEWS_H

#include "common.h"

#define WMAP_SAVE_VIEWS_LAND_COUNT 64
#define WMAP_SAVE_VIEWS_SPIRIT_COUNT 8

/**
 * @brief Saved land state; coordinates share a byte and flags share a word.
 * @note wmap_land_layout.c manipulates the whole status.word with native bit ops
 *       (e.g. status.word & WMAP_LAND_ACTIVE), so on big-endian the byte that holds
 *       bits 0-7 of that word is the last one, not the first.
 */
typedef union
{
    u32 word;
    struct
    {
#if defined(LOM_BIG_ENDIAN)
        u8 artifact_order;
        u8 placement_order;
        u8 position;
        u8 flags;
#else
        u8 flags;
        u8 position;
        u8 placement_order;
        u8 artifact_order;
#endif
    } data;
} WmapLandStatus;

/** @brief Saved placement status and eight spirit levels. */
typedef struct
{
    WmapLandStatus status;
    u8 spirits[WMAP_SAVE_VIEWS_SPIRIT_COUNT];
} WmapSavedLand;

/** @brief Save fields used by the land-layout routines. */
typedef struct
{
    u8 unknown_0x000[0x2E4];
    /**
     * @note wmap_update_travel_growth shifts calendar.word by 0x10 to read/write day,
     *       so on big-endian day must sit in the first two bytes, not the last two.
     */
    union
    {
        u32 word;
        struct
        {
#if defined(LOM_BIG_ENDIAN)
            u16 day;
            u8 placed_land_count;
            u8 unknown_0x2e4;
#else
            u8 unknown_0x2e4;
            u8 placed_land_count;
            u16 day;
#endif
        } data;
    } calendar;
    u32 events[2];
    WmapSavedLand lands[WMAP_SAVE_VIEWS_LAND_COUNT];
} WmapSave;

/** @brief Save header fields used to select the map layout. */
typedef struct
{
    u8 unknown_0x00[0x28];
#if defined(LOM_BIG_ENDIAN)
    unsigned int other_flags : 29;
    unsigned int option : 1;
    unsigned int unused_flags : 2;
    u8 unknown_0x2c[0xE2 - 0x2C];
#else
    unsigned int unused_flags : 2;
    unsigned int option : 1;
    unsigned int other_flags : 29;
    u8 unknown_0x2c[0xE0 - 0x2C];
#endif
    u16 layout_id;
} WmapSaveHeader;

/**
 * @brief Saved map entry status at its original byte offsets.
 * @note entry.data.flags shares its byte with WmapLandStatus.status.data.flags, which
 *       wmap_land_layout.c manipulates as a native u32 word, so on big-endian the flags
 *       byte and bits move to the end of the entry the same way status.data.flags does.
 */
typedef struct
{
    u8 pad_000[0x2F0];
    union
    {
#if defined(LOM_BIG_ENDIAN)
        struct
        {
            unsigned int unused_hi : 24;
            unsigned int other_flags : 6;
            unsigned int placed : 1;
            unsigned int available : 1;
        } flags;
        struct
        {
            u8 artifact_order;
            u8 pad[2];
            u8 flags;
        } data;
#else
        struct
        {
            unsigned int available : 1;
            unsigned int placed : 1;
            unsigned int other_flags : 6;
        } flags;
        struct
        {
            u8 flags;
            u8 pad[2];
            u8 artifact_order;
        } data;
#endif
    } entry;
} WmapSavedEntry;

#endif
