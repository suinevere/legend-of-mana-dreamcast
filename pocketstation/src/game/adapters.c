/*----------------------
 | adapters.c
 | Description: Temporary register-level entries for structured routines whose R1 callers read more than r0; each hands back exactly the registers live_regs.py names.
 | Author: suinevere
 | Dependencies: game.h, psk_cpu.h, rrl_rom.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "psk_cpu.h"
#include "rrl_rom.h"

/*----------------------
 | STATE_SUB
 | Description: Offset of the mode sub-state from the game state that r6 points at.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STATE_SUB 0x20u

/*----------------------
 | nibble_leftovers
 | Description: What game_nibble's ARM leaves in r1 (the nibble's bit offset) and r2 (its word shifted down by it).
 | Author: suinevere
 | Dependencies: psk_cpu.h, rrl_rom.h
 | Globals: N/A
 | Params: c -- ARM register file; index -- nibble entry
 | Returns: N/A
 ----------------------*/
static void nibble_leftovers(PskCpu *c, uint32_t index) {
    c->r[1] = (index & 7u) << 2;
    c->r[2] = psk_ldr(ROM_NIBBLES_9DF0 + ((index & 0xF8u) >> 1)) >> c->r[1];
}

/*----------------------
 | mirror_last_bit
 | Description: The last bit game_mirror_direction's ARM tests, which it leaves in r1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: bits -- the direction bits mirrored
 | Returns: 1, 2, 4 or 8
 ----------------------*/
static uint32_t mirror_last_bit(uint32_t bits) {
    return (bits & 1u) ? 1u : (bits & 2u) ? 2u : (bits & 4u) ? 4u : 8u;
}









/*----------------------
 | game_read_buttons_regs
 | Description: Register entry for game_read_buttons (ARM); r1 = presses, r2 = the last direction countdown written, r3 = long hold, r4 = 0x0A000000, r5 = 0x1F.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_read_buttons_regs(PskCpu *c) {
    uint8_t held = ram_btn_latch();
    c->r[3] = game_read_buttons((uint8_t)(c->r[0] != 0));
    c->r[1] = ram_btn_press();
    if (held & BTN_RIGHT) {
        c->r[2] = ram_repeat_right();
    } else if (held & BTN_LEFT) {
        c->r[2] = ram_repeat_left();
    } else if (held & BTN_DOWN) {
        c->r[2] = ram_repeat_down();
    } else if (held & BTN_UP) {
        c->r[2] = ram_repeat_up();
    }
    c->r[4] = 0x0A000000u;
    c->r[5] = 0x1Fu;
    c->r[15] = c->r[14] & ~3u;
}

/*----------------------
 | game_buttons_irq_regs
 | Description: Register entry for game_buttons_irq (ARM); r9 loses the latched lines.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_buttons_irq_regs(PskCpu *c) {
    game_buttons_irq(c->r[0]);
    c->r[9] &= ~(c->r[0] & 0x1Fu);
    c->r[15] = c->r[14] & ~3u;
}


/*----------------------
 | game_fb_put_rows_regs
 | Description: Register entry for game_fb_put_rows (ARM); r4 ends one word past the last row read.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_fb_put_rows_regs(PskCpu *c) {
    game_fb_put_rows((uint8_t)c->r[1], (uint8_t)c->r[3], c->r[4], (uint8_t)(c->r[5] != 0));
    c->r[4] += 4u * (uint8_t)c->r[3];
    c->r[15] = psk_bx(c->r[14]);
}



/*----------------------
 | game_sprite_info_regs
 | Description: Register entry for game_sprite_info taking r0-r1 position and r2 id (16-bit, so a free anim slot's 0x11B survives); returns r0-r1 position, r2-r3 size, r4 bitmap, r5 flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_sprite_info_regs(PskCpu *c) {
    SpriteInfo s = game_sprite_info((int16_t)c->r[0], (int16_t)c->r[1], (uint16_t)c->r[2]);
    c->r[0] = (uint32_t)(int32_t)s.x;
    c->r[1] = (uint32_t)(int32_t)s.y;
    c->r[2] = s.width;
    c->r[3] = s.height;
    c->r[4] = s.bitmap;
    c->r[5] = s.flags;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_sprite_draw_regs
 | Description: Register entry for game_sprite_draw (ARM), taking game_sprite_info's register layout; it needs r4-r5, and no caller reads a register back.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_sprite_draw_regs(PskCpu *c) {
    game_sprite_draw((int16_t)c->r[0], (int16_t)c->r[1], (uint8_t)c->r[2], (uint8_t)c->r[3], c->r[4], (uint8_t)c->r[5]);
    c->r[15] = psk_bx(c->r[14]);
}


/*----------------------
 | game_frame_info_regs
 | Description: Register entry for game_frame_info on the object r7 points at (type, frame); r0-r1 offsets, r2 bitmap, r3 word, r4 extra.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_frame_info_regs(PskCpu *c) {
    FrameInfo f = game_frame_info(psk_r8(c->r[7]), psk_r8(c->r[7] + 1u));
    c->r[0] = (uint32_t)(int32_t)f.dx;
    c->r[1] = (uint32_t)(int32_t)f.dy;
    c->r[2] = f.bitmap;
    c->r[3] = f.word;
    c->r[4] = f.extra;
    c->r[15] = psk_bx(c->r[14]);
}






/*----------------------
 | game_digits_x_regs
 | Description: Register entry for game_digits_x: x in r0, the number in r2.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_digits_x_regs(PskCpu *c) {
    c->r[0] = game_digits_x(c->r[0], c->r[2]);
    c->r[15] = psk_bx(c->r[14]);
}




/*----------------------
 | game_divide_regs
 | Description: Register entry for game_divide; r0 = quotient, r1 = remainder.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_divide_regs(PskCpu *c) {
    uint32_t rem;
    c->r[0] = game_divide(c->r[0], c->r[1], &rem);
    c->r[1] = rem;
    c->r[15] = psk_bx(c->r[14]);
}


/*----------------------
 | game_timer_start_regs
 | Description: Register entry for starting a timer; r3 = 4, r4 = the timer's base, r5 = the mode written.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_timer_start_regs(PskCpu *c) {
    c->r[5] = game_timer_run(c->r[0], 1);
    c->r[3] = 4u;
    c->r[4] = 0x0A800000u + c->r[0] * 16u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_timer_stop_regs
 | Description: Register entry for stopping a timer; r3 = 4, r4 = the timer's base, r5 = the mode written.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_timer_stop_regs(PskCpu *c) {
    c->r[5] = game_timer_run(c->r[0], 0);
    c->r[3] = 4u;
    c->r[4] = 0x0A800000u + c->r[0] * 16u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_timer_irq_on_regs
 | Description: Register entry for enabling a timer's interrupt; r4 = the interrupt controller, r5 = the line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_timer_irq_on_regs(PskCpu *c) {
    c->r[5] = game_timer_irq(c->r[0], 1);
    c->r[4] = 0x0A000000u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_timer_irq_off_regs
 | Description: Register entry for disabling a timer's interrupt; r4 = the interrupt controller, r5 = the line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_timer_irq_off_regs(PskCpu *c) {
    c->r[5] = game_timer_irq(c->r[0], 0);
    c->r[4] = 0x0A000000u;
    c->r[15] = psk_bx(c->r[14]);
}


/*----------------------
 | game_reels_step_regs
 | Description: Register entry for game_reels_step; r1 = reel_phase and r3 = reel_speed_c after steering, r2 = reel_len << 9.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: reel_phase, reel_speed_c, reel_len
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reels_step_regs(PskCpu *c) {
    game_reels_steer();
    c->r[1] = ram_reel_phase();
    c->r[3] = ram_reel_speed_c();
    game_reels_turn();
    c->r[2] = (uint32_t)ram_reel_len() << 9;
    c->r[15] = psk_bx(c->r[14]);
}


/*----------------------
 | game_irq_dispatch_regs
 | Description: Register entry for game_irq_dispatch (ARM) on the lines in r9; r9 comes back as it went in, as the original pops it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_irq_dispatch_regs(PskCpu *c) {
    uint32_t lr = c->r[14], pending = c->r[9];
    game_irq_dispatch(c, (uint16_t)pending);
    c->r[9] = pending;
    c->r[15] = lr & ~3u;
}

/*----------------------
 | game_irq_callback_regs
 | Description: Register entry for game_irq_callback (ARM), returning to the kernel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_irq_callback_regs(PskCpu *c) {
    uint32_t lr = c->r[14];
    game_irq_callback(c);
    c->r[14] = lr;
    c->r[15] = psk_bx(lr);
}

/*----------------------
 | game_fiq_callback_regs
 | Description: Register entry for game_fiq_callback (ARM): continues at the sound driver with the kernel's link register, or returns to the kernel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_fiq_callback_regs(PskCpu *c) {
    uint32_t lr = c->r[14], driver = game_fiq_callback(c);
    c->r[14] = lr;
    c->r[15] = driver != 0u ? psk_bx(driver) : psk_bx(lr);
}

/*----------------------
 | game_pop_return_regs
 | Description: Register entry for the ARM epilogue fragment at 0200375C, which pops the return address its predecessor pushed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_pop_return_regs(PskCpu *c) {
    c->r[15] = psk_r32(c->r[13]) & ~3u;
    c->r[13] += 4u;
}


/*----------------------
 | game_menu_pick_b_regs
 | Description: Register entry for game_menu_pick_b; r1 = mode_wait, r2 = the anim value after a choice, else 1 when the blink flipped and 8 when not.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: mode_phase, mode_wait, sel_a, sel_b
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_menu_pick_b_regs(PskCpu *c) {
    uint8_t phase = ram_mode_phase();
    if (game_menu_pick_b()) {
        c->r[2] = ((1u - ram_sel_a()) << 1) + ram_sel_b();
    } else {
        c->r[2] = ram_mode_phase() != phase ? 1u : 8u;
    }
    c->r[1] = ram_mode_wait();
    c->r[15] = psk_bx(c->r[14]);
}


/*----------------------
 | meter_leftover
 | Description: What game_meter_update's ARM will leave in r1, worked out before it runs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: meter, meter_shown, meter_delay
 | Params: N/A
 | Returns: the delay limit while counting, else 0x5C4 after a sound request, else meter
 ----------------------*/
static uint32_t meter_leftover(void) {
    uint8_t target = ram_meter(), shown = ram_meter_shown(), delay = target == 0u ? 40u : 30u;
    if (ram_meter_delay() != delay) {
        return delay;
    }
    return (target != 0u && target >= shown) ? 0x5C4u : target;
}


/*----------------------
 | WalkSeen
 | Description: The walk state game_walk_step starts from, kept so its register leftovers can be worked out after it runs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint8_t busy, kind, frame, cell, facing;
} WalkSeen;

/*----------------------
 | walk_seen
 | Description: Reads the walk state game_walk_step is about to start from.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: walk_busy, walk_kind, walk_frame, sel_c, sel_d
 | Params: N/A
 | Returns: the walk state
 ----------------------*/
static WalkSeen walk_seen(void) {
    WalkSeen w;
    w.busy = ram_walk_busy();
    w.kind = ram_walk_kind();
    w.frame = ram_walk_frame();
    w.cell = ram_sel_c();
    w.facing = ram_sel_d();
    return w;
}

/*----------------------
 | walk_step_leftovers
 | Description: What game_walk_step's ARM leaves: r7 = r6 + 0x20, with a step running r1 = walk_kind, then a finished step's nibble and mirror leftovers in r1-r3.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: sel_d
 | Params: c -- ARM register file; w -- the walk state the step started from
 | Returns: N/A
 ----------------------*/
static void walk_step_leftovers(PskCpu *c, WalkSeen w) {
    uint8_t at = w.kind == 0u ? (uint8_t)((w.cell + 1u) & 0x1Fu) : w.cell;
    c->r[7] = c->r[6] + STATE_SUB;
    if (w.busy != 0u) {
        c->r[1] = w.kind;
    }
    if (w.busy != 0u && w.kind < 2u && w.frame == 15u) {
        uint8_t exits = game_nibble(at);
        nibble_leftovers(c, at);
        if (w.kind == 1u) {
            c->r[3] = (w.cell - 1u) & 0x1Fu;
        }
        if (exits != 0xFu) {
            c->r[1] = mirror_last_bit(w.kind == 0u ? w.facing : (uint32_t)(exits & ~w.facing));
            if (w.kind == 0u) {
                c->r[3] = ram_sel_d();
            }
        }
    }
}






/*----------------------
 | MARKER_BASE
 | Description: RAM address of the marker slots, which the ARM passes as a slot pointer in r7.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MARKER_BASE 0x424u




/*----------------------
 | game_sprite_origin_regs
 | Description: Register entry for the fragment that zeroes r0-r1 and falls into game_sprite_info, giving a sprite's descriptor offsets alone.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_sprite_origin_regs(PskCpu *c) {
    c->r[0] = 0;
    c->r[1] = 0;
    game_sprite_info_regs(c);
}


/*----------------------
 | marker_start_leftovers
 | Description: What game_marker_start's ARM leaves for object type: r1 = frame 0's y offset, r2 = its bitmap word >> 16, r3 its second word, r4 its extra bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file; type -- object type started
 | Returns: N/A
 ----------------------*/
static void marker_start_leftovers(PskCpu *c, uint8_t type) {
    FrameInfo f = game_frame_info(type, 0);
    c->r[1] = (uint32_t)(int32_t)f.dy;
    c->r[2] = f.bitmap >> 16;
    c->r[3] = f.word;
    c->r[4] = f.extra;
}

/*----------------------
 | game_marker_start_regs
 | Description: Register entry for game_marker_start with x offset 0; r1 = frame 0's y offset, r2 = its bitmap word >> 16, r3 its second word, r4 its extra bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_marker_start_regs(PskCpu *c) {
    uint8_t type = (uint8_t)c->r[1];
    game_marker_start((uint8_t)c->r[0], type, 0);
    marker_start_leftovers(c, type);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_marker_frame_regs
 | Description: Register entry for game_marker_frame on r7's slot; r0-r5 the sprite (r4 0 for none), r8 its bitmap word, or once freed the frame info and r5 = 0xFF. The top-bit flag's r5 is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: marker
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_marker_frame_regs(PskCpu *c) {
    uint8_t at = (uint8_t)(c->r[7] - MARKER_BASE), type = ram_marker(at);
    SpriteInfo s = game_marker_frame((uint8_t)(at >> 2));
    FrameInfo f = game_frame_info(type, ram_marker((uint8_t)(at + 1u)));
    if (ram_marker(at) == 0xFFu) {
        c->r[0] = (uint32_t)(int32_t)f.dx;
        c->r[1] = (uint32_t)(int32_t)f.dy;
        c->r[2] = f.bitmap;
        c->r[3] = f.word;
        c->r[5] = 0xFFu;
    } else {
        c->r[0] = (uint32_t)(int32_t)s.x;
        c->r[1] = (uint32_t)(int32_t)s.y;
        c->r[2] = s.width;
        c->r[3] = s.height;
        c->r[5] = s.flags;
        c->r[8] = f.bitmap;
    }
    c->r[4] = s.bitmap;
    c->r[15] = psk_bx(c->r[14]);
}


/*----------------------
 | sound_start_run
 | Description: Runs game_sound_start on r5's id and leaves its ARM's registers: r1 = 0x5C4, r2 = 0xFF when queued, or snd_len and Timer 2's leftovers in r3-r5 when started.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: snd_mode, snd_playing, snd_len
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
static void sound_start_run(PskCpu *c) {
    uint8_t off = ram_snd_mode() == 2u, queued = ram_snd_playing() != 0xFFu;
    uint32_t mode = psk_ldr(TIMER_BASE + 2u * 16u + 8u) | 4u;
    game_sound_start((uint8_t)c->r[5]);
    c->r[1] = 0x5C4u;
    if (!off && queued) {
        c->r[2] = 0xFFu;
    } else if (!off) {
        c->r[2] = ram_snd_len();
        c->r[3] = 4u;
        c->r[4] = TIMER_BASE + 0x20u;
        c->r[5] = mode;
    }
}

/*----------------------
 | game_sound_start_regs
 | Description: Register entry for game_sound_start after its caller pushed lr; r1 = 0x5C4, r2 = 0xFF when queued, or snd_len and Timer 2's leftovers in r3-r5 when started. The snd_mode 2 and queued leftovers are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: snd_mode, snd_playing, snd_len
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_sound_start_regs(PskCpu *c) {
    sound_start_run(c);
    c->r[15] = psk_bx(psk_r32(c->r[13]));
    c->r[13] += 4u;
}

/*----------------------
 | game_sound_play_regs
 | Description: Register entry for the fragment that pushes lr and falls into game_sound_start; it leaves what game_sound_start_regs does and returns to lr.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: snd_mode, snd_playing, snd_len
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_sound_play_regs(PskCpu *c) {
    sound_start_run(c);
    c->r[15] = psk_bx(c->r[14]);
}



/*----------------------
 | game_event_roll_regs
 | Description: Register entry for game_event_roll; r1 = the popped lr, r2 = 0xFF, r3 = the picked entry's weight, r4 = the address after it. Converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: scene_id
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_event_roll_regs(PskCpu *c) {
    uint32_t list = psk_r32(ROM_EVENT_ODDS + ((uint32_t)ram_scene_id() - 7u) * 4u);
    uint32_t entry = list + game_event_roll() * 2u;
    c->r[1] = c->r[14];
    c->r[2] = 0xFFu;
    c->r[3] = psk_r16(entry) & 0xFFu;
    c->r[4] = entry + 2u;
    c->r[15] = psk_bx(c->r[14]);
}




/*----------------------
 | strip3_leftovers
 | Description: What game_reels_load_strip3's ARM leaves: r1 = strip_a's address, r2 = r3 = 0, r4 = the end of strip 3 in the card, r5 = 0x404.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
static void strip3_leftovers(PskCpu *c) {
    c->r[1] = 0x544u;
    c->r[2] = 0;
    c->r[3] = 0;
    c->r[4] = psk_r32(ROM_STRIPS + 16u);
    c->r[5] = 0x404u;
}




/*----------------------
 | game_strip_shuffle_regs
 | Description: Register entry for game_strip_shuffle; r2 = len * 16, r3 = the last first draw, r4 = the symbol the last swap moved, kept from entry when none swapped. That case is unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_strip_shuffle_regs(PskCpu *c) {
    uint8_t roll;
    int16_t moved = game_strip_shuffle((uint8_t)c->r[0], c->r[1], &roll);
    c->r[2] = c->r[0] << 4;
    c->r[3] = roll;
    if (moved >= 0) {
        c->r[4] = (uint32_t)moved;
    }
    c->r[15] = psk_bx(c->r[14]);
}


/*----------------------
 | pick_three_leftovers
 | Description: What game_strip_pick_three's ARM leaves: r2 = the highest pick, r3 = what the three-swap sort last moved (count - 3 if none), r4 = the bit of the third pick.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file; count -- game_strip_pick_three's result; d -- its picks in draw order
 | Returns: N/A
 ----------------------*/
static void pick_three_leftovers(PskCpu *c, uint8_t count, const uint8_t d[3]) {
    uint32_t t = (uint32_t)count - 3u;
    uint32_t lo = d[0], mid = d[1], hi = d[2];
    if (lo >= mid) {
        t = lo;
        lo = mid;
        mid = t;
    }
    if (lo >= hi) {
        t = lo;
        lo = hi;
        hi = t;
    }
    if (mid >= hi) {
        t = mid;
        hi = t;
    }
    c->r[2] = hi;
    c->r[3] = t;
    c->r[4] = 1u << d[2];
}

/*----------------------
 | game_strip_pick_three_regs
 | Description: Register entry for game_strip_pick_three; r2 = the highest pick, r3 = what the three-swap sort last moved (count - 3 if none), r4 = the bit of the third pick.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_strip_pick_three_regs(PskCpu *c) {
    uint8_t d[3];
    uint8_t count = game_strip_pick_three((uint8_t)c->r[0], c->r[1], d);
    pick_three_leftovers(c, count, d);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | sprite_regs
 | Description: Writes a drawn sprite's height, bitmap and flags to r3-r5, as the draws below leave them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file; s -- the sprite
 | Returns: N/A
 ----------------------*/
static void sprite_regs(PskCpu *c, SpriteInfo s) {
    c->r[3] = s.height;
    c->r[4] = s.bitmap;
    c->r[5] = s.flags;
}

/*----------------------
 | game_board_gather_regs
 | Description: Register entry for game_board_gather; r0-r5 x, y, cell, dir, depth and count, r7 the list; r5 = the new count.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_board_gather_regs(PskCpu *c) {
    c->r[5] = game_board_gather((int16_t)c->r[0], (int16_t)c->r[1], (uint8_t)c->r[2], (uint8_t)c->r[3], (uint8_t)c->r[4], (uint8_t)c->r[5], c->r[7]);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_anim_draw_regs
 | Description: Register entry for game_anim_draw; r0-r1 position, r2 slot; r2-r5 the drawn sprite's size, bitmap and flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_anim_draw_regs(PskCpu *c) {
    SpriteInfo s = game_anim_draw((int16_t)c->r[0], (int16_t)c->r[1], (uint8_t)c->r[2]);
    c->r[2] = s.width;
    sprite_regs(c, s);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_icon_draw_regs
 | Description: Register entry for game_icon_draw; r0-r1 position, r2 type, r3 marks; r3-r5 the overlay, r8 = 4 for four marks, else 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_icon_draw_regs(PskCpu *c) {
    c->r[8] = c->r[3] == 4u ? 4u : 0u;
    sprite_regs(c, game_icon_draw((int16_t)c->r[0], (int16_t)c->r[1], (uint8_t)c->r[2], (uint8_t)c->r[3]));
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_reel_window_draw_regs
 | Description: Register entry for game_reel_window_draw; r3 y, r7 position, r8 length, r9 strip; r2 = 0x1FF800, r4 the last symbol, r5 = 0, or r2 = -1 when off screen.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reel_window_draw_regs(PskCpu *c) {
    uint32_t last = game_reel_window_draw((int16_t)c->r[3], (uint16_t)c->r[7], (uint8_t)c->r[8], c->r[9]);
    c->r[2] = 0xFFFFFFFFu;
    if (last != 0u) {
        c->r[2] = 0x001FF800u;
        c->r[4] = last;
        c->r[5] = 0;
    }
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_reel_draw_regs
 | Description: Register entry for game_reel_draw; r3 y, r7 position, r8 length, r9 strip; r2-r3 = 8 and 11, r4 the last symbol, r5 = 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reel_draw_regs(PskCpu *c) {
    c->r[4] = game_reel_draw((int16_t)c->r[3], (uint16_t)c->r[7], (uint8_t)c->r[8], c->r[9]);
    c->r[2] = 8u;
    c->r[3] = 11u;
    c->r[5] = 0;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_meter_draw_regs
 | Description: Register entry for game_meter_draw; r10 y; r1-r5 the digit's y, size, bitmap and flags, r8 = meter.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: meter
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_meter_draw_regs(PskCpu *c) {
    SpriteInfo s = game_meter_draw((int16_t)c->r[10]);
    c->r[1] = (uint32_t)(int32_t)s.y;
    c->r[2] = s.width;
    sprite_regs(c, s);
    c->r[8] = ram_meter();
    c->r[15] = psk_bx(c->r[14]);
}


/*----------------------
 | game_count_of_draw_regs
 | Description: Register entry for game_count_of_draw; r0-r1 position, r2 count, r3 total; r2-r5 the slash sprite, r8 = total's units digit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_count_of_draw_regs(PskCpu *c) {
    uint8_t total = (uint8_t)c->r[3];
    SpriteInfo s = game_count_of_draw((int16_t)c->r[0], (int16_t)c->r[1], c->r[2], total);
    c->r[2] = s.width;
    sprite_regs(c, s);
    c->r[8] = total % 10u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_small_number_draw_regs
 | Description: Register entry for game_small_number_draw; r0-r1 position, r2 the number; r2-r5 the units digit sprite, r8 the digit sheet. Converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_small_number_draw_regs(PskCpu *c) {
    uint32_t n = c->r[2];
    SpriteInfo s = game_small_number_draw((int16_t)c->r[0], (int16_t)c->r[1], n);
    c->r[2] = s.width;
    sprite_regs(c, s);
    c->r[8] = s.bitmap - (n % 10u) * 4u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_wide_draw_regs
 | Description: Register entry for game_wide_draw; r0-r1 position, r2 picture number; r2-r5 the column sprite, its bitmap one column past the last drawn.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_wide_draw_regs(PskCpu *c) {
    SpriteInfo s = game_wide_draw((int16_t)c->r[0], (int16_t)c->r[1], (uint16_t)c->r[2]);
    c->r[2] = s.width;
    sprite_regs(c, s);
    c->r[15] = psk_bx(c->r[14]);
}


/*----------------------
 | game_walk_pending_regs
 | Description: Entry for game_walk_pending: a new cell sets r2 (lap length), r3 (steps + 1), at a lap r5 (0x404); turning slot 0 sets r5 (0x434) and nibble leftovers. The meter path and the back-path end are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: meter, meter_shown, walk_busy, walk_kind, walk_frame, walk_left, walk_back, walk_behind, steps, odds_set, step_result, sel_b, sel_c, sel_d
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_walk_pending_regs(PskCpu *c) {
    WalkSeen w = walk_seen();
    uint8_t steps = ram_steps();
    if (ram_meter_shown() != ram_meter()) {
        game_walk_pending();
        c->r[15] = psk_bx(c->r[14]);
        return;
    }
    if (w.busy == 0u && ram_walk_left() != 0u && ram_walk_back() == 0u && ram_walk_behind() == 0u) {
        c->r[2] = (uint32_t)(ram_odds_set() + 1u) << 5;
        c->r[3] = steps + 1u;
        if ((steps & 0x1Fu) == 0u) {
            c->r[5] = c->r[6] + 0x40u;
        }
    }
    game_walk_pending();
    walk_step_leftovers(c, w);
    if ((ram_walk_busy() == 0u || ram_walk_frame() == 0u) && ram_step_result() != 3u) {
        c->r[5] = c->r[6] + 0x70u;
        if ((ram_walk_left() == 0u || ram_walk_back() == 0u) == (ram_sel_b() != 0u)) {
            nibble_leftovers(c, ram_sel_c());
        }
    }
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_bag_reveal_regs
 | Description: Entry for game_bag_reveal: r1 the meter leftover or walk_frame count; a wait's end gives bag_count and r5 0x4C4, or strip 3's leftovers. Phase 3 gives r1 the frame index (-0xD0 if negative) and r5 0x18; only phase 0's waiting is verified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: meter, meter_shown, mode_done, scene_id, walk_frame, bag_count, scratch_first, scratch_count
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_bag_reveal_regs(PskCpu *c) {
    uint8_t phase = ram_mode_done(), wait = ram_scene_id(), frame = ram_walk_frame();
    int16_t shown;
    if (ram_meter_shown() != ram_meter()) {
        c->r[1] = meter_leftover();
        game_bag_reveal();
        c->r[15] = psk_bx(c->r[14]);
        return;
    }
    game_bag_reveal();
    if (phase == 3u) {
        shown = (int16_t)(ram_scratch_first() | (ram_scratch_count() << 8));
        shown = (int16_t)(shown - 0x48);
        c->r[1] = shown < 0 ? 0xFFFFFF30u : (uint32_t)(shown & 7);
        c->r[5] = 0x18u;
        c->r[15] = psk_bx(c->r[14]);
        return;
    }
    if (phase != 1u && phase != 2u) {
        c->r[1] = frame == 0x40u ? 0x40u : frame + 1u;
    }
    if (wait == 1u && phase == 2u) {
        strip3_leftovers(c);
    } else if (wait == 1u) {
        c->r[1] = ram_bag_count();
        c->r[5] = c->r[6] + 0x100u;
    }
    c->r[15] = psk_bx(c->r[14]);
}















/*----------------------
 | game_reels_load_pick_regs
 | Description: Register entry for game_reels_load_pick; r2-r4 are what game_strip_pick_three leaves and r5 = 0x404.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reels_load_pick_regs(PskCpu *c) {
    uint8_t d[3];
    uint8_t count = game_reels_load_pick(d);
    pick_three_leftovers(c, count, d);
    c->r[5] = 0x404u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_reels_load_pair_regs
 | Description: Register entry for game_reels_load_pair; r1 = 0x0200442B (the return address game_reel_random_pos pops), r2-r4 as game_strip_shuffle leaves them, r5 = 0x404. With no swap r4 is the loaded card strip's end, unverified.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: scratch_count, reel_len_b
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reels_load_pair_regs(PskCpu *c) {
    uint16_t strip = (uint16_t)(ram_scratch_count() + 8u);
    uint8_t roll;
    int16_t moved = game_reels_load_pair(&roll);
    c->r[1] = 0x0200442Bu;
    c->r[2] = (uint32_t)ram_reel_len_b() << 4;
    c->r[3] = roll;
    c->r[4] = moved >= 0 ? (uint32_t)moved : psk_r32(ROM_STRIPS + strip * 4u + 4u);
    c->r[5] = 0x404u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | sprite_all_regs
 | Description: Writes a sprite into r0-r5 in game_sprite_info_regs's layout, as the last sprite routine called leaves them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file; s -- the sprite
 | Returns: N/A
 ----------------------*/
static void sprite_all_regs(PskCpu *c, SpriteInfo s) {
    c->r[0] = (uint32_t)(int32_t)s.x;
    c->r[1] = (uint32_t)(int32_t)s.y;
    c->r[2] = s.width;
    c->r[3] = s.height;
    c->r[4] = s.bitmap;
    c->r[5] = s.flags;
}

/*----------------------
 | window_regs
 | Description: What the window draw's ARM leaves in r2-r5: r2 = -1 when off screen, else 0x1FF800, r3 the last frame buffer row it inverted, r4 the last symbol and r5 = 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: fb
 | Params: c -- ARM register file; y -- the window's y; last -- game_reel_window_draw's result
 | Returns: N/A
 ----------------------*/
static void window_regs(PskCpu *c, int16_t y, uint32_t last) {
    c->r[2] = 0xFFFFFFFFu;
    if (last != 0u) {
        c->r[2] = 0x001FF800u;
        c->r[3] = ram_fb((uint8_t)(y >= 16 ? 2 * y - 1 : y + 8));
        c->r[4] = last;
        c->r[5] = 0;
    }
}

/*----------------------
 | game_menu_a_draw_regs
 | Description: Register entry for game_menu_a_draw (ARM); r0-r5 the last sprite drawn.
 | Author: suinevere
 | Dependencies: sprite_all_regs
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_menu_a_draw_regs(PskCpu *c) {
    sprite_all_regs(c, game_menu_a_draw());
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_menu_row_draw_regs
 | Description: Register entry for game_menu_row_draw (ARM); r0-r5 the last sprite drawn.
 | Author: suinevere
 | Dependencies: sprite_all_regs
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_menu_row_draw_regs(PskCpu *c) {
    sprite_all_regs(c, game_menu_row_draw());
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_markers_draw_regs
 | Description: Register entry for game_markers_draw (ARM); r0-r5 and r8 as game_marker_frame_regs leaves them for the last slot stepped, r8 from the last slot still in use, all untouched when no slot was.
 | Author: suinevere
 | Dependencies: game_frame_info, sprite_all_regs
 | Globals: marker
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_markers_draw_regs(PskCpu *c) {
    uint8_t types[4], slot, last;
    SpriteInfo s;
    FrameInfo f;
    for (slot = 0; slot < 4u; slot++) {
        types[slot] = ram_marker((uint8_t)(slot * 4u));
    }
    s = game_markers_draw(&last);
    for (slot = 0; slot < 4u; slot++) {
        if (types[slot] != 0xFFu && ram_marker((uint8_t)(slot * 4u)) != 0xFFu) {
            c->r[8] = game_frame_info(types[slot], ram_marker((uint8_t)(slot * 4u + 1u))).bitmap;
        }
    }
    if (last != 0xFFu) {
        if (ram_marker((uint8_t)(last * 4u)) == 0xFFu) {
            f = game_frame_info(types[last], ram_marker((uint8_t)(last * 4u + 1u)));
            c->r[0] = (uint32_t)(int32_t)f.dx;
            c->r[1] = (uint32_t)(int32_t)f.dy;
            c->r[2] = f.bitmap;
            c->r[3] = f.word;
            c->r[4] = s.bitmap;
            c->r[5] = 0xFFu;
        } else {
            sprite_all_regs(c, s);
        }
    }
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_reel_window_show_regs
 | Description: Register entry for game_reel_window_show (ARM); r6 = 0x3C4, and unless in phase 8 r3 the window's y, r8 reel_len and r2-r5 as the window draw leaves them.
 | Author: suinevere
 | Dependencies: window_regs
 | Globals: reel_phase, reel_len
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reel_window_show_regs(PskCpu *c) {
    uint8_t drawn = ram_reel_phase() != 8u;
    uint32_t y = c->r[3], last = game_reel_window_show(&y);
    c->r[6] = 0x3C4u;
    if (drawn) {
        c->r[3] = y;
        c->r[8] = ram_reel_len();
        window_regs(c, (int16_t)y, last);
    }
    c->r[15] = psk_bx(c->r[14]);
}








/*----------------------
 | game_board_draw_regs
 | Description: Register entry for game_board_draw (ARM); r1 as its last item, character or walker draw leaves it and r8 the walker's front threshold, or cell_saved once a pass ran with sel_e clear.
 | Author: suinevere
 | Dependencies: game_sprite_info
 | Globals: board_list, cell_arg, cell_type, cell_saved, sel_e, mode, mode_done, mode_phase
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_board_draw_regs(PskCpu *c) {
    uint32_t result = game_board_draw(), r1;
    uint8_t count = (uint8_t)result, walker = ram_sel_e(), i, cell, arg, type;
    int16_t below = (int16_t)(result >> 8);
    int8_t ex, ey;
    r1 = (uint32_t)(int32_t)game_sprite_info(0, 0, 0).y;
    for (i = 0; i < count; i++) {
        ey = (int8_t)ram_board_list((uint8_t)(i * 3u));
        ex = (int8_t)ram_board_list((uint8_t)(i * 3u + 1u));
        cell = ram_board_list((uint8_t)(i * 3u + 2u));
        if (walker != 0u && ey > below) {
            r1 = 0;
            walker = 0;
        }
        arg = ram_cell_arg(cell);
        if (arg != 0xFFu) {
            r1 = (uint32_t)(int32_t)game_sprite_info(ex, ey, (uint16_t)(arg + 1u)).y;
        }
        type = ram_cell_type(cell);
        if (type != 0xFFu && type != 0x10u) {
            r1 = (uint32_t)(int32_t)ey;
        }
        if (ram_sel_e() == 0u && cell == ram_cell_saved()) {
            r1 = (uint32_t)(int32_t)ey;
            if (ram_mode() == 11u && ram_mode_done() >= 10u) {
                r1 = (uint32_t)(int32_t)ey - ram_mode_phase();
            }
        }
    }
    if (walker != 0u) {
        r1 = 0;
    }
    c->r[1] = r1;
    c->r[8] = ram_sel_e() == 0u ? ram_cell_saved() : (uint32_t)(int32_t)below;
    c->r[15] = psk_bx(c->r[14]);
}





/*----------------------
 | game_mode_leave_regs
 | Description: Register entry for game_mode_leave on r0 (ARM); like the original it returns through the link register its caller pushed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode_leave_regs(PskCpu *c) {
    game_mode_leave((uint8_t)c->r[0]);
    c->r[15] = psk_r32(c->r[13]) & ~3u;
    c->r[13] += 4u;
}

/*----------------------
 | pop_return
 | Description: Returns through the link register a fragment's caller pushed, as the original's pop at 0200375C or 020036C0 does.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
static void pop_return(PskCpu *c) {
    c->r[15] = psk_r32(c->r[13]) & ~3u;
    c->r[13] += 4u;
}

/*----------------------
 | game_mode_leave_changed_regs
 | Description: Register entry for the fragment at 0200373C: game_mode_leave_changed on r1, then the pop.
 | Author: suinevere
 | Dependencies: pop_return
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode_leave_changed_regs(PskCpu *c) {
    game_mode_leave_changed(c->r[1]);
    pop_return(c);
}

/*----------------------
 | game_mode_advance_regs
 | Description: Register entry for game_mode_advance on r1 (ARM, 020036C4); it ends with its own pushed return.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode_advance_regs(PskCpu *c) {
    game_mode_advance(c->r[1]);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_mode_advance_pop_regs
 | Description: Register entry for the fragment at 020036BC: game_mode_advance on r1, then the pop.
 | Author: suinevere
 | Dependencies: pop_return
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode_advance_pop_regs(PskCpu *c) {
    game_mode_advance(c->r[1]);
    pop_return(c);
}

/*----------------------
 | game_mode_pressed_regs
 | Description: Register entry for the fragment at 020036B4: sound 3, game_mode_advance as the sound start leaves r1, then the pop.
 | Author: suinevere
 | Dependencies: pop_return
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode_pressed_regs(PskCpu *c) {
    game_sound_begin(3);
    game_mode_advance(0x5C4u);
    pop_return(c);
}

/*----------------------
 | game_mode_press_regs
 | Description: Register entry for the fragment at 020036A4: game_mode_press, then the pop.
 | Author: suinevere
 | Dependencies: pop_return
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode_press_regs(PskCpu *c) {
    game_mode_press();
    pop_return(c);
}

/*----------------------
 | game_board_reel_screen_regs
 | Description: Register entry for game_board_reel_screen (ARM); r1 the meter's y, as its draw leaves it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_board_reel_screen_regs(PskCpu *c) {
    c->r[1] = (uint32_t)(int32_t)game_board_reel_screen().y;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_entry_regs
 | Description: Register entry for game_entry (ARM); like the original it first points the stack at 0x800. It never returns.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_entry_regs(PskCpu *c) {
    c->r[13] = 0x800u;
    game_entry();
    c->r[15] = psk_bx(c->r[14]);
}
