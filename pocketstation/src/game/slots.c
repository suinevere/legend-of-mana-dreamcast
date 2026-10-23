/*----------------------
 | slots.c
 | Description: Ring Ring Land's marker slots (4 at 0x424) and animation slots (10 at 0x434), four bytes each, 0xFF in the first byte when free: clearing, starting and stepping them.
 | Author: suinevere
 | Dependencies: game.h, rrl_rom.h
 | Globals: marker, anim, seed
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "rrl_rom.h"

/*----------------------
 | game_markers_clear
 | Description: Frees the four marker slots.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: marker
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_markers_clear(void) {
    for (uint8_t i = 0; i < 4u; i++) {
        set_marker((uint8_t)(i * 4u), 0xFFu);
    }
}

/*----------------------
 | game_marker_used
 | Description: Whether marker slot index is in use.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: marker
 | Params: index -- slot 0-3
 | Returns: 1 when used, else 0
 ----------------------*/
uint8_t game_marker_used(uint8_t index) {
    return ram_marker((uint8_t)(index * 4u)) != 0xFFu;
}

/*----------------------
 | game_anims_clear
 | Description: Frees the ten animation slots.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: anim
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_anims_clear(void) {
    for (uint8_t i = 0; i < 10u; i++) {
        set_anim((uint8_t)(i * 4u), 0xFFu);
    }
}

/*----------------------
 | game_anim_start
 | Description: Fills animation slot with id and arg and starts its timer at a random 1 to the id's ROM_ANIM_DELAY byte.
 | Author: suinevere
 | Dependencies: rrl_ram.h, rrl_rom.h
 | Globals: anim, seed
 | Params: slot -- slot 0-9; id -- animation id; arg -- the slot's second byte
 | Returns: N/A
 ----------------------*/
void game_anim_start(uint8_t slot, uint8_t id, uint8_t arg) {
    uint8_t at = (uint8_t)(slot * 4u);
    uint16_t delay = (uint16_t)(psk_r8(ROM_ANIM_DELAY + id) << 4);
    set_anim(at, id);
    set_anim((uint8_t)(at + 1u), arg);
    set_anim((uint8_t)(at + 2u), 0);
    set_anim((uint8_t)(at + 3u), (uint8_t)((game_random(delay) >> 4) + 1u));
}

/*----------------------
 | game_marker_start
 | Description: Starts marker slot on object type at frame 0 with x offset dx, taking the frame's length from bits 16-23 of its bitmap word.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: marker
 | Params: slot -- slot 0-3; type -- object type; dx -- signed x offset
 | Returns: N/A
 ----------------------*/
void game_marker_start(uint8_t slot, uint8_t type, uint8_t dx) {
    uint8_t at = (uint8_t)(slot * 4u);
    set_marker(at, type);
    set_marker((uint8_t)(at + 1u), 0);
    set_marker((uint8_t)(at + 3u), dx);
    set_marker((uint8_t)(at + 2u), (uint8_t)(game_frame_info(type, 0).bitmap >> 16));
}

/*----------------------
 | game_marker_frame
 | Description: Steps marker slot's animation and returns the sprite to draw, bitmap 0 for none; a next frame of 0x80 frees the slot. The bitmap word, its sub-image offset and the address are 32-bit as in the ARM. The top-bit flag is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: marker
 | Params: slot -- slot 0-3, in use
 | Returns: the sprite, with flags bit 2 set when the bitmap word's top bit is
 ----------------------*/
SpriteInfo game_marker_frame(uint8_t slot) {
    uint8_t at = (uint8_t)(slot * 4u), type = ram_marker(at), frame = ram_marker((uint8_t)(at + 1u));
    uint8_t left = ram_marker((uint8_t)(at + 2u)), sub;
    FrameInfo f = game_frame_info(type, frame);
    SpriteInfo s = {0, 0, 0, 0, 0, 0};
    uint32_t size;
    if (left == 0u) {
        frame = (uint8_t)(frame + f.bitmap);
        set_marker((uint8_t)(at + 1u), frame);
        f = game_frame_info(type, frame);
        if (f.bitmap == 0x80u) {
            set_marker(at, 0xFFu);
            return s;
        }
        left = (uint8_t)(f.bitmap >> 16);
    }
    set_marker((uint8_t)(at + 2u), (uint8_t)(left - 1u));
    s = game_sprite_info((int16_t)(f.dx + (int8_t)ram_marker((uint8_t)(at + 3u))), f.dy, (uint8_t)(type - f.extra));
    if (f.bitmap & 0x80000000u) {
        s.flags = (uint8_t)(s.flags + 4u);
    }
    sub = (uint8_t)(f.bitmap >> 8);
    if (sub == 0xFFu) {
        s.bitmap = 0;
        return s;
    }
    size = (((uint32_t)s.width * s.height + 31u) >> 5) << 2;
    size *= sub;
    if (s.flags & 1u) {
        size <<= 1;
    }
    s.bitmap += size;
    return s;
}

/*----------------------
 | ROM_ANIM_HOLD
 | Description: Frames each animation id holds a frame, one byte per id.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_ANIM_HOLD 0x0200A7B0u

/*----------------------
 | game_anims_tick
 | Description: Counts down every animation slot in use; when a slot's count runs out its frame bit flips and the count restarts at its id's hold time.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: anim
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_anims_tick(void) {
    uint8_t slot, at, id, left;
    for (slot = 0; slot < 10u; slot++) {
        at = (uint8_t)(slot * 4u);
        id = ram_anim(at);
        if (id == 0xFFu) {
            continue;
        }
        left = (uint8_t)(ram_anim((uint8_t)(at + 3u)) - 1u);
        if (left == 0u) {
            set_anim((uint8_t)(at + 2u), (uint8_t)(ram_anim((uint8_t)(at + 2u)) ^ 1u));
            left = psk_r8(ROM_ANIM_HOLD + id);
        }
        set_anim((uint8_t)(at + 3u), left);
    }
}
