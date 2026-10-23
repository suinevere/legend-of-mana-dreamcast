/*----------------------
 | game.h
 | Description: Declarations for the VMU-shaped Ring Ring Land: its handler ids, its reads of the game's file by card address, and its functions.
 | Author: suinevere
 | Dependencies: vmu_hw.h, rrl_ram.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#include "vmu_hw.h"
#include "rrl_ram.h"

/*----------------------
 | ROM_BASE
 | Description: Card address of the game file's first byte; a card address minus it is the file offset vmu_file8 reads.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_BASE 0x02000000u

/*----------------------
 | rom8 / rom16 / rom32
 | Description: Read a little-endian byte, halfword or word of the game's file at a card address.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: N/A
 | Params: a -- card address
 | Returns: the value
 ----------------------*/
static inline uint8_t rom8(uint32_t a) { return vmu_file8((uint16_t)(a - ROM_BASE)); }
static inline uint16_t rom16(uint32_t a) { return (uint16_t)(rom8(a) | (rom8(a + 1u) << 8)); }
static inline uint32_t rom32(uint32_t a) { return rom16(a) | ((uint32_t)rom16(a + 2u) << 16); }

/*----------------------
 | ref8
 | Description: Reads a byte from the game's file when given a card address, or from vmu_ram when given a RAM offset, as the original's one load reached both.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: vmu_ram
 | Params: a -- card address or RAM offset
 | Returns: the byte
 ----------------------*/
static inline uint8_t ref8(uint32_t a) { return a >= ROM_BASE ? rom8(a) : vmu_r8((uint16_t)a); }

/*----------------------
 | game_asr
 | Description: Arithmetic right shift with the ARM rule for amounts of 32 and more.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value; n -- amount
 | Returns: shifted value
 ----------------------*/
static inline uint32_t game_asr(uint32_t v, uint32_t n) { return (uint32_t)(n < 32u ? ((int32_t)v >> n) : ((int32_t)v >> 31)); }

/*----------------------
 | FB_RUN_ROWS
 | Description: Rows of fb, blit_a and blit_b together, which the RAM map keeps in a row so a frame buffer row past 31 lands in the blit buffers as in the original.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FB_RUN_ROWS 96u

/*----------------------
 | fb_run / set_fb_run
 | Description: Read or write row of the frame buffer where the original's row can run past 31 into blit_a and blit_b.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: fb, blit_a, blit_b
 | Params: row -- row 0-95; v -- value
 | Returns: the row (fb_run)
 ----------------------*/
static inline uint32_t fb_run(uint8_t row) { return vmu_r32((uint16_t)(RAM_FB + VMU_IX("fb", row, FB_RUN_ROWS) * 4u)); }
static inline void set_fb_run(uint8_t row, uint32_t v) { vmu_w32((uint16_t)(RAM_FB + VMU_IX("fb", row, FB_RUN_ROWS) * 4u), v); }

/*----------------------
 | HANDLER_NONE / HANDLER_BUTTONS / HANDLER_TIMER_TICK / HANDLER_SOUND / HANDLER_DOCK
 | Description: Handler ids the interrupt slots and fiq_driver hold where the original held code addresses (0, 02002F8C, 02003E70, 02003B64, 020005A0).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define HANDLER_NONE 0u
#define HANDLER_BUTTONS 1u
#define HANDLER_TIMER_TICK 2u
#define HANDLER_SOUND 3u
#define HANDLER_DOCK 4u

/*----------------------
 | BTN_FIRE, BTN_RIGHT, BTN_LEFT, BTN_DOWN, BTN_UP
 | Description: Button bits as the interrupt controller reports them; bit 0 is fire.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BTN_FIRE 0x01u
#define BTN_RIGHT 0x02u
#define BTN_LEFT 0x04u
#define BTN_DOWN 0x08u
#define BTN_UP 0x10u

/*----------------------
 | BAG_SIZE
 | Description: Items the list holds before it counts as full.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BAG_SIZE 0x33u

/*----------------------
 | STATE_BLOCK
 | Description: The 128-byte state block (0x4C4 in the original), from points to save_tail, that the app's copy at 0x02000300 initialises.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STATE_BLOCK RAM_POINTS

/*----------------------
 | SPRITE_MASKED
 | Description: Blit flag: a mask bitmap follows the image, and masked pixels are cleared before the image is ORed in.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPRITE_MASKED 1u

/*----------------------
 | SPRITE_CLEAR
 | Description: Blit flag: clears the sprite's whole box before ORing the image in; honoured only for unpacked sprites.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPRITE_CLEAR 2u

/*----------------------
 | SPRITE_MIRROR
 | Description: Blit flag: mirrors each row left to right.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPRITE_MIRROR 4u

/*----------------------
 | SPRITE_DIGITS
 | Description: Sprite id of the digit sheet; digit d's rows start 4 * d bytes into its bitmap, and 10 is a blank.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPRITE_DIGITS 0x35u

/*----------------------
 | SpriteInfo
 | Description: A sprite placed from its descriptor: position, size, flip flags and bitmap address.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    int16_t x, y;
    uint8_t width, height, flags;
    uint32_t bitmap;
} SpriteInfo;

/*----------------------
 | FrameInfo
 | Description: One frame of an object type: offsets, bitmap word, a second word and the set's extra bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    int8_t dx, dy;
    uint32_t bitmap, word, extra;
} FrameInfo;

/*----------------------
 | WalkDir
 | Description: A held button and the pair of cell exit bits it walks along.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint8_t button, exits;
} WalkDir;

extern const WalkDir game_walk_dirs[4];

uint16_t game_random(uint16_t limit);
void game_fb_put_rows(uint8_t row, uint8_t count, uint32_t src, uint8_t invert);
void game_fb_present(void);
void game_fb_clear(void);
void game_state_reset(void);
SpriteInfo game_sprite_info(int16_t x, int16_t y, uint16_t id);
FrameInfo game_frame_info(uint8_t type, uint8_t frame);
void game_sprite_draw(int16_t x, int16_t y, uint8_t width, uint8_t height, uint32_t bitmap, uint8_t flags);
void game_markers_clear(void);
uint8_t game_marker_used(uint8_t index);
void game_anims_clear(void);
void game_quit_beep(uint8_t shift);
uint8_t game_wait_ticks(uint8_t which, uint32_t n);
void game_timer_tick(uint32_t line);
void game_sound_request(uint8_t request);
void game_sound_on(void);
void game_sound_off(void);
void game_save(void);
uint8_t game_byte_is(uint8_t index, uint32_t value);
uint8_t game_nibble(uint8_t index);
uint32_t game_digits_x(uint32_t x, uint32_t n);
uint32_t game_mirror_direction(uint32_t bits);
void game_sound_fiq(void);
void game_timer_setup(uint32_t n, uint32_t reload, uint32_t divider, uint32_t flags);
void game_timer_run(uint32_t n, uint8_t on);
void game_timer_irq(uint32_t n, uint8_t on);
uint32_t game_divide(uint32_t n, uint32_t d, uint32_t *rem);
uint32_t game_add_capped(uint32_t count, uint16_t dst, uint32_t add, uint32_t src);
uint8_t game_read_buttons(uint8_t check_hold);
void game_buttons_irq(uint32_t lines);
void game_set_irq_handler(uint8_t line, uint8_t handler);
void game_call_handler(uint8_t id, uint16_t line);
void game_lcd_mode(uint8_t low, uint8_t high);
void game_bag_add(uint8_t item);
void game_bag_remove(uint8_t index, uint8_t move_cursor);
uint16_t game_rom_word_8094(uint8_t index);
void game_mode1_leave(void);
void game_mode2_leave(void);
void game_mode4_leave(void);
void game_mode5_leave(void);
void game_mode7_leave(void);
void game_mode13_leave(void);
uint16_t game_strip_load(uint16_t strip, uint16_t dst, uint8_t add);
void game_reels_steer(void);
void game_reels_turn(void);
void game_reels_step(void);
void game_irq_dispatch(uint16_t pending);
void game_irq_callback(void);
void game_fiq_callback(void);
void game_menu_pick_a(void);
uint8_t game_menu_pick_b(void);
void game_menu_pick_row(void);
void game_meter_update(void);
void game_walk_step(void);
uint8_t game_cell_exits(void);
void game_walk_choose(void);
void game_board_enter(void);
uint8_t game_pick_by_sel(void);
uint8_t game_pick_by_set(void);
uint32_t game_bag_kinds(void);
uint8_t game_bag_count_first(void);
uint8_t game_bag_find_nth(void);
void game_buttons_init(uint8_t hold);
void game_anim_start(uint8_t slot, uint8_t id, uint8_t arg);
void game_marker_start(uint8_t slot, uint8_t type, uint8_t dx);
SpriteInfo game_marker_frame(uint8_t slot);
void game_quit_sound(void);
void game_sound_start(uint8_t id);
void game_sound_stop(void);
uint8_t game_cell_event(void);
uint8_t game_pick_bag_nth(void);
uint8_t game_event_roll(void);
void game_reels_load(void);
void game_reels_load_strip3(void);
void game_reels_load_raised(void);
int16_t game_strip_shuffle(uint8_t len, uint16_t strip, uint8_t *last_roll);
uint16_t game_reel_random_pos(uint8_t len);
uint8_t game_strip_next_symbol(uint8_t len, uint16_t pos, uint16_t strip);
uint8_t game_strip_pick_three(uint8_t len, uint16_t strip, uint8_t drawn[3]);
uint8_t game_board_gather(int16_t x, int16_t y, uint8_t cell, uint8_t dir, uint8_t depth, uint8_t count, uint16_t list);
SpriteInfo game_anim_draw(int16_t x, int16_t y, uint8_t slot);
SpriteInfo game_icon_draw(int16_t x, int16_t y, uint8_t type, uint8_t marks);
uint32_t game_reel_window_draw(int16_t y, uint16_t pos, uint8_t len, uint16_t strip);
uint32_t game_reel_draw(int16_t y, uint16_t pos, uint8_t len, uint16_t strip);
SpriteInfo game_meter_draw(int16_t y);
uint32_t game_number_draw(int16_t x, int16_t y, uint32_t n);
SpriteInfo game_count_of_draw(int16_t x, int16_t y, uint32_t count, uint8_t total);
SpriteInfo game_small_number_draw(int16_t x, int16_t y, uint32_t n);
SpriteInfo game_wide_draw(int16_t x, int16_t y, uint16_t id);
void game_walk_by_buttons(void);
void game_walk_pending(void);
void game_bag_reveal(void);
void game_reels_next_two(void);
void game_scene_step(void);
uint32_t game_payout(void);
void game_mode0_leave(void);
void game_mode10_leave(void);
void game_mode14_leave(void);
uint16_t game_pick_item(uint8_t how);
void game_power_down(void);
void game_timer_reset(uint8_t n, uint16_t reload, uint8_t divider);
void game_ticks_start(uint8_t n, uint16_t reload, uint8_t divider);
uint8_t game_board_fill(void);
void game_reels_load_single(void);
uint8_t game_reels_load_pick(uint8_t drawn[3]);
int16_t game_reels_load_pair(uint8_t *last_roll);

void game_sound_init(void);
void game_sound_begin(uint8_t id);
void game_quit(void);
SpriteInfo game_menu_a_draw(void);
SpriteInfo game_menu_row_draw(void);
SpriteInfo game_markers_draw(uint8_t *last);
uint32_t game_reel_window_show(uint32_t *y);
uint32_t game_reel_screen_draw(void);
void game_anims_tick(void);
uint32_t game_reel_anim_screen_draw(void);
void game_mode3_leave(void);
void game_mode9_leave(void);
void game_sound_play_waiting(void);
void game_standby(void);
void game_reel_game_step(void);
uint32_t game_board_draw(void);
void game_board_walk_screen(void);
void game_board_auto_screen(void);
uint32_t game_board_wait_screen(void);
void game_board_choice_draw(void);
void game_dock_quit(void);
void game_menu_pick_two(void);
void game_scene_screen_draw(void);
void game_mode11_leave(void);
void game_bag_screen_draw(void);
uint8_t game_event_shake(uint8_t type);
void game_reel_race_step(void);
void game_reel_spin_step(void);
void game_reel_pair_spin_step(void);
uint32_t game_bag_menu_step(void);
void game_dock_check(void);
void game_dock_screen(void);
void game_mode_leave(uint8_t mode);
void game_reel_pair_step(void);
void game_reel_pair_screen(void);
void game_mode_leave_changed(uint32_t was);
void game_mode_advance(uint32_t was);
void game_mode_press(void);
void game_board_auto_frame(void);
void game_reel_game_screen(void);
SpriteInfo game_board_reel_screen(void);
void game_reel_race_screen(void);
void game_menu_two_screen(void);
void game_bag_menu_screen(void);
void game_mode0_frame(void);
void game_mode1_frame(void);
void game_mode2_frame(void);
void game_mode3_frame(void);
void game_mode4_frame(void);
void game_mode5_frame(void);
void game_mode6_frame(void);
void game_mode8_frame(void);
void game_mode9_frame(void);
void game_mode10_frame(void);
void game_mode11_frame(void);
void game_mode13_frame(void);
void game_mode14_frame(void);
void game_mode_frame(void);
void game_quit_dialog(void);
void game_main_loop(void);
void game_entry(void);
void game_dock_irq(void);
#endif
