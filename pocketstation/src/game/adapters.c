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
 | game_mode1_leave_regs
 | Description: Register entry for game_mode1_leave; r7 = r6 + 0x20.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode1_leave_regs(PskCpu *c) {
    game_mode1_leave();
    c->r[7] = c->r[6] + STATE_SUB;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_mode2_leave_regs
 | Description: Register entry for game_mode2_leave; r7 = r6 + 0x20.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode2_leave_regs(PskCpu *c) {
    game_mode2_leave();
    c->r[7] = c->r[6] + STATE_SUB;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_mode4_leave_regs
 | Description: Register entry for game_mode4_leave; r7 = r6 + 0x20.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode4_leave_regs(PskCpu *c) {
    game_mode4_leave();
    c->r[7] = c->r[6] + STATE_SUB;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_mode7_leave_regs
 | Description: Register entry for game_mode7_leave; r7 = r6 + 0x20.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode7_leave_regs(PskCpu *c) {
    game_mode7_leave();
    c->r[7] = c->r[6] + STATE_SUB;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_mode13_leave_regs
 | Description: Register entry for game_mode13_leave; r1 = sel_c's new value, r7 = r6 + 0x20.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode13_leave_regs(PskCpu *c) {
    game_mode13_leave();
    c->r[1] = ram_sel_c();
    c->r[7] = c->r[6] + STATE_SUB;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_set_irq_handler_regs
 | Description: Register entry for game_set_irq_handler (ARM); r4 = the handler table base.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_set_irq_handler_regs(PskCpu *c) {
    game_set_irq_handler((uint8_t)c->r[0], c->r[1]);
    c->r[4] = 0x208u;
    c->r[15] = c->r[14] & ~3u;
}

/*----------------------
 | game_bag_remove_regs
 | Description: Register entry for game_bag_remove; r1 = the cursor when moved, r2 = 0, r5 = one past the last byte moved within the list base 0x4C4.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_bag_remove_regs(PskCpu *c) {
    uint32_t index = c->r[0], n = ram_bag_count() - 1u - index;
    game_bag_remove((uint8_t)index, (uint8_t)c->r[1]);
    if (c->r[1] == 1u) {
        c->r[1] = ram_bag_cursor();
    }
    c->r[2] = 0;
    c->r[5] = n ? 0x4C4u + index + n : 0x4C4u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_rom_word_8094_regs
 | Description: Register entry for game_rom_word_8094; r1 = 0x8000.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_rom_word_8094_regs(PskCpu *c) {
    c->r[0] = game_rom_word_8094((uint8_t)c->r[0]);
    c->r[1] = 0x8000u;
    c->r[15] = psk_bx(c->r[14]);
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
 | game_lcd_mode_regs
 | Description: Register entry for game_lcd_mode (ARM); r3 = 0, r4 = the LCD base.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_lcd_mode_regs(PskCpu *c) {
    game_lcd_mode((uint8_t)c->r[0], (uint8_t)c->r[1]);
    c->r[3] = 0;
    c->r[4] = LCD_MODE;
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
 | game_fb_present_regs
 | Description: Register entry for game_fb_present (ARM); r1-r5 and r8 hold what the last eight-word copy left.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_fb_present_regs(PskCpu *c) {
    game_fb_present();
    c->r[1] = 0x0D000180u;
    c->r[2] = 0;
    c->r[3] = ram_fb(24);
    c->r[4] = ram_fb(25);
    c->r[5] = ram_fb(26);
    c->r[8] = ram_fb(29);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_fb_clear_regs
 | Description: Register entry for game_fb_clear; r0 = the frame buffer, r1 = r2 = 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_fb_clear_regs(PskCpu *c) {
    game_fb_clear();
    c->r[0] = 0x240u;
    c->r[1] = 0;
    c->r[2] = 0;
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
 | game_state_reset_regs
 | Description: Register entry for game_state_reset (ARM); r2-r5 and r8 hold what the last eight-word copy left.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_state_reset_regs(PskCpu *c) {
    game_state_reset();
    c->r[2] = STATE_BLOCK + 0x80u;
    c->r[3] = psk_r32(0x02000360u);
    c->r[4] = psk_r32(0x02000364u);
    c->r[5] = psk_r32(0x02000368u);
    c->r[8] = psk_r32(0x02000374u);
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
 | game_wait_ticks_regs
 | Description: Register entry for game_wait_ticks (ARM); r4 = the counter's address, r5 = its value at the end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_wait_ticks_regs(PskCpu *c) {
    uint8_t which = (uint8_t)c->r[0];
    c->r[5] = game_wait_ticks(which, c->r[1]);
    c->r[4] = 0x5D4u + which * 2u;
    c->r[15] = c->r[14] & ~3u;
}

/*----------------------
 | game_sound_request_regs
 | Description: Register entry for game_sound_request; r1 = the sound driver state base.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_sound_request_regs(PskCpu *c) {
    game_sound_request((uint8_t)c->r[0]);
    c->r[1] = 0x5C4u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_sound_on_regs
 | Description: Register entry for game_sound_on; r4 = the sound register base.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_sound_on_regs(PskCpu *c) {
    game_sound_on();
    c->r[4] = 0x0D800000u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_sound_off_regs
 | Description: Register entry for game_sound_off; r4 = the sound register base.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_sound_off_regs(PskCpu *c) {
    game_sound_off();
    c->r[4] = 0x0D800000u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_byte_is_regs
 | Description: Register entry for game_byte_is; r3 = the table base.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_byte_is_regs(PskCpu *c) {
    c->r[0] = game_byte_is((uint8_t)c->r[0], c->r[1]);
    c->r[3] = 0x02009EFCu;
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
 | game_save_regs
 | Description: Register entry for game_save; r1 = the block, r2 = the interrupt controller, r3 = the old mask, r4 = the lines masked.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_save_regs(PskCpu *c) {
    c->r[3] = game_save();
    c->r[1] = STATE_BLOCK;
    c->r[2] = 0x0A000000u;
    c->r[4] = 0x2040u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_nibble_regs
 | Description: Register entry for game_nibble; r1 = the nibble's bit offset, r2 = its word shifted down by it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_nibble_regs(PskCpu *c) {
    uint32_t index = c->r[0];
    c->r[0] = game_nibble((uint8_t)index);
    nibble_leftovers(c, index);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_mirror_direction_regs
 | Description: Register entry for game_mirror_direction; r1 = the last bit tested.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mirror_direction_regs(PskCpu *c) {
    uint32_t bits = c->r[0];
    c->r[0] = game_mirror_direction(bits);
    c->r[1] = mirror_last_bit(bits);
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
 | game_timer_setup_regs
 | Description: Register entry for game_timer_setup; r3 = the mode written, r4 = the timer's base.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_timer_setup_regs(PskCpu *c) {
    c->r[3] = game_timer_setup(c->r[0], c->r[1], c->r[2], c->r[3]);
    c->r[4] = 0x0A800000u + c->r[0] * 16u;
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
 | game_add_capped_regs
 | Description: Register entry for game_add_capped; r3 = one past the last source byte, r4 = the last byte written.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_add_capped_regs(PskCpu *c) {
    c->r[4] = game_add_capped(c->r[0], c->r[1], c->r[2], c->r[3]);
    c->r[3] += c->r[0];
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
 | game_strip_load_regs
 | Description: Register entry for game_strip_load; r0 = the length, r3 = 0, r4 = one past the strip's last card byte.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_strip_load_regs(PskCpu *c) {
    uint32_t src = psk_r32(ROM_STRIPS + c->r[0] * 4u);
    c->r[0] = game_strip_load((uint16_t)c->r[0], c->r[1], (uint8_t)c->r[2]);
    c->r[3] = 0;
    c->r[4] = src + c->r[0];
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
 | game_menu_pick_a_regs
 | Description: Register entry for game_menu_pick_a; r0 = the return address it popped, r1 = mode_wait.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: mode_wait
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_menu_pick_a_regs(PskCpu *c) {
    game_menu_pick_a();
    c->r[0] = c->r[14];
    c->r[1] = ram_mode_wait();
    c->r[15] = psk_bx(c->r[14]);
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
 | game_menu_pick_row_regs
 | Description: Register entry for game_menu_pick_row; r0 = the return address it popped, r1 = mode_wait.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: mode_wait
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_menu_pick_row_regs(PskCpu *c) {
    game_menu_pick_row();
    c->r[0] = c->r[14];
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
 | game_meter_update_regs
 | Description: Register entry for game_meter_update; r1 = the delay limit while counting, else 0x5C4 after a sound request, else meter.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: meter, meter_shown, meter_delay
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_meter_update_regs(PskCpu *c) {
    c->r[1] = meter_leftover();
    game_meter_update();
    c->r[15] = psk_bx(c->r[14]);
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
 | game_walk_step_regs
 | Description: Register entry for game_walk_step; r7 = r6 + 0x20, and with a step running r1 = walk_kind, then what a finished step's nibble and mirror calls leave in r1-r3.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: walk_busy, walk_kind, walk_frame, sel_c, sel_d
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_walk_step_regs(PskCpu *c) {
    WalkSeen w = walk_seen();
    game_walk_step();
    walk_step_leftovers(c, w);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | walk_choose_leftovers
 | Description: What game_walk_choose's ARM will leave, worked out before it runs: with no step running r4 = sel_d, r3 = sel_b (0 for 2), r2 = the last bits tested.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: walk_busy, sel_b, sel_d, btn_prev
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
static void walk_choose_leftovers(PskCpu *c) {
    if (ram_walk_busy() == 0u) {
        uint8_t exits = game_cell_exits(), held = ram_btn_prev(), i;
        c->r[4] = ram_sel_d();
        c->r[3] = ram_sel_b() == 2u ? 0u : ram_sel_b();
        for (i = 0u; i < 4u; i++) {
            c->r[2] = game_walk_dirs[i].button;
            if (held & c->r[2]) {
                c->r[2] = exits & game_walk_dirs[i].exits;
                if (c->r[2] != 0u) {
                    break;
                }
            }
        }
    }
}

/*----------------------
 | game_walk_choose_regs
 | Description: Register entry for game_walk_choose; with no step running r4 = sel_d, r3 = sel_b (0 for 2), r2 = the last button or exit bits the direction scan tested.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: walk_busy, sel_b, sel_d, btn_prev
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_walk_choose_regs(PskCpu *c) {
    walk_choose_leftovers(c);
    game_walk_choose();
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_board_enter_regs
 | Description: Register entry for game_board_enter; after a save r2-r4 hold what game_save leaves (the controller, the old mask, the lines masked).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: meter
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_board_enter_regs(PskCpu *c) {
    uint32_t mask = psk_ldr(INT_MASK_SET);
    uint8_t saves = ram_meter() != 0u;
    game_board_enter();
    if (saves) {
        c->r[2] = INT_BASE;
        c->r[3] = mask;
        c->r[4] = 0x2040u;
    }
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_bag_kinds_regs
 | Description: Register entry for game_bag_kinds; r2 = 0, r3 = the kinds above the lowest, r4 = 0xFF, r5 = 0x4C4 + bag_count.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: bag_count, scratch_first
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_bag_kinds_regs(PskCpu *c) {
    uint32_t n = ram_bag_count(), kinds = game_bag_kinds();
    c->r[2] = 0u;
    c->r[3] = (kinds >> ram_scratch_first()) >> 1;
    c->r[4] = 0xFFu;
    c->r[5] = STATE_BLOCK + n;
    c->r[15] = psk_bx(c->r[14]);
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
 | game_bag_count_first_regs
 | Description: Register entry for game_bag_count_first; r1 = 0x8000 from the table read, r2 = 0, r3 = the count, r4 = 0xFF, r5 = 0x4C4 + bag_count. Converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: bag_count
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_bag_count_first_regs(PskCpu *c) {
    uint32_t n = ram_bag_count();
    c->r[3] = game_bag_count_first();
    c->r[1] = 0x8000u;
    c->r[2] = 0;
    c->r[4] = 0xFFu;
    c->r[5] = STATE_BLOCK + n;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_bag_find_nth_regs
 | Description: Register entry for game_bag_find_nth; r1 = 0x8000 from the table read, r3 = 0, r4 = 0xFF, r5 = 0x4C4 + the index. Converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_bag_find_nth_regs(PskCpu *c) {
    c->r[5] = STATE_BLOCK + game_bag_find_nth();
    c->r[1] = 0x8000u;
    c->r[3] = 0;
    c->r[4] = 0xFFu;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_buttons_init_regs
 | Description: Register entry for game_buttons_init (ARM); r4 = the handler table base the last handler store leaves.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_buttons_init_regs(PskCpu *c) {
    game_buttons_init((uint8_t)c->r[0]);
    c->r[4] = 0x208u;
    c->r[15] = psk_bx(c->r[14]);
}

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
 | game_anim_start_regs
 | Description: Register entry for game_anim_start; r2 = ROM_ANIM_DELAY.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_anim_start_regs(PskCpu *c) {
    game_anim_start((uint8_t)c->r[0], (uint8_t)c->r[1], (uint8_t)c->r[2]);
    c->r[2] = ROM_ANIM_DELAY;
    c->r[15] = psk_bx(c->r[14]);
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
 | game_quit_sound_regs
 | Description: Register entry for game_quit_sound; r2 = 1 when the kernel flag stopped it, else snd_mode, and r4 the sound register base once it played. Converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: docked, com_flags, snd_mode
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_quit_sound_regs(PskCpu *c) {
    if (ram_docked() == 0u) {
        c->r[2] = 1u;
        if ((psk_ldr(ram_com_flags()) & 1u) == 0u) {
            c->r[2] = ram_snd_mode();
            if (c->r[2] != 2u) {
                c->r[4] = 0x0D800000u;
            }
        }
    }
    game_quit_sound();
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
 | game_sound_stop_regs
 | Description: Register entry for game_sound_stop; r1 = 0x5C4, r2 = 0xFF, r3 = 4, r4 the sound or interrupt base, r5 = Timer 2's interrupt line. The idle path's r4 is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: snd_playing
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_sound_stop_regs(PskCpu *c) {
    uint8_t idle = ram_snd_playing() == 0xFFu;
    game_sound_stop();
    c->r[1] = 0x5C4u;
    c->r[2] = 0xFFu;
    c->r[3] = 4u;
    c->r[4] = idle ? 0x0D800000u : INT_BASE;
    c->r[5] = 0x2000u;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_cell_event_regs
 | Description: Register entry for game_cell_event; r0 = the mode, r2 = 0xE0 + sel_c once the cell was read, r5 = 0x404 after a refill. The refill's r5 is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: step_result, sel_c, cell_type, cell_arg
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_cell_event_regs(PskCpu *c) {
    uint8_t result = ram_step_result(), cell = ram_sel_c();
    uint8_t refill = result != 0u && result != 2u && result != 3u && ram_cell_type(cell) == 0xFFu && ram_cell_arg(cell) == 0u;
    c->r[0] = game_cell_event();
    if (result != 0u && result != 3u) {
        c->r[2] = 0xE0u + cell;
    }
    if (refill) {
        c->r[5] = 0x404u;
    }
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
 | game_pick_bag_nth_regs
 | Description: Register entry for game_pick_bag_nth; r3 = 0xFF, r4 = the picked weight.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_pick_bag_nth_regs(PskCpu *c) {
    uint8_t i = game_pick_bag_nth();
    c->r[3] = 0xFFu;
    c->r[4] = (psk_r32(ROM_ODDS_NTH) >> (i * 8u)) & 0xFFu;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | reels_load_leftovers
 | Description: What game_reels_load's ARM leaves: r1 = strip_c's address, r2 = r3 = 0, r4 = the end of strip 2 in the card, r5 = 0x404.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
static void reels_load_leftovers(PskCpu *c) {
    c->r[1] = 0x564u;
    c->r[2] = 0;
    c->r[3] = 0;
    c->r[4] = psk_r32(ROM_STRIPS + 12u);
    c->r[5] = 0x404u;
}

/*----------------------
 | game_reels_load_regs
 | Description: Register entry for game_reels_load; r1 = strip_c's address, r2 = 0, r3 = 0, r4 = the end of strip 2 in the card, r5 = 0x404.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reels_load_regs(PskCpu *c) {
    game_reels_load();
    reels_load_leftovers(c);
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
 | game_reels_load_strip3_regs
 | Description: Register entry for game_reels_load_strip3; r1 = strip_a's address, r2 = r3 = 0, r4 = the end of strip 3 in the card, r5 = 0x404. Converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reels_load_strip3_regs(PskCpu *c) {
    game_reels_load_strip3();
    strip3_leftovers(c);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | raised_leftovers
 | Description: What game_reels_load_raised's ARM leaves: r1 = strip_a's address, r2 = lap_mark, r3 = one past strip_c's copied symbols, r4 = the last symbol written, r5 = 0x404.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lap_mark, strip_a
 | Params: c -- ARM register file; len -- strip_c_len before the load
 | Returns: N/A
 ----------------------*/
static void raised_leftovers(PskCpu *c, uint8_t len) {
    c->r[1] = 0x544u;
    c->r[2] = ram_lap_mark();
    c->r[3] = 0x564u + len;
    c->r[4] = ram_strip_a((uint8_t)(len - 1u));
    c->r[5] = 0x404u;
}

/*----------------------
 | game_reels_load_raised_regs
 | Description: Register entry for game_reels_load_raised; r1 = strip_a's address, r2 = lap_mark, r3 = one past strip_c's copied symbols, r4 = the last symbol written, r5 = 0x404. Converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: strip_c_len, lap_mark, strip_a
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reels_load_raised_regs(PskCpu *c) {
    uint8_t len = ram_strip_c_len();
    game_reels_load_raised();
    raised_leftovers(c, len);
    c->r[15] = psk_bx(c->r[14]);
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
 | game_strip_next_symbol_regs
 | Description: Register entry for game_strip_next_symbol; r1 = the popped return address, r3 = len.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_strip_next_symbol_regs(PskCpu *c) {
    uint32_t len = c->r[0];
    c->r[0] = game_strip_next_symbol((uint8_t)len, (uint16_t)c->r[1], c->r[2]);
    c->r[1] = c->r[14];
    c->r[3] = len;
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
 | game_veneer_039c2_regs
 | Description: Register entry for the ARM veneer at 02008384, a tail jump through r12 to 020039C2, which stays on R1 until its callers' batch.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_veneer_039c2_regs(PskCpu *c) {
    c->r[12] = 0x020039C3u;
    c->r[15] = psk_bx(c->r[12]);
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
 | game_number_draw_regs
 | Description: Register entry for game_number_draw; r0-r1 position, r2 the number; r2-r5 the digit sprite with r4 the units digit, r8 the digit sheet.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_number_draw_regs(PskCpu *c) {
    SpriteInfo s = game_sprite_info((int16_t)c->r[0], (int16_t)c->r[1], SPRITE_DIGITS);
    c->r[8] = s.bitmap;
    s.bitmap = game_number_draw((int16_t)c->r[0], (int16_t)c->r[1], c->r[2]);
    c->r[2] = s.width;
    sprite_regs(c, s);
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
 | game_walk_by_buttons_regs
 | Description: Register entry for game_walk_by_buttons; r2-r4 as game_walk_choose leaves them when it ran, then what game_walk_step leaves.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: meter, meter_shown, walk_busy, walk_kind, walk_frame, sel_b, sel_c, sel_d, btn_prev
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_walk_by_buttons_regs(PskCpu *c) {
    WalkSeen w = walk_seen();
    if (ram_meter_shown() == ram_meter()) {
        walk_choose_leftovers(c);
    }
    game_walk_by_buttons();
    walk_step_leftovers(c, w);
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
 | game_reels_next_two_regs
 | Description: Register entry for game_reels_next_two (r5 = 0x404 from its one caller); r1 = the second symbol call's return address, r2 = strip_b's address, r3 = reel_len_b.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: reel_len_b
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reels_next_two_regs(PskCpu *c) {
    game_reels_next_two();
    c->r[1] = 0x0200227Du;
    c->r[2] = 0x554u;
    c->r[3] = ram_reel_len_b();
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_scene_step_regs
 | Description: Register entry for game_scene_step; r1 = btn_new, r2 = 1, r3 = mode_wait after the count, with each scene's own leftovers. Scene 2's are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: mode_wait, btn_new, scene_id, mode_done, scratch_wait, scratch_first, scratch_count, strip_c_len
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_scene_step_regs(PskCpu *c) {
    uint8_t wait = ram_mode_wait(), scene = ram_scene_id(), started = ram_mode_done() == 1u;
    uint8_t ready = wait <= 1u && (ram_btn_new() & BTN_FIRE) != 0u, count = ram_scratch_count();
    uint8_t len = ram_strip_c_len(), flag = ram_scratch_wait();
    game_scene_step();
    c->r[1] = ram_btn_new();
    c->r[2] = BTN_FIRE;
    c->r[3] = wait != 0u ? wait - 1u : 0u;
    if (scene == 1u && !started && ready) {
        c->r[2] = flag == 0xFFu ? 0x68u : 0u;
    } else if (scene == 1u && started) {
        c->r[4] = ram_scratch_first();
    } else if (scene == 2u && started) {
        c->r[3] = count != 0u ? count - 1u : 0u;
        if (count == 1u) {
            raised_leftovers(c, len);
            c->r[1] = 0x5C4u;
        }
    }
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_payout_regs
 | Description: Register entry for game_payout; r1 = lap_points before plus the points added (32-bit), r4 = marker type 0x68's frame 0 extra bits, r5 = the points added.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lap_points
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_payout_regs(PskCpu *c) {
    uint16_t lap = ram_lap_points();
    c->r[5] = game_payout();
    c->r[1] = lap + c->r[5];
    c->r[4] = game_frame_info(0x68u, 0).extra;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_mode0_leave_regs
 | Description: Register entry for game_mode0_leave; r2-r5 hold what game_reels_load leaves.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode0_leave_regs(PskCpu *c) {
    game_mode0_leave();
    reels_load_leftovers(c);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_mode10_leave_regs
 | Description: Register entry for game_mode10_leave; r1-r4 hold what game_marker_start leaves for type 0x5B.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode10_leave_regs(PskCpu *c) {
    game_mode10_leave();
    marker_start_leftovers(c, 0x5Bu);
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_mode14_leave_regs
 | Description: Register entry for game_mode14_leave; r1 = 0x8000, r2 = 0, r3 = the count, r4 = 0xFF, r5 = 0x4C4 + bag_count, as game_bag_count_first leaves them. Converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: scratch_lo, bag_count
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_mode14_leave_regs(PskCpu *c) {
    game_mode14_leave();
    c->r[1] = 0x8000u;
    c->r[2] = 0;
    c->r[3] = ram_scratch_lo();
    c->r[4] = 0xFFu;
    c->r[5] = STATE_BLOCK + ram_bag_count();
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | next_roll
 | Description: What the next game_random(limit) will return, worked out from the seed without stepping it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: seed
 | Params: limit -- exclusive upper bound
 | Returns: the coming roll
 ----------------------*/
static uint16_t next_roll(uint16_t limit) {
    uint16_t s = (uint16_t)((ram_seed() + 1u) * 0x3D09u);
    return (uint16_t)(((uint32_t)limit * s) >> 16);
}

/*----------------------
 | weights_left
 | Description: The weight word a weighted pick's ARM leaves in r2 for a roll: shifted down by a byte per weight the roll passed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: weights -- four byte weights, lowest byte first; roll -- the roll below 100
 | Returns: the shifted word
 ----------------------*/
static uint32_t weights_left(uint32_t weights, uint16_t roll) {
    while (roll >= (weights & 0xFFu)) {
        roll = (uint16_t)(roll - (weights & 0xFFu));
        weights >>= 8;
    }
    return weights;
}

/*----------------------
 | game_pick_item_regs
 | Description: Register entry for game_pick_item; r0 = r3 = the item, then the matched entry's kind, address and odds pick in r2, r4, r5. How 2 leaves its pick's weights in r2 and r4; how 2 and the fixed items leave r5 = r6 + 0x20. Both are unverified.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: seed, odds_set, scene_id
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_pick_item_regs(PskCpu *c) {
    uint8_t how = (uint8_t)c->r[0];
    uint8_t fixed = how == 1u && ram_scene_id() == 13u && next_roll(100u) < 80u;
    uint32_t weights = weights_left(psk_r32(ROM_ODDS_SET + ram_odds_set() * 4u), next_roll(100u)), at;
    c->r[0] = game_pick_item(how);
    c->r[3] = c->r[0];
    if (how == 2u) {
        c->r[2] = weights;
        c->r[4] = weights & 0xFFu;
    } else if (!fixed) {
        at = ROM_WORDS_8094 + c->r[0] * 2u;
        c->r[2] = psk_r16(at) & 0xFFu;
        c->r[4] = at;
        c->r[5] = psk_r16(at) >> 8;
    }
    if (how == 2u || fixed) {
        c->r[5] = c->r[6] + STATE_SUB;
    }
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_power_down_regs
 | Description: Register entry for game_power_down (ARM); r0 = the interrupt controller, r1 = the input word last read, r4-r5 what stopping Timer 2 leaves. Converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_power_down_regs(PskCpu *c) {
    uint32_t mode = psk_ldr(TIMER_BASE + 2u * 16u + 8u) & ~4u;
    c->r[1] = game_power_down();
    c->r[0] = INT_BASE;
    c->r[4] = TIMER_BASE + 0x20u;
    c->r[5] = mode;
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_ticks_start_regs
 | Description: Register entry for game_ticks_start (ARM) on r0-r2; r1 = game_timer_tick's address, r4 = the timer's base, r5 = the mode that started it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_ticks_start_regs(PskCpu *c) {
    uint8_t n = (uint8_t)c->r[0];
    c->r[5] = game_ticks_start(n, (uint16_t)c->r[1], (uint8_t)c->r[2]);
    c->r[1] = 0x02003E70u;
    c->r[4] = TIMER_BASE + n * 16u;
    c->r[15] = c->r[14] & ~3u;
}

/*----------------------
 | game_timer_reset_regs
 | Description: Register entry for game_timer_reset (ARM) on r0-r2; r4 = the timer's base, r5 = its interrupt line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_timer_reset_regs(PskCpu *c) {
    uint8_t n = (uint8_t)c->r[0];
    game_timer_reset(n, (uint16_t)c->r[1], (uint8_t)c->r[2]);
    c->r[4] = TIMER_BASE + n * 16u;
    c->r[5] = n == 0u ? 0x80u : n == 1u ? 0x100u : 0x2000u;
    c->r[15] = c->r[14] & ~3u;
}

/*----------------------
 | game_board_fill_regs
 | Description: Register entry for game_board_fill; r5 = cell_arg's address, r8 = the choice kinds still open when the even-cell pass ran, else kept from entry. The kept case is unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: board_args, board_steps
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_board_fill_regs(PskCpu *c) {
    uint8_t kinds = game_board_fill();
    c->r[5] = c->r[6] + 0xC0u;
    if (ram_board_args() != ram_board_steps()) {
        c->r[8] = kinds;
    }
    c->r[15] = psk_bx(c->r[14]);
}

/*----------------------
 | game_reels_load_single_regs
 | Description: Register entry for game_reels_load_single; r2 = lap_mark, r3 = one past strip_c's copied symbols, r4 = the last symbol written, r5 = 0x404, as game_reels_load_raised leaves them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: strip_c_len
 | Params: c -- ARM register file
 | Returns: N/A
 ----------------------*/
void game_reels_load_single_regs(PskCpu *c) {
    uint8_t len = ram_strip_c_len();
    game_reels_load_single();
    raised_leftovers(c, len);
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
