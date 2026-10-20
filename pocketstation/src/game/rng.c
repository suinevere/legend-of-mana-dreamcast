/*----------------------
 | rng.c
 | Description: Ring Ring Land's random numbers, its weighted picks from the card's odds tables and its random picks from the item table.
 | Author: suinevere
 | Dependencies: game.h, rrl_rom.h
 | Globals: seed, sel_a, sel_b, odds_set, scratch_count, scene_id, mode_done, anim
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "rrl_rom.h"

/*----------------------
 | game_random
 | Description: Steps the 16-bit seed (seed = (seed + 1) * 0x3D09) and scales it to a value below limit.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: seed
 | Params: limit -- exclusive upper bound
 | Returns: a value in [0, limit)
 ----------------------*/
uint16_t game_random(uint16_t limit) {
    uint16_t s = (uint16_t)((ram_seed() + 1u) * 0x3D09u);
    set_seed(s);
    return (uint16_t)(((uint32_t)limit * s) >> 16);
}

/*----------------------
 | pick_weighted
 | Description: Rolls below 100 and returns the index of the byte weight the roll lands in; weights is the card's 32-bit word of four packed weights.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: seed
 | Params: weights -- four byte weights, lowest byte first, summing to 100
 | Returns: the index picked
 ----------------------*/
static uint8_t pick_weighted(uint32_t weights) {
    uint16_t roll = game_random(100u);
    uint8_t i = 0u;
    while (roll >= (weights & 0xFFu)) {
        roll = (uint16_t)(roll - (weights & 0xFFu));
        weights >>= 8;
        i++;
    }
    return i;
}

/*----------------------
 | game_pick_by_sel
 | Description: A weighted pick from the odds set sel_a | sel_b << 1.
 | Author: suinevere
 | Dependencies: rrl_ram.h, rrl_rom.h
 | Globals: sel_a, sel_b, seed
 | Params: N/A
 | Returns: the index picked
 ----------------------*/
uint8_t game_pick_by_sel(void) {
    uint16_t set = ram_sel_a() | (ram_sel_b() << 1);
    return pick_weighted(psk_r32(ROM_ODDS_SEL + set * 4u));
}

/*----------------------
 | game_pick_by_set
 | Description: A weighted pick from the odds set odds_set.
 | Author: suinevere
 | Dependencies: rrl_ram.h, rrl_rom.h
 | Globals: odds_set, seed
 | Params: N/A
 | Returns: the index picked
 ----------------------*/
uint8_t game_pick_by_set(void) {
    return pick_weighted(psk_r32(ROM_ODDS_SET + ram_odds_set() * 4u));
}

/*----------------------
 | game_pick_bag_nth
 | Description: A weighted pick from ROM_ODDS_NTH, stored in scratch_count for 02002D8A's item search.
 | Author: suinevere
 | Dependencies: rrl_ram.h, rrl_rom.h
 | Globals: scratch_count, seed
 | Params: N/A
 | Returns: the index picked
 ----------------------*/
uint8_t game_pick_bag_nth(void) {
    uint8_t i = pick_weighted(psk_r32(ROM_ODDS_NTH));
    set_scratch_count(i);
    return i;
}

/*----------------------
 | game_event_roll
 | Description: Rolls below 100 against scene_id's ROM_EVENT_ODDS list, a 32-bit address, and makes scene_id the high byte of the entry the roll lands in. No script reaches it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h, rrl_rom.h
 | Globals: scene_id, seed
 | Params: N/A
 | Returns: the index of the entry picked
 ----------------------*/
uint8_t game_event_roll(void) {
    uint32_t list = psk_r32(ROM_EVENT_ODDS + ((uint32_t)ram_scene_id() - 7u) * 4u);
    uint16_t roll = (uint16_t)(game_random(1600u) >> 4), entry;
    uint8_t i = 0u;
    for (;; i++) {
        entry = psk_r16(list + i * 2u);
        if (roll < (entry & 0xFFu)) {
            break;
        }
        roll = (uint16_t)(roll - (entry & 0xFFu));
    }
    set_scene_id((uint8_t)(entry >> 8));
    return i;
}

/*----------------------
 | ITEM_ENTRIES
 | Description: Entries of the item table at ROM_WORDS_8094 that game_pick_item counts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ITEM_ENTRIES 255u

/*----------------------
 | item_matches
 | Description: Whether item table entry i has kind in its low byte and odds pick in its whole high byte, top bit included.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: N/A
 | Params: i -- entry; kind -- low byte wanted; pick -- high byte wanted
 | Returns: 1 on a match, else 0
 ----------------------*/
static uint8_t item_matches(uint16_t i, uint8_t kind, uint8_t pick) {
    uint16_t h = psk_r16(ROM_WORDS_8094 + i * 2u);
    return (h & 0xFFu) == kind && (h >> 8) == pick;
}

/*----------------------
 | game_pick_item
 | Description: Picks at random an item table entry matching two weighted picks, rolled with odds set 0 for how 3. How 2 offsets an anim slot's id, and scene 13 with how 1 gives a fixed item 80% of the time. How 2 and scene 13 are converted from the listing and unverified; how 2's anim index is 8-bit, as mode_done stays at 0-3.
 | Author: suinevere
 | Dependencies: rrl_ram.h, rrl_rom.h
 | Globals: seed, odds_set, sel_a, sel_b, scene_id, mode_done, anim
 | Params: how -- 2 for the anim slot's item, 3 for odds set 0, 1 for scene 13's fixed items
 | Returns: the entry index, or the fixed or anim-based item
 ----------------------*/
uint16_t game_pick_item(uint8_t how) {
    uint8_t pick, kind, n = 0u;
    uint16_t i;
    if (how == 2u) {
        pick = game_pick_by_set();
        return (uint16_t)(ram_anim((uint8_t)((ram_mode_done() + 5u) * 4u)) + (pick == 3u ? 0x40u : 0x48u));
    }
    if (how == 3u) {
        uint8_t saved = ram_odds_set();
        set_odds_set(0);
        pick = game_pick_by_set();
        set_odds_set(saved);
    } else if (how == 1u && ram_scene_id() == 13u) {
        if (game_random(100u) < 80u) {
            if (ram_sel_a() == 0u) {
                return ram_sel_b() == 0u ? 0x34u : 0x15u;
            }
            return ram_sel_b() == 0u ? 8u : 0x1Bu;
        }
        pick = 3u;
    } else {
        pick = game_pick_by_set();
    }
    kind = game_pick_by_sel();
    for (i = 0u; i < ITEM_ENTRIES; i++) {
        n = (uint8_t)(n + item_matches(i, kind, pick));
    }
    n = (uint8_t)(game_random((uint16_t)(n << 4)) >> 4);
    for (i = 0u;; i++) {
        if (item_matches(i, kind, pick)) {
            if (n == 0u) {
                return i;
            }
            n--;
        }
    }
}
