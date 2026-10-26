/*----------------------
 | record_synthetic.c
 | Description: Records the game functions no script reaches, and the level 0 to 10 branch outcomes no script takes. Each case loads a nearby recorded state, sets the steering fields and calls the wrapper, appending synthetic records to DIR.
 | Author: suinevere
 | Dependencies: game.h, vmu_host.h, vmu_record.h, rrl_ram.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "vmu_host.h"
#include "vmu_record.h"

#define FILE_MAX 0x10000u

static uint8_t g_file[FILE_MAX];
static uint32_t g_file_len;
static uint32_t g_words[64], g_nwords;
static const char *g_dir;
static uint8_t g_queue[64];
static int g_queue_n, g_queue_at;
static uint32_t g_frame;

static void fail(const char *what, const char *arg) {
    fprintf(stderr, "record_synthetic: %s %s\n", what, arg);
    exit(2);
}

void vmu_index_fault(const char *field, unsigned i) {
    fprintf(stderr, "record_synthetic: index %u past %s at frame %u\n", i, field, g_frame);
    exit(8);
}

static void load_file(const char *path) {
    static uint8_t card[0x20000];
    FILE *f = fopen(path, "rb");
    if (!f || fread(card, 1, sizeof card, f) != sizeof card) fail("cannot read a 128 KB card from", path);
    fclose(f);
    for (unsigned b = 1, n = 0; b >= 1 && b <= 15 && n < 15; n++) {
        if (g_file_len + 0x2000u > FILE_MAX) fail("file longer than 64 KB in", path);
        memcpy(g_file + g_file_len, card + b * 0x2000u, 0x2000u);
        g_file_len += 0x2000u;
        unsigned link = card[b * 0x80u + 8u] | (card[b * 0x80u + 9u] << 8);
        if (link == 0xFFFFu) break;
        b = link + 1u;
    }
}

static void load_entry(const char *path) {
    static uint8_t wram[0x800];
    FILE *f = fopen(path, "rb");
    if (!f || fread(wram, 1, sizeof wram, f) != sizeof wram || fread(&g_nwords, 4, 1, f) != 1 || g_nwords > 64 ||
        fread(g_words, 4, g_nwords, f) != g_nwords) {
        fail("bad entry state", path);
    }
    fclose(f);
}

static RecFn fn(const char *name) {
    RecFn f = rec_lookup(name);
    if (!f) fail("no wrapper for", name);
    return f;
}

#define CALL0(name) ((void (*)(void))fn(name))()
#define CALL0R(type, name) ((type (*)(void))fn(name))()

static void frame_cb(void) {
    g_frame++;
    rec_set_frame(g_frame);
    vmu_host_set_buttons(g_queue_at < g_queue_n ? g_queue[g_queue_at++] : 0u);
}

static void queue(const uint8_t *masks, int n) {
    memcpy(g_queue, masks, (size_t)n);
    g_queue_n = n;
    g_queue_at = 0;
}

static void base(const char *name, int off, uint8_t value, int nth) {
    char path[600];
    snprintf(path, sizeof path, "%s/%s.rec", g_dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) fail("no records in", path);
    static uint8_t buf[1u << 22];
    size_t n = fread(buf, 1, sizeof buf, f);
    if (!feof(f)) fail("records longer than 4 MB in", path);
    fclose(f);
    for (size_t at = 0; at + 14 <= n;) {
        uint32_t len = buf[at + 4] | (buf[at + 5] << 8) | (buf[at + 6] << 16) | ((uint32_t)buf[at + 7] << 24);
        if (len < 14u || at + len > n) fail("bad record in", path);
        uint32_t nbits = buf[at + 12] | (buf[at + 13] << 8), p = (uint32_t)at + 14u + (nbits + 7u) / 8u;
        uint8_t flags = buf[p + 1];
        p += 6;
        p += 1u + buf[p];
        p += 1u + buf[p];
        uint8_t nargs = buf[p++];
        for (uint8_t i = 0; i < nargs; i++) {
            uint8_t kind = buf[p], size = buf[p + 1];
            p += 2u + size + (kind == 1u ? size : 0u);
        }
        p += 1u + buf[p];
        if (!(flags & 1u) && (off < 0 || buf[p + (uint32_t)off] == value) && nth-- == 0) {
            memcpy(vmu_ram, buf + p, sizeof vmu_ram);
            memcpy(vmu_fb(), buf + p + sizeof vmu_ram, VMU_FB_BYTES);
            return;
        }
        at += len;
    }
    fail("no matching base record in", path);
}

static void setup(void) {
    rec_pause(1);
    vmu_host_set_buttons(0);
    vmu_irq_disable(0x3FFFu);
    vmu_set_handler(fn("game_irq_callback"));
    vmu_irq_enable(VMU_LINE_RTC | VMU_LINE_DOCK);
    ((void (*)(uint8_t))fn("game_ticks_start"))(VMU_TICK_FRAME);
    CALL0("game_sound_init");
    ((void (*)(uint8_t))fn("game_buttons_init"))(0x60u);
    rec_pause(0);
}

static void press(uint8_t mask) {
    set_btn_press(mask);
    set_btn_new(mask);
}

static void fill_bag(uint8_t count) {
    rec_pause(1);
    set_bag_count(0);
    set_bag_full(0);
    for (uint8_t i = 0; i < count; i++) {
        set_bag_items(i, (uint8_t)((uint16_t (*)(uint8_t))fn("game_pick_item"))(2u));
    }
    set_bag_count(count);
    set_bag_full(count == BAG_SIZE);
    rec_pause(0);
}

static void bag_menu(uint8_t count, const uint8_t *masks, int n) {
    base("game_mode_frame", -1, 0, 0);
    fill_bag(count);
    set_mode(14);
    CALL0("game_mode14_leave");
    for (int i = 0; i < n && ram_mode() == 14u; i++) {
        press(masks[i]);
        CALL0("game_mode14_frame");
    }
}

static void case_bag(void) {
    static const uint8_t browse[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BTN_RIGHT, BTN_LEFT, BTN_FIRE, BTN_RIGHT,
                                     BTN_RIGHT, BTN_LEFT, BTN_UP, BTN_FIRE, BTN_FIRE, BTN_DOWN, BTN_UP, BTN_FIRE, 0,
                                     BTN_FIRE, BTN_FIRE, BTN_FIRE, BTN_DOWN, BTN_UP, BTN_UP, BTN_FIRE, BTN_UP};
    static const uint8_t full[] = {0, BTN_DOWN, BTN_UP, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   BTN_FIRE, BTN_LEFT, BTN_FIRE, BTN_FIRE, BTN_FIRE, BTN_UP, BTN_FIRE};
    static const uint8_t discard[] = {0, BTN_DOWN, BTN_FIRE};
    static const uint8_t one[] = {BTN_RIGHT, BTN_FIRE, BTN_RIGHT, BTN_FIRE, BTN_FIRE};
    bag_menu(8, browse, (int)sizeof browse);
    bag_menu(BAG_SIZE, full, (int)sizeof full);
    bag_menu(BAG_SIZE, discard, (int)sizeof discard);
    bag_menu(1, one, (int)sizeof one);
}

static void case_modes(void) {
    base("game_mode_frame", -1, 0, 1);
    set_mode(12);
    CALL0("mode12_leave");
    for (int i = 0; i < 7 && ram_mode() == 12u; i++) {
        press(i == 2 || i == 6 ? BTN_FIRE : 0u);
        CALL0("mode12_frame");
    }
    for (uint8_t done = 0; done < 3; done++) {
        base("game_mode_frame", -1, 0, 2);
        fill_bag(3);
        set_mode(15);
        set_mode_done(done);
        CALL0("mode15_leave");
        for (int i = 0; i < 7 && ram_mode() == 15u; i++) {
            press(i == 2 || i == 6 ? BTN_FIRE : 0u);
            CALL0("mode15_frame");
        }
    }
}

static void case_board(void) {
    static const uint8_t scenes[] = {9, 11, 12};
    base("game_cell_event", -1, 0, 0);
    set_step_result(3);
    set_odds_set(2);
    CALL0R(uint8_t, "game_cell_event");
    base("game_cell_event", -1, 0, 0);
    set_step_result(1);
    set_cell_type(ram_sel_c(), 0x10u);
    set_bonus_done(0);
    set_odds_set(0);
    set_steps(5);
    CALL0R(uint8_t, "game_cell_event");
    for (int i = 0; i < 3; i++) {
        for (int k = 0; k < 2; k++) {
            base("game_reel_game_step", RAM_SCENE_ID, scenes[i], k);
            CALL0R(uint8_t, "game_event_roll");
        }
    }
}

static void case_reels(void) {
    static const uint8_t waits[] = {0, 1, 3, 6, 17, 30};
    uint8_t (*shake)(uint8_t) = (uint8_t (*)(uint8_t))fn("game_event_shake");
    void (*price)(int16_t, int16_t) = (void (*)(int16_t, int16_t))fn("price_draw");
    for (int i = 0; i < 6; i++) {
        base("game_reel_pair_step", -1, 0, 0);
        set_mode_wait(waits[i]);
        shake(10u);
        base("game_reel_pair_step", -1, 0, 0);
        set_mode_wait(waits[i]);
        shake(11u);
    }
    base("game_reel_anim_screen_draw", -1, 0, 0);
    price(0x16, 0x10);
    price(0, 8);
    price(0x0B, 0x11);
    base("game_mode9_leave", -1, 0, 0);
    CALL0("game_reels_load_raised");
    base("game_bag_reveal", -1, 0, 0);
    CALL0("game_reels_load_strip3");
}

static void case_reveal(void) {
    static const int16_t pos[] = {0x60, 0x50, 0x48, 0x40, 0, -0xCF, -0xD0};
    for (int i = 0; i < 7; i++) {
        base("game_bag_reveal", -1, 0, 0);
        set_meter_shown(ram_meter());
        set_mode_done(3);
        set_scratch_first((uint8_t)pos[i]);
        set_scratch_count((uint8_t)((uint16_t)pos[i] >> 8));
        CALL0("game_bag_reveal");
    }
    for (int last = 0; last < 2; last++) {
        base("game_bag_reveal", -1, 0, 0);
        fill_bag(4);
        set_meter_shown(ram_meter());
        set_mode_done(1);
        set_scene_id(1);
        set_bag_cursor(last ? 4u : 1u);
        CALL0("game_bag_reveal");
    }
}

static void case_quit(void) {
    static const uint8_t dialog[] = {0, 0, BTN_UP, 0, BTN_DOWN, 0, BTN_FIRE, 0};
    void (*rows)(uint8_t, uint8_t, uint32_t, uint8_t) = (void (*)(uint8_t, uint8_t, uint32_t, uint8_t))fn("game_fb_put_rows");
    base("game_mode_frame", -1, 0, 0);
    rows(8, 7, 0x020064B0u, 1u);
    rows(0x11, 7, 0x020064CCu, 0u);
    for (uint8_t mode = 0; mode < 3; mode++) {
        set_docked(0);
        set_snd_mode(mode);
        CALL0("game_quit_sound");
    }
    set_snd_mode(0);
    set_docked(1);
    CALL0("game_quit_sound");
    set_docked(0);
    vmu_host_set_sys(VMU_SYS_MUTE);
    CALL0("game_quit_sound");
    vmu_host_set_sys(0);
    base("game_call_handler", -1, 0, 0);
    CALL0("game_dock_irq");
    setup();
    base("game_mode_frame", -1, 0, 0);
    set_docked(0);
    queue(dialog, (int)sizeof dialog);
    CALL0("game_quit_dialog");
    queue(0, 0);
    vmu_host_set_buttons(0);
    base("game_mode_frame", -1, 0, 0);
    CALL0("game_power_down");
    setup();
    base("game_mode_frame", -1, 0, 0);
    vmu_irq_enable(VMU_LINE_BUTTONS);
    vmu_host_set_buttons(BTN_FIRE);
    CALL0("game_standby");
    vmu_host_set_buttons(0);
    setup();
}

static void case_levels01(void) {
    void (*cell_meter)(uint8_t) = (void (*)(uint8_t))fn("cell_meter");
    void (*bag_remove)(uint8_t, uint8_t) = (void (*)(uint8_t, uint8_t))fn("game_bag_remove");
    uint32_t (*digits_x)(uint32_t, uint32_t) = (uint32_t (*)(uint32_t, uint32_t))fn("game_digits_x");
    uint32_t (*divide)(uint32_t, uint32_t, uint32_t *) = (uint32_t (*)(uint32_t, uint32_t, uint32_t *))fn("game_divide");
    uint32_t (*mirror)(uint32_t) = (uint32_t (*)(uint32_t))fn("game_mirror_direction");
    void (*timer_tick)(uint32_t) = (void (*)(uint32_t))fn("game_timer_tick");
    uint32_t (*marker_base)(uint32_t, SpriteInfo, uint8_t) = (uint32_t (*)(uint32_t, SpriteInfo, uint8_t))fn("marker_base");
    uint8_t (*next_arg)(uint32_t *, uint8_t) = (uint8_t (*)(uint32_t *, uint8_t))fn("next_arg");
    uint8_t (*repeat)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *) =
        (uint8_t (*)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *))fn("repeat_step");
    uint8_t (*free_cell)(uint8_t, uint8_t, uint8_t) = (uint8_t (*)(uint8_t, uint8_t, uint8_t))fn("board_free_cell");
    uint8_t (*takes)(uint8_t, uint8_t) = (uint8_t (*)(uint8_t, uint8_t))fn("board_takes");
    uint8_t (*read_buttons)(uint8_t) = (uint8_t (*)(uint8_t))fn("game_read_buttons");
    static const uint8_t meter[][3] = {{2, 5, 5}, {0, 5, 5}, {0, 9, 9}, {0, 3, 5}};
    static const uint8_t walk[][6] = {{1, 3, 3, 0, 5, 3}, {1, 2, 3, 0, 5, 3}, {0, 0, 0x20, 0, 5, 3}, {0, 0, 0x20, 0, 5, 5}};
    static const uint8_t steer[][5] = {{0, 0x28, 0x10, 0x80, 0x70}, {0, 0x28, 0x60, 0x80, 0x10},
                                       {2, 0x54, 0x50, 0x30, 0x30}, {2, 0x54, 0x30, 0x30, 0x50}};
    static const uint8_t turn[][3] = {{0x40, 0, 0}, {0, 0, 0x40}};
    static const uint8_t take[][2] = {{10, 2}, {5, 2}, {3, 2}};
    SpriteInfo s = {0, 0, 8, 8, 5, 0};
    uint32_t set = 4u, rem;
    uint8_t press = 0;
    for (int i = 0; i < 4; i++) {
        base("cell_meter", -1, 0, 0);
        set_step_result(meter[i][0]);
        set_meter_max(meter[i][1]);
        set_meter_shown(meter[i][2]);
        cell_meter(0);
    }
    base("game_bag_add", -1, 0, 0);
    set_bag_count(BAG_SIZE - 1u);
    ((void (*)(uint8_t))fn("game_bag_add"))(3);
    base("game_mode_frame", -1, 0, 0);
    fill_bag(5);
    set_bag_cursor(3);
    bag_remove(0, 1);
    bag_remove(3, 1);
    base("game_digits_x", -1, 0, 0);
    digits_x(0x10, 12345);
    digits_x(0x10, 1234);
    digits_x(0x10, 123);
    base("game_divide", -1, 0, 0);
    divide(0x80000000u, 0x8001u, &rem);
    base("game_mirror_direction", -1, 0, 0);
    mirror(0x10);
    mirror(0);
    base("game_timer_tick", -1, 0, 0);
    timer_tick(0x2000u);
    timer_tick(1u);
    base("marker_base", -1, 0, 0);
    marker_base(5, s, 0xFFu);
    base("next_arg", -1, 0, 0);
    next_arg(&set, 0);
    base("pair_end", -1, 0, 0);
    set_scene_id(13);
    CALL0("pair_end");
    base("repeat_step", -1, 0, 0);
    repeat(5, BTN_UP, BTN_UP, 0, &press);
    repeat(1, BTN_UP, BTN_UP, 0, &press);
    for (int i = 0; i < 4; i++) {
        base("walk_next", -1, 0, 0);
        set_walk_back(walk[i][0]);
        set_walk_behind(walk[i][1]);
        set_steps(walk[i][2]);
        set_mode_done(walk[i][3]);
        set_meter_max(walk[i][4]);
        set_meter_shown(walk[i][5]);
        CALL0R(uint8_t, "walk_next");
    }
    base("board_free_cell", -1, 0, 0);
    for (uint8_t i = 0; i < 32u; i++) {
        set_cell_arg(i, 0);
        set_cell_type(i, 0);
    }
    set_cell_arg(0, 0xFFu);
    set_cell_type(0, 5);
    set_cell_arg(1, 0xFFu);
    set_cell_type(1, 0xFFu);
    free_cell(1, 1, 1);
    for (int i = 0; i < 3; i++) {
        base("board_takes", -1, 0, 0);
        takes(take[i][0], take[i][1]);
    }
    base("game_board_enter", -1, 0, 0);
    set_meter(0);
    CALL0("game_board_enter");
    for (uint8_t b = 0; b < 2; b++) {
        base("game_menu_pick_b", -1, 0, 0);
        set_sel_b(1);
        set_btn_new(b ? BTN_DOWN : BTN_UP);
        CALL0R(uint8_t, "game_menu_pick_b");
    }
    for (uint8_t d = 0; d < 2; d++) {
        base("game_meter_update", -1, 0, 0);
        set_meter(0);
        set_meter_delay(d ? 40u : 3u);
        CALL0("game_meter_update");
    }
    base("game_mode13_leave", -1, 0, 0);
    set_sel_a(0);
    CALL0("game_mode13_leave");
    base("game_read_buttons", -1, 0, 0);
    set_btn_latch(BTN_FIRE);
    set_hold_count(1);
    read_buttons(1);
    for (int i = 0; i < 4; i++) {
        base("game_reels_steer", -1, 0, 0);
        set_reel_phase(steer[i][0]);
        set_mode_wait(steer[i][1]);
        set_reel_speed_a(steer[i][2]);
        set_reel_speed_b(steer[i][3]);
        set_reel_speed_c(steer[i][4]);
        CALL0("game_reels_steer");
    }
    for (int i = 0; i < 2; i++) {
        base("game_reels_turn", -1, 0, 0);
        set_reel_phase(3);
        set_reel_pos_a(0);
        set_reel_pos_b(0);
        set_reel_pos_c(0);
        set_reel_speed_a(turn[i][0]);
        set_reel_speed_b(turn[i][1]);
        set_reel_speed_c(turn[i][2]);
        CALL0("game_reels_turn");
    }
}

static void choice_hits(uint8_t cell, uint8_t kinds_set, int want) {
    uint8_t (*choice)(uint8_t, uint8_t, uint8_t, uint8_t *) = (uint8_t (*)(uint8_t, uint8_t, uint8_t, uint8_t *))fn("board_choice");
    int hits = 0, misses = 0;
    for (uint16_t seed = 0; seed < 4000u && (hits < want || misses < 1); seed++) {
        uint8_t choices = 0;
        base("board_choice", -1, 0, 0);
        set_board_arg_kinds(0, kinds_set);
        set_seed(seed);
        rec_pause(1);
        choice(cell, 7, 0xFFu, &choices);
        rec_pause(0);
        if (choices ? hits >= want : misses >= 1) continue;
        hits += choices;
        misses += !choices;
        choices = 0;
        base("board_choice", -1, 0, 0);
        set_board_arg_kinds(0, kinds_set);
        set_seed(seed);
        choice(cell, 7, 0xFFu, &choices);
    }
    if (hits < want || misses < 1) fail("no seed makes the choice for", "board_choice");
}

static void case_level2(void) {
    uint32_t (*add_capped)(uint32_t, uint16_t, uint32_t, uint32_t) =
        (uint32_t (*)(uint32_t, uint16_t, uint32_t, uint32_t))fn("game_add_capped");
    uint16_t (*strip_load)(uint16_t, uint16_t, uint8_t) = (uint16_t (*)(uint16_t, uint16_t, uint8_t))fn("game_strip_load");
    uint8_t (*pick_three)(uint8_t, uint16_t, uint8_t *) = (uint8_t (*)(uint8_t, uint16_t, uint8_t *))fn("game_strip_pick_three");
    uint8_t (*choice)(uint8_t, uint8_t, uint8_t, uint8_t *) = (uint8_t (*)(uint8_t, uint8_t, uint8_t, uint8_t *))fn("board_choice");
    static const uint8_t cells[][4] = {{1, 0, 0, 0}, {5, 0, 0, 0}, {0x10, 1, 0, 0}, {0x10, 0, 0, 40}};
    uint8_t drawn[3], choices;
    base("game_add_capped", -1, 0, 0);
    add_capped(ram_strip_c_len(), RAM_STRIP_A, 5u, RAM_STRIP_C);
    base("game_add_capped", -1, 0, 0);
    set_bag_kind_count(2);
    set_scratch_count(2);
    strip_load((uint16_t)(ram_scratch_count() + 8u), RAM_STRIP_B, (uint8_t)(ram_bag_kind_count() >> 1));
    for (int i = 0; i < 4; i++) {
        base("game_cell_event", -1, 0, 0);
        set_step_result(1);
        set_cell_type(ram_sel_c(), cells[i][0]);
        set_bonus_done(cells[i][1]);
        set_odds_set(cells[i][2]);
        set_steps(cells[i][3]);
        CALL0R(uint8_t, "game_cell_event");
    }
    base("game_strip_pick_three", -1, 0, 0);
    vmu_w8(RAM_STRIP_A + 3u, 0x40u);
    pick_three(ram_reel_len(), RAM_STRIP_A, drawn);
    base("board_choice", -1, 0, 0);
    set_board_arg_kinds(0, 0x0Fu);
    choices = 5;
    choice(10, 7, 0xFFu, &choices);
    choices = 0;
    choice(10, 7, 0xF0u, &choices);
    choice_hits(10, 0xAAu, 4);
}

static uint8_t item_kind(uint8_t id) {
    return vmu_rom8((uint16_t)(0x8094u + 2u * id));
}

static void case_level3(void) {
    static const uint8_t fills[][4] = {{0, 0, 20, 1}, {1, 3, 3, 2}, {0, 2, 22, 0}};
    uint8_t a = 0, b = 1;
    while (item_kind(b) == item_kind(a)) b++;
    base("game_bag_add", -1, 0, 0);
    set_bag_count(2);
    set_bag_items(0, a);
    set_bag_items(1, b);
    set_scratch_first(item_kind(a));
    CALL0R(uint8_t, "game_bag_count_first");
    base("game_bag_add", -1, 0, 0);
    set_bag_count(2);
    set_bag_items(0, b);
    set_bag_items(1, a);
    set_scratch_first(item_kind(a));
    set_scratch_count(0);
    CALL0R(uint8_t, "game_bag_find_nth");
    for (int i = 0; i < 3; i++) {
        base("game_board_fill", -1, 0, 0);
        set_sel_a(fills[i][0]);
        set_board_steps(fills[i][1]);
        set_board_args(fills[i][2]);
        set_odds_set(fills[i][3]);
        set_board_arg_set(0x03FFFFFFu);
        CALL0R(uint8_t, "game_board_fill");
    }
    base("game_sound_fiq", -1, 0, 0);
    set_snd_next(1);
    CALL0("game_sound_fiq");
    base("game_sound_start", -1, 0, 0);
    set_snd_mode(2);
    ((void (*)(uint8_t, uint16_t))fn("game_sound_start"))(1, VMU_SOUND_SLOW);
}

static uint16_t seed_for(uint8_t want, uint16_t limit, int cmp) {
    uint16_t (*random)(uint16_t) = (uint16_t (*)(uint16_t))fn("game_random");
    uint8_t (*by_set)(void) = (uint8_t (*)(void))fn("game_pick_by_set");
    uint16_t saved = ram_seed();
    rec_pause(1);
    for (uint32_t s = 0; s < 0x10000u; s++) {
        set_seed((uint16_t)s);
        int hit = limit ? (cmp < 0 ? random(limit) < want : random(limit) >= want) : by_set() == want;
        if (hit) {
            set_seed(saved);
            rec_pause(0);
            return (uint16_t)s;
        }
    }
    fail("no seed for", "case_level4");
    return 0;
}

static void case_level4(void) {
    void (*handler)(uint8_t, uint16_t) = (void (*)(uint8_t, uint16_t))fn("game_call_handler");
    uint16_t (*pick_item)(uint8_t) = (uint16_t (*)(uint8_t))fn("game_pick_item");
    SpriteInfo (*marker_frame)(uint8_t) = (SpriteInfo (*)(uint8_t))fn("game_marker_frame");
    int16_t (*load_pair)(uint8_t *) = (int16_t (*)(uint8_t *))fn("game_reels_load_pair");
    void (*sound_begin)(uint8_t) = (void (*)(uint8_t))fn("game_sound_begin");
    SpriteInfo (*sprite_info)(int16_t, int16_t, uint16_t) = (SpriteInfo (*)(int16_t, int16_t, uint16_t))fn("game_sprite_info");
    void (*sprite_draw)(int16_t, int16_t, uint8_t, uint8_t, uint32_t, uint8_t) =
        (void (*)(int16_t, int16_t, uint8_t, uint8_t, uint32_t, uint8_t))fn("game_sprite_draw");
    uint8_t (*nibble)(uint8_t) = (uint8_t (*)(uint8_t))fn("game_nibble");
    uint8_t (*facing)(void) = (uint8_t (*)(void))fn("walk_facing");
    static const uint8_t scenes[][6] = {{1, 0, 0, BTN_FIRE, 0, 0}, {1, 1, 0x10, 0, 5, 0}, {2, 0, 0, 0, 0, 0}, {2, 0, 0, BTN_FIRE, 0, 0},
                                        {2, 1, 0, 0, 2, 3}, {2, 1, 0, 0, 1, 9}, {2, 1, 0, 0, 1, 3}, {2, 1, 0, BTN_FIRE, 0, 0}, {2, 1, 0, 0, 0, 0}};
    uint8_t last = 0, cells[2] = {0xFF, 0xFF};
    SpriteInfo s;
    base("game_sound_fiq", -1, 0, 0);
    set_snd_next(1);
    handler(HANDLER_SOUND, 13);
    base("game_call_handler", -1, 0, 0);
    handler(0, 0);
    base("game_call_handler", -1, 0, 0);
    handler(HANDLER_DOCK, 0);
    setup();
    base("game_marker_frame", -1, 0, 0);
    set_marker(0, 0x58u);
    set_marker(1, 2);
    set_marker(2, 3);
    set_marker(3, 0);
    marker_frame(0);
    base("game_mode3_leave", -1, 0, 0);
    set_sel_b(0);
    set_odds_set(0);
    set_steps(32);
    CALL0("game_mode3_leave");
    base("game_pick_item", -1, 0, 0);
    set_odds_set(3);
    set_seed(seed_for(3, 0, 0));
    pick_item(2);
    for (uint8_t k = 0; k < 5; k++) {
        base("game_pick_item", -1, 0, 0);
        set_scene_id(13);
        set_sel_a((uint8_t)(k >> 1 & 1u));
        set_sel_b((uint8_t)(k & 1u));
        set_seed(k < 4 ? seed_for(80, 100, -1) : seed_for(80, 100, 1));
        pick_item(1);
    }
    base("game_reels_load_pair", -1, 0, 0);
    set_bag_kind_count(2);
    load_pair(&last);
    for (unsigned i = 0; i < sizeof scenes / sizeof scenes[0]; i++) {
        base("game_scene_step", -1, 0, 0);
        set_mode_wait(0);
        set_scene_id(scenes[i][0]);
        set_mode_done(scenes[i][1]);
        set_scratch_wait(0);
        set_scratch_first(scenes[i][2]);
        set_btn_new(scenes[i][3]);
        set_scratch_count(scenes[i][4]);
        set_lap_mark(scenes[i][5]);
        CALL0("game_scene_step");
    }
    base("game_sound_begin", -1, 0, 0);
    set_docked(1);
    sound_begin(1);
    set_docked(0);
    vmu_host_set_sys(VMU_SYS_MUTE);
    sound_begin(1);
    vmu_host_set_sys(0);
    base("game_sprite_draw", -1, 0, 0);
    rec_pause(1);
    s = sprite_info(8, 4, 0);
    rec_pause(0);
    sprite_draw(s.x, s.y, s.width, s.height, s.bitmap, (uint8_t)(s.flags | SPRITE_MIRROR));
    base("walk_facing", -1, 0, 0);
    rec_pause(1);
    for (uint8_t c = 0; c < 32; c++) {
        uint8_t open = nibble(c) == 0xFu;
        if (cells[open] == 0xFFu) cells[open] = c;
    }
    rec_pause(0);
    for (int open = 0; open < 2; open++) {
        base("walk_facing", -1, 0, 0);
        set_walk_left(1);
        set_walk_back(1);
        set_sel_b(0);
        set_sel_c(cells[open]);
        facing();
    }
}

static void bag_step(const uint8_t *s) {
    base("game_mode_frame", -1, 0, 0);
    fill_bag(s[0]);
    set_mode(14);
    CALL0("game_mode14_leave");
    set_mode_done(s[1]);
    set_scene_id(s[2]);
    press(s[3]);
    set_bag_full(s[4]);
    set_scratch_lo(s[5]);
    set_scratch_count(s[6]);
    set_scratch_first(s[7]);
    set_scratch_kinds(s[8]);
    set_bag_kind_count(s[9]);
    set_bag_nth_index(0);
    CALL0R(uint32_t, "game_bag_menu_step");
}

static uint8_t walk_cell(uint8_t mask, int want_all) {
    uint8_t (*nibble)(uint8_t) = (uint8_t (*)(uint8_t))fn("game_nibble");
    rec_pause(1);
    for (uint8_t c = 0; c < 32; c++) {
        uint8_t e = nibble(c);
        if (e != 0xFu && (want_all ? (e & mask) == mask : (e & mask) == 0u)) {
            rec_pause(0);
            return c;
        }
    }
    fail("no cell for", "case_level5");
    return 0;
}

static void case_level5(void) {
    static const uint8_t steps[][10] = {
        {3, 0, 0, BTN_UP, 0, 1, 0, 0, 1, 1}, {3, 0, 0, BTN_LEFT, 1, 1, 0, 0, 5, 2}, {3, 0, 0, BTN_RIGHT, 1, 1, 0, 2, 3, 2},
        {3, 0, 0, 0, 1, 1, 0, 0, 1, 2}, {3, 2, 0, BTN_FIRE, 1, 2, 0, 0, 1, 1}, {1, 2, 0, BTN_FIRE, 0, 1, 0, 0, 1, 1},
        {3, 2, 0, BTN_FIRE, 0, 1, 0, 0, 1, 1}, {4, 2, 0, BTN_FIRE, 0, 3, 2, 0, 1, 1}, {3, 1, 0, BTN_LEFT, 0, 3, 0, 0, 1, 1},
        {3, 1, 0, BTN_RIGHT, 0, 3, 2, 0, 1, 1}};
    static const uint8_t reveals[][3] = {{0, 3, 1}, {1, 3, 1}, {2, 1, 1}, {4, 1, 1}, {4, 3, 2}};
    static const uint8_t pays[][3] = {{3, 40, 0}, {4, 70, 0}, {1, 0, 1}};
    static const uint8_t backs[] = {0, 1, 2};
    void (*dispatch)(uint16_t) = (void (*)(uint16_t))fn("game_irq_dispatch");
    uint8_t (*byte_is)(uint8_t, uint32_t) = (uint8_t (*)(uint8_t, uint32_t))fn("game_byte_is");
    uint8_t pet = 0, scene = 0;
    for (unsigned i = 0; i < sizeof steps / sizeof steps[0]; i++) bag_step(steps[i]);
    for (unsigned i = 0; i < sizeof reveals / sizeof reveals[0]; i++) {
        base("game_bag_reveal", -1, 0, 0);
        set_meter_shown(ram_meter());
        set_mode_done(reveals[i][0]);
        set_scene_id(reveals[i][1]);
        set_mode_wait(reveals[i][2]);
        CALL0("game_bag_reveal");
    }
    base("game_bag_reveal", -1, 0, 0);
    set_meter(3);
    set_meter_shown(2);
    CALL0("game_bag_reveal");
    base("game_irq_dispatch", -1, 0, 0);
    set_irq_handler(5, 0);
    dispatch(1u << 5);
    base("game_menu_pick_two", -1, 0, 0);
    fill_bag(3);
    set_mode_done(1);
    set_btn_new(BTN_DOWN | BTN_FIRE);
    CALL0("game_menu_pick_two");
    base("game_mode11_leave", -1, 0, 0);
    set_scene_id(13);
    CALL0("game_mode11_leave");
    rec_pause(1);
    for (pet = 0; pet < 8; pet++) {
        for (scene = 0; scene < 13 && !byte_is(pet, scene); scene++) {
        }
        if (scene < 13) break;
    }
    rec_pause(0);
    if (pet == 8) fail("no pet match for", "case_level5");
    base("game_mode11_leave", -1, 0, 0);
    set_pet_kind(pet);
    set_scene_id(scene);
    CALL0("game_mode11_leave");
    rec_pause(1);
    for (pet = 0; pet < 8; pet++) {
        for (scene = 0; scene < 13 && !(byte_is(scene, pet) && !byte_is(pet, scene)); scene++) {
        }
        if (scene < 13) break;
    }
    rec_pause(0);
    if (pet == 8) fail("no reverse pet match for", "case_level5");
    base("game_mode11_leave", -1, 0, 0);
    set_pet_kind(pet);
    set_scene_id(scene);
    CALL0("game_mode11_leave");
    for (int i = 0; i < 4; i++) {
        base("game_mode9_leave", -1, 0, 0);
        set_scene_id(i < 2 ? 0 : (uint8_t)(i - 1));
        set_meter_max(i == 0 ? 5 : 9);
        set_meter_shown(i == 0 ? 3 : 9);
        if (i == 2) fill_bag(3);
        if (i == 3 && ram_strip_c_len() == 0) set_strip_c_len(4);
        CALL0("game_mode9_leave");
    }
    for (int i = 0; i < 3; i++) {
        base("game_payout", -1, 0, 0);
        set_scratch_count(pays[i][0]);
        set_steps(pays[i][1]);
        if (pays[i][2]) set_points(99990u);
        CALL0R(uint32_t, "game_payout");
    }
    base("game_reel_pair_spin_step", -1, 0, 0);
    set_reel_phase(6);
    set_reel_speed_a(0);
    set_reel_speed_b(0x40);
    CALL0("game_reel_pair_spin_step");
    base("game_reel_pair_spin_step", -1, 0, 0);
    set_reel_phase(4);
    set_btn_new(0);
    vmu_w16(RAM_MODE_WAIT, 1);
    CALL0("game_reel_pair_spin_step");
    base("game_reel_race_step", -1, 0, 0);
    set_mode_done(5);
    set_btn_new(BTN_FIRE);
    CALL0("game_reel_race_step");
    base("game_reel_spin_step", -1, 0, 0);
    set_reel_phase(6);
    set_btn_new(0);
    set_mode_wait(1);
    CALL0("game_reel_spin_step");
    base("game_reel_spin_step", -1, 0, 0);
    set_reel_phase(2);
    set_btn_new(0);
    vmu_w16(RAM_MODE_WAIT, 1);
    CALL0("game_reel_spin_step");
    base("game_walk_choose", -1, 0, 0);
    set_walk_busy(0);
    set_sel_c(walk_cell(0x5u, 0));
    set_btn_prev(BTN_LEFT);
    CALL0("game_walk_choose");
    for (int i = 0; i < 3; i++) {
        base("game_walk_choose", -1, 0, 0);
        set_walk_busy(0);
        set_sel_c(walk_cell(0x5u, 1));
        set_sel_b(backs[i]);
        set_btn_prev(BTN_LEFT);
        CALL0("game_walk_choose");
    }
    base("game_walk_pending", -1, 0, 0);
    set_meter(3);
    set_meter_shown(2);
    CALL0("game_walk_pending");
    base("game_walk_pending", -1, 0, 0);
    set_meter_shown(ram_meter());
    set_walk_busy(0);
    set_walk_left(0);
    set_step_result(0);
    CALL0("game_walk_pending");
    base("game_walk_pending", -1, 0, 0);
    set_meter_shown(ram_meter());
    set_walk_busy(0);
    set_walk_left(2);
    set_walk_back(1);
    set_walk_behind(ram_steps());
    CALL0("game_walk_pending");
}

static void case_level6(void) {
    static const uint8_t icons[][2] = {{1, 4}, {1, 3}};
    static const uint8_t pairs[][5] = {{3, 0, 0, BTN_FIRE, 0}, {6, 13, 0, BTN_FIRE, 0}, {8, 5, 1, BTN_FIRE, 0},
                                       {8, 13, 3, BTN_FIRE, 0}, {8, 5, 3, BTN_FIRE, 0}, {10, 5, 0, 0, 3}, {11, 5, 0, 0, 0}};
    void (*arrows)(uint8_t) = (void (*)(uint8_t))fn("bag_arrows_draw");
    SpriteInfo (*icon)(int16_t, int16_t, uint8_t, uint8_t) = (SpriteInfo (*)(int16_t, int16_t, uint8_t, uint8_t))fn("game_icon_draw");
    uint32_t (*window)(int16_t, uint16_t, uint8_t, uint16_t) = (uint32_t (*)(int16_t, uint16_t, uint8_t, uint16_t))fn("game_reel_window_draw");
    uint8_t (*next_symbol)(uint8_t, uint16_t, uint16_t) = (uint8_t (*)(uint8_t, uint16_t, uint16_t))fn("game_strip_next_symbol");
    uint8_t symbol;
    base("game_mode_frame", -1, 0, 0);
    set_mode_phase(1);
    arrows(2);
    for (int i = 0; i < 2; i++) {
        for (uint8_t c = 0; c < 2; c++) {
            base("game_icon_draw", -1, 0, 0);
            set_cycle16(c ? 8u : 0u);
            icon(4, 4, icons[i][0], icons[i][1]);
        }
    }
    base("game_reel_window_draw", -1, 0, 0);
    window(0x16, ram_reel_pos_b(), ram_reel_len_b() ? ram_reel_len_b() : 4u, RAM_STRIP_B);
    for (int i = 0; i < 4; i++) {
        base("game_reel_game_step", -1, 0, 0);
        set_mode_done(2);
        set_reel_phase(8);
        set_mode_wait(0);
        if (ram_reel_len() == 0) set_reel_len(4);
        rec_pause(1);
        symbol = next_symbol(ram_reel_len(), ram_reel_pos_a(), RAM_STRIP_A);
        rec_pause(0);
        set_scratch_kinds((uint8_t)(symbol + 1u));
        set_scratch_first(i == 0 ? symbol : (uint8_t)(symbol + 1u));
        set_scratch_count(symbol);
        if (i == 2) set_seed(seed_for(20, 100, -1));
        if (i == 3) set_seed(seed_for(70, 100, 1));
        CALL0("game_reel_game_step");
    }
    for (uint8_t state = 3; state < 7; state += 3) {
        base("game_reel_game_step", -1, 0, 0);
        set_mode_done(state);
        set_mode_wait(0);
        set_btn_new(BTN_FIRE);
        CALL0("game_reel_game_step");
    }
    for (unsigned i = 0; i < sizeof pairs / sizeof pairs[0]; i++) {
        base("game_reel_pair_step", -1, 0, 0);
        set_mode_done(pairs[i][0]);
        set_scene_id(pairs[i][1]);
        set_meter_shown(pairs[i][2] ? pairs[i][2] : ram_meter_shown());
        set_btn_new(pairs[i][3]);
        set_mode_wait(pairs[i][4]);
        CALL0("game_reel_pair_step");
    }
    for (int i = 0; i < 3; i++) {
        base("game_sound_play_waiting", -1, 0, 0);
        set_sound_req(1);
        set_docked(i == 0);
        vmu_host_set_sys(i == 1 ? VMU_SYS_MUTE : 0u);
        if (i == 2) vmu_host_set_dock(1);
        CALL0("game_sound_play_waiting");
        vmu_host_set_sys(0);
        vmu_host_set_dock(0);
        setup();
    }
}

static void case_level7(void) {
    SpriteInfo (*count_of)(int16_t, int16_t, uint32_t, uint8_t) =
        (SpriteInfo (*)(int16_t, int16_t, uint32_t, uint8_t))fn("game_count_of_draw");
    for (int i = 0; i < 3; i++) {
        base("game_board_draw", -1, 0, 0);
        set_sel_e(0);
        set_cell_saved(ram_sel_c());
        if (i < 2) {
            set_mode(11);
            set_mode_done(i ? 10 : 9);
            set_mode_phase(2);
            set_scratch_lo(0xFE);
            set_scratch_hi(3);
            for (uint8_t k = 0; k < 4; k++) set_anim((uint8_t)(4u + k), ram_anim(k));
        } else {
            set_cell_type(ram_sel_c(), 0x10);
        }
        CALL0R(uint32_t, "game_board_draw");
    }
    base("game_count_of_draw", -1, 0, 0);
    count_of(6, 0x1A, ram_bag_count(), 7);
    for (uint8_t kinds = 1; kinds < 3; kinds++) {
        base("game_reel_anim_screen_draw", -1, 0, 0);
        set_bag_kind_count(kinds);
        set_reel_phase(kinds == 1 ? 0 : 2);
        CALL0R(uint32_t, "game_reel_anim_screen_draw");
    }
}

static void case_level8(void) {
    static const uint8_t bags[][3] = {{1, 8, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 4}, {3, 0, 0}};
    static const uint8_t scenes[][3] = {{2, 0, 0}, {2, 0, 5}, {3, 5, 5}, {3, 5, 20}};
    for (unsigned i = 0; i < sizeof bags / sizeof bags[0]; i++) {
        base("game_bag_screen_draw", -1, 0, 0);
        fill_bag(3);
        set_bag_cursor(1);
        set_mode_done(bags[i][0]);
        set_scene_id(bags[i][1]);
        set_lap_points(123);
        set_scratch_first(0);
        set_scratch_count(0);
        set_reel_len(bags[i][2]);
        CALL0("game_bag_screen_draw");
    }
    for (int nth = 0; nth < 4; nth++) {
        base("game_board_choice_draw", -1, 0, nth);
        set_anim(1, 3);
        CALL0("game_board_choice_draw");
    }
    base("game_reel_pair_screen", -1, 0, 0);
    set_mode_done(10);
    CALL0("game_reel_pair_screen");
    base("game_reel_pair_screen", -1, 0, 0);
    set_mode_done(8);
    set_mode_wait(0);
    set_scene_id(0);
    set_btn_new(BTN_FIRE);
    CALL0("game_reel_pair_screen");
    base("game_reel_pair_screen", -1, 0, 0);
    set_mode(14);
    set_mode_done(2);
    CALL0("game_reel_pair_screen");
    for (unsigned i = 0; i < sizeof scenes / sizeof scenes[0]; i++) {
        base("game_scene_screen_draw", RAM_MODE_DONE, 1, 0);
        set_scene_id(scenes[i][0]);
        set_scratch_count(scenes[i][2]);
        set_scratch_wait(scenes[i][1]);
        if (ram_reel_len() == 0) set_reel_len(4);
        CALL0("game_scene_screen_draw");
    }
    bag_menu(3, 0, 0);
    set_mode_done(2);
    set_mode_phase(1);
    CALL0("game_bag_menu_screen");
    bag_menu(3, 0, 0);
    set_mode_done(3);
    set_scene_id(1);
    set_scratch_wait(5);
    CALL0("game_bag_menu_screen");
}

static void case_level9(void) {
    base("game_mode3_frame", -1, 0, 0);
    set_mode(5);
    CALL0("game_mode3_frame");
    base("game_mode3_frame", -1, 0, 0);
    set_btn_new(BTN_FIRE);
    set_meter_shown(ram_meter());
    set_cell_saved((uint8_t)(ram_sel_c() ^ 1u));
    CALL0("game_mode3_frame");
    base("game_mode3_frame", -1, 0, 0);
    set_btn_new(BTN_FIRE);
    set_meter_shown(ram_meter());
    set_cell_saved(ram_sel_c());
    set_walk_busy(1);
    set_walk_frame(2);
    CALL0("game_mode3_frame");
}

static void case_level10(void) {
    base("game_mode8_frame", -1, 0, 0);
    set_mode(5);
    CALL0("game_mode8_frame");
    base("game_mode8_frame", -1, 0, 0);
    set_mode_done(0);
    set_walk_frame(0x40);
    set_scene_id(3);
    set_meter_shown(ram_meter());
    CALL0("game_mode8_frame");
    base("game_mode8_frame", -1, 0, 0);
    set_mode_done(1);
    set_scene_id(3);
    set_meter_shown(ram_meter());
    CALL0("game_mode8_frame");
}

int main(int argc, char **argv) {
    const char *card = 0, *entry = 0;
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "--card")) card = argv[i + 1];
        else if (!strcmp(argv[i], "--entry")) entry = argv[i + 1];
        else if (!strcmp(argv[i], "--dir")) g_dir = argv[i + 1];
        else fail("unknown option", argv[i]);
    }
    if (!card || !entry || !g_dir) fail("usage:", "record_synthetic --card C --entry E --dir DIR");
    load_file(card);
    load_entry(entry);
    vmu_host_init(g_file, g_file_len, g_file, g_file_len);
    vmu_host_set_entry(g_words, g_nwords);
    vmu_host_set_sys(0);
    vmu_host_set_frame(frame_cb);
    rec_begin(g_dir, "synthetic", 1);
    setup();
    static void (*const cases[])(void) = {case_bag, case_modes, case_board, case_reels, case_reveal, case_quit, case_levels01, case_level2, case_level3, case_level4, case_level5, case_level6, case_level7, case_level8, case_level9, case_level10};
    static const char *const names[] = {"bag", "modes", "board", "reels", "reveal", "quit", "levels01", "level2", "level3", "level4", "level5", "level6", "level7", "level8", "level9", "level10"};
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        fprintf(stderr, "record_synthetic: case %s\n", names[i]);
        cases[i]();
    }
    rec_finish();
    printf("record_synthetic: frames %u\n", g_frame);
    return 0;
}
