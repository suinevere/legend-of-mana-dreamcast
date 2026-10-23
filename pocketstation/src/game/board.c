/*----------------------
 | board.c
 | Description: Ring Ring Land's board: laying it out, entering it, walking round the 32-cell ring that sel_c and sel_d track by button or on its own, and the meter catching up on show. It also picks what a finished step leads to, lists the cells on screen and draws them.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: sel_a, sel_c, sel_d, sel_b, sel_e, walk_busy, walk_frame, walk_kind, cell_saved, facing_saved, meter, meter_shown, meter_delay, meter_max, scene_id, mode, mode_done, mode_phase, mode_wait, scratch_kinds, btn_prev, step_result, odds_set, steps, walk_left, walk_back, walk_behind, anim, cell_arg, cell_type, bonus_done, lap_mark, seed, board_steps, board_args, board_arg_set, board_arg_kinds, scratch_lo, scratch_hi, board_list
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | game_walk_dirs
 | Description: Held button and the pair of cell exit bits it walks along, in the order they are tried.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
const WalkDir game_walk_dirs[4] = {{BTN_LEFT, 0x5u}, {BTN_RIGHT, 0xAu}, {BTN_UP, 0x3u}, {BTN_DOWN, 0xCu}};

/*----------------------
 | WALK_STEP_FRAMES
 | Description: Frames a step forward or back takes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define WALK_STEP_FRAMES 16u

/*----------------------
 | game_board_enter
 | Description: Starts the board: saves first when meter is nonzero, then sets the opening count running and records where the walk starts. The no-save path is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: meter, mode_phase, mode_wait, walk_busy, walk_frame, walk_kind, cell_saved, facing_saved, sel_c, sel_d, sel_e, scene_id, mode_done, scratch_kinds
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_board_enter(void) {
    if (ram_meter() != 0u) {
        game_save();
    }
    set_mode_phase(0);
    set_mode_wait(5);
    set_walk_busy(1);
    set_walk_frame(0);
    set_walk_kind(2);
    set_cell_saved(ram_sel_c());
    set_facing_saved(ram_sel_d());
    set_sel_e(0);
    set_scene_id(0x5Eu);
    set_mode_done(0);
    set_scratch_kinds(0);
}

/*----------------------
 | game_meter_update
 | Description: Counts meter_delay up to 30 (40 when meter is 0); once there, meter_shown takes meter, and 0 starts mode 12 while a rise requests sound 1. The mode 12 and silent-drop paths are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: meter, meter_shown, meter_delay, mode, sound_req
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_meter_update(void) {
    uint8_t target = ram_meter(), delay = target == 0u ? 40u : 30u, shown;
    if (ram_meter_delay() != delay) {
        set_meter_delay((uint8_t)(ram_meter_delay() + 1u));
        return;
    }
    shown = ram_meter_shown();
    set_meter_shown(target);
    if (target == 0u) {
        set_mode(12);
    } else if (target >= shown) {
        game_sound_request(1);
    }
}

/*----------------------
 | game_walk_step
 | Description: Advances the running step: the opening count stops at 12, a count down ends the step at 0, and a forward or back step moves sel_c after 16 frames and turns sel_d by the exits of the step's forward-end cell. The opening count's stop at 12 and a step onto a 0xF cell are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: walk_busy, walk_frame, walk_kind, sel_c, sel_d
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_walk_step(void) {
    uint8_t frame, kind, cell, exits;
    if (ram_walk_busy() == 0u) {
        return;
    }
    frame = ram_walk_frame();
    kind = ram_walk_kind();
    if (kind == 2u) {
        if (frame != 12u) {
            set_walk_frame((uint8_t)(frame + 1u));
        }
        return;
    }
    if (kind > 2u) {
        frame = (uint8_t)(frame - 1u);
        set_walk_frame(frame);
        if (frame == 0u) {
            set_walk_busy(0);
        }
        return;
    }
    frame = (uint8_t)(frame + 1u);
    set_walk_frame(frame);
    if (frame != WALK_STEP_FRAMES) {
        return;
    }
    set_walk_busy(0);
    set_walk_frame(0);
    cell = ram_sel_c();
    if (kind == 0u) {
        cell = (uint8_t)((cell + 1u) & 0x1Fu);
        set_sel_c(cell);
        exits = game_nibble(cell);
        if (exits != 0xFu) {
            set_sel_d((uint8_t)(exits & ~game_mirror_direction(ram_sel_d())));
        }
        return;
    }
    set_sel_c((uint8_t)((cell - 1u) & 0x1Fu));
    exits = game_nibble(cell);
    if (exits != 0xFu) {
        set_sel_d((uint8_t)game_mirror_direction(exits & ~ram_sel_d()));
    }
}

/*----------------------
 | game_cell_exits
 | Description: The exit bits of cell sel_c; a cell marked 0xF takes sel_d and its mirror instead.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: sel_c, sel_d
 | Params: N/A
 | Returns: the exit bits
 ----------------------*/
uint8_t game_cell_exits(void) {
    uint8_t exits = game_nibble(ram_sel_c());
    if (exits == 0xFu) {
        exits = (uint8_t)(game_mirror_direction(ram_sel_d()) | ram_sel_d());
    }
    return exits;
}

/*----------------------
 | game_walk_choose
 | Description: With no step running, the first held direction with an exit starts a step: back when both of its exits are open and sel_b is 1 or above 2, or when its one exit is not sel_d; forward otherwise. The both-exits case is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: walk_busy, walk_frame, walk_kind, sel_b, sel_c, sel_d, btn_prev
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_walk_choose(void) {
    uint8_t exits, held, open = 0u, back, i;
    if (ram_walk_busy() != 0u) {
        return;
    }
    exits = game_cell_exits();
    held = ram_btn_prev();
    for (i = 0u; i < 4u; i++) {
        open = (uint8_t)(exits & game_walk_dirs[i].exits);
        if ((held & game_walk_dirs[i].button) && open != 0u) {
            break;
        }
    }
    if (i == 4u) {
        return;
    }
    if (open == game_walk_dirs[i].exits) {
        back = (uint8_t)(ram_sel_b() != 0u && ram_sel_b() != 2u);
    } else {
        back = (uint8_t)((open & ram_sel_d()) == 0u);
    }
    set_walk_kind(back);
    set_walk_busy(1);
    set_walk_frame(0);
}

/*----------------------
 | cell_bonus
 | Description: Starts bonus scene 13 and marks it played. No script reaches it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: bonus_done, scene_id
 | Params: N/A
 | Returns: mode 11
 ----------------------*/
static uint8_t cell_bonus(void) {
    set_bonus_done(1);
    set_scene_id(0xDu);
    return 11u;
}

/*----------------------
 | cell_random_scene
 | Description: Picks one of scenes 0-12 at random.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: scene_id, seed
 | Params: N/A
 | Returns: mode 11
 ----------------------*/
static uint8_t cell_random_scene(void) {
    set_scene_id((uint8_t)(game_random(0xD0u) >> 4));
    return 11u;
}

/*----------------------
 | cell_meter
 | Description: The 0xFF cell's meter effects: argument 0 refills the meter to meter_max (raising a full one by one up to 9) unless step_result is 2, 1-6 add one below meter_max. The refill is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: step_result, meter, meter_shown, meter_max, meter_delay, lap_mark
 | Params: arg -- the cell's argument, 0-6
 | Returns: N/A
 ----------------------*/
static void cell_meter(uint8_t arg) {
    uint8_t shown = ram_meter_shown(), max = ram_meter_max();
    if (arg != 0u) {
        if (shown != max) {
            set_meter((uint8_t)(shown + 1u));
            set_meter_delay(0);
        }
        return;
    }
    if (ram_step_result() == 2u) {
        return;
    }
    if (shown == max && max != 9u) {
        max++;
        set_meter_max(max);
    }
    set_meter(max);
    set_meter_delay(0);
    set_lap_mark(0);
}

/*----------------------
 | game_cell_event
 | Description: Chooses the mode a finished step leads to from step_result and the cell under sel_c, setting scene_id, mode_done or the meter on the way. The 0x10 cell, bonus scene 13, the refill and mode 15 are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: sel_e, step_result, odds_set, steps, sel_c, cell_arg, cell_type, bonus_done, scene_id, mode_done, mode, meter, meter_shown, meter_max, meter_delay, lap_mark, seed
 | Params: N/A
 | Returns: the mode set: 3, 8, 9, 10, 11 or 15
 ----------------------*/
uint8_t game_cell_event(void) {
    uint8_t result = ram_step_result(), mode = 3u, cell, arg, type;
    set_sel_e(0);
    if (result == 3u) {
        mode = ram_odds_set() == 2u ? cell_bonus() : 8u;
    } else if (result != 0u) {
        cell = ram_sel_c();
        arg = ram_cell_arg(cell);
        type = ram_cell_type(cell);
        if (type >= 2u && type < 5u) {
            set_scene_id((uint8_t)(type - 2u));
            mode = 9u;
        } else if (type == 0x10u) {
            if (ram_bonus_done() == 0u && (uint16_t)(((ram_odds_set() + 1u) << 5) - ram_steps()) < 32u) {
                mode = cell_bonus();
            } else {
                mode = cell_random_scene();
            }
        } else if (type != 0xFFu) {
            set_mode_done((uint8_t)(type - 5u));
            mode = 15u;
        } else if (arg < 7u) {
            cell_meter(arg);
        } else if (arg != 0xFFu) {
            set_scene_id(arg);
            mode = 10u;
        } else {
            mode = cell_random_scene();
        }
    }
    set_mode(mode);
    return mode;
}

/*----------------------
 | BOARD_JUNCTION_A, BOARD_JUNCTION_B
 | Description: The two ring cells a walk off screen turns back from, and the cells its branches restart at.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BOARD_JUNCTION_A 0x08u
#define BOARD_JUNCTION_B 0x18u

/*----------------------
 | board_insert
 | Description: Inserts a (y, x, cell) entry into the three-byte entries of list, after the last entry whose signed y is not above y.
 | Author: suinevere
 | Dependencies: psk_hw.h
 | Globals: N/A
 | Params: list -- entries; count -- entries in it; x, y -- screen position; cell -- ring cell
 | Returns: count + 1
 ----------------------*/
static uint8_t board_insert(uint32_t list, uint8_t count, int16_t x, int16_t y, uint8_t cell) {
    int16_t i = (int16_t)((int16_t)count - 1);
    uint8_t moved = 0;
    uint32_t from = list + (uint32_t)(int32_t)(i * 3), at;
    while ((int8_t)psk_r8(list + (uint32_t)(int32_t)(i * 3)) > y) {
        moved++;
        if (i == 0) {
            i = -1;
            break;
        }
        i--;
    }
    for (; moved != 0u; moved--) {
        psk_w8(from + 3u, psk_r8(from));
        psk_w8(from + 4u, psk_r8(from + 1u));
        psk_w8(from + 5u, psk_r8(from + 2u));
        from -= 3u;
    }
    at = list + (uint32_t)(int32_t)(i * 3) + 3u;
    psk_w8(at, (uint8_t)y);
    psk_w8(at + 1u, (uint8_t)x);
    psk_w8(at + 2u, cell);
    return (uint8_t)(count + 1u);
}

/*----------------------
 | board_branch
 | Description: Restarts the walk from the junction cells along each exit set in exits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: x, y -- screen position; exits -- exit bits to follow; depth -- steps left; count -- entries in list; list -- entries
 | Returns: the new entry count
 ----------------------*/
static uint8_t board_branch(int16_t x, int16_t y, uint8_t exits, uint8_t depth, uint8_t count, uint32_t list) {
    if (exits & 4u) {
        count = game_board_gather(x, y, BOARD_JUNCTION_A, 0x82u, depth, count, list);
    }
    if (exits & 2u) {
        count = game_board_gather(x, y, BOARD_JUNCTION_A, 0x04u, depth, count, list);
    }
    if (exits & 8u) {
        count = game_board_gather(x, y, BOARD_JUNCTION_B, 0x81u, depth, count, list);
    }
    if (exits & 1u) {
        count = game_board_gather(x, y, BOARD_JUNCTION_B, 0x08u, depth, count, list);
    }
    return count;
}

/*----------------------
 | game_board_gather
 | Description: Moves one ring cell along dir and one isometric screen step, lists the cell by y when on screen, then recurses down each new exit until depth runs out. depth, count, x and y are narrowed where the ARM's are 32-bit. The empty-list insert is unreachable, since callers seed one entry; it is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: x, y -- screen position; cell -- ring cell; dir -- step bits (1, 2, 4, 8; 0x80 steps back a cell); depth -- steps left; count -- entries in list; list -- (y, x, cell) entries
 | Returns: the new entry count
 ----------------------*/
uint8_t game_board_gather(int16_t x, int16_t y, uint8_t cell, uint8_t dir, uint8_t depth, uint8_t count, uint32_t list) {
    uint8_t exits, back = (uint8_t)(dir & 0x80u);
    x = (int16_t)(x + ((dir & 5u) ? 16 : -16));
    y = (int16_t)(y + ((dir & 3u) ? 8 : -8));
    cell = (uint8_t)((cell + (back ? 0x1Fu : 1u)) & 0x1Fu);
    if (x < 32 && y < 32 && x > -32 && y > -32) {
        count = board_insert(list, count, x, y, cell);
    } else if (cell == BOARD_JUNCTION_A || cell == BOARD_JUNCTION_B) {
        return board_branch(x, y, (uint8_t)(0x0Fu & ~dir), depth, count, list);
    }
    if (depth == 0u) {
        return count;
    }
    depth--;
    exits = game_nibble(cell);
    if (exits == 0x0Fu) {
        return board_branch(x, y, (uint8_t)(0x0Fu & ~dir), depth, count, list);
    }
    exits &= (uint8_t)~dir;
    if (exits & 1u) {
        count = game_board_gather(x, y, cell, (uint8_t)(back | 8u), depth, count, list);
    }
    if (exits & 2u) {
        count = game_board_gather(x, y, cell, (uint8_t)(back | 4u), depth, count, list);
    }
    if (exits & 4u) {
        count = game_board_gather(x, y, cell, (uint8_t)(back | 2u), depth, count, list);
    }
    if (exits & 8u) {
        count = game_board_gather(x, y, cell, (uint8_t)(back | 1u), depth, count, list);
    }
    return count;
}

/*----------------------
 | game_walk_by_buttons
 | Description: One frame of walking under the player's control: the meter catches up first, else a held direction may start a step; the running step then advances.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: meter, meter_shown, meter_delay, mode, sound_req, walk_busy, walk_frame, walk_kind, sel_b, sel_c, sel_d, btn_prev
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_walk_by_buttons(void) {
    if (ram_meter_shown() != ram_meter()) {
        game_meter_update();
    } else {
        game_walk_choose();
    }
    game_walk_step();
}

/*----------------------
 | walk_next
 | Description: Takes one of walk_left's steps: backwards it backs off a cell until walk_behind reaches steps; forwards it retraces walk_behind, else counts a new cell, refilling a meter point per lap. The back path's end and the lap's meter refill are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: walk_left, walk_back, walk_behind, steps, step_result, odds_set, lap_mark, mode_done, meter, meter_shown, meter_max, meter_delay
 | Params: N/A
 | Returns: 1 when a step starts, 0 when the backing off is already at its end
 ----------------------*/
static uint8_t walk_next(void) {
    uint8_t behind = ram_walk_behind(), steps = ram_steps();
    uint16_t next;
    set_walk_left((uint8_t)(ram_walk_left() - 1u));
    if (ram_walk_back() != 0u) {
        set_step_result(0);
        if (behind == steps) {
            set_walk_left(0);
            return 0;
        }
        next = (uint16_t)(behind + 1u);
        set_walk_behind((uint8_t)next);
        if (next == steps) {
            set_walk_left(0);
        }
        return 1;
    }
    if (behind != 0u) {
        set_walk_behind((uint8_t)(behind - 1u));
        set_step_result(2);
        return 1;
    }
    if ((steps & 0x1Fu) == 0u) {
        set_lap_mark(0);
        if (ram_mode_done() == 0u && ram_meter_max() != ram_meter_shown()) {
            set_meter((uint8_t)(ram_meter_shown() + 1u));
            set_meter_delay(0);
        }
    }
    next = (uint16_t)(steps + 1u);
    set_steps((uint8_t)next);
    if (next == (uint16_t)((ram_odds_set() + 1u) << 5)) {
        set_step_result(3);
        set_walk_left(0);
    } else {
        set_step_result(1);
    }
    return 1;
}

/*----------------------
 | walk_facing
 | Description: The direction bits the walker faces once steps stop, taken from sel_d, its mirror or the cell's exits by walk_back and sel_b. A 0xF cell, and the cell's exits when walking back, are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: walk_left, walk_back, sel_b, sel_c, sel_d
 | Params: N/A
 | Returns: the direction bits
 ----------------------*/
static uint8_t walk_facing(void) {
    uint8_t facing = ram_sel_d(), exits;
    if (ram_walk_left() == 0u || ram_walk_back() == 0u) {
        if (ram_sel_b() == 0u) {
            return facing;
        }
        exits = game_nibble(ram_sel_c());
        return exits == 0xFu ? (uint8_t)game_mirror_direction(facing) : (uint8_t)(exits & ~facing);
    }
    if (ram_sel_b() != 0u) {
        return (uint8_t)game_mirror_direction(facing);
    }
    exits = game_nibble(ram_sel_c());
    return exits == 0xFu ? facing : (uint8_t)game_mirror_direction((uint8_t)(exits & ~facing));
}

/*----------------------
 | game_walk_pending
 | Description: One frame of walking walk_left steps on their own once the meter has caught up; when no step runs after, anim slot 0 turns to face the walker's direction. The meter path and facing values 2 and 3 are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: meter, meter_shown, meter_delay, meter_max, mode, sound_req, walk_busy, walk_frame, walk_kind, walk_left, walk_back, walk_behind, steps, step_result, odds_set, lap_mark, mode_done, sel_b, sel_c, sel_d, anim
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_walk_pending(void) {
    uint8_t dir;
    if (ram_meter_shown() != ram_meter()) {
        game_meter_update();
        return;
    }
    if (ram_walk_busy() == 0u && ram_walk_left() != 0u && walk_next() != 0u) {
        set_walk_kind((uint8_t)(ram_walk_back() ^ ram_sel_b()));
        set_walk_busy(1);
        set_walk_frame(0);
    }
    game_walk_step();
    if ((ram_walk_busy() == 0u || ram_walk_frame() == 0u) && ram_step_result() != 3u) {
        dir = walk_facing();
        set_anim(1, (dir & 1u) ? 1u : (dir & 2u) ? 3u : (dir & 4u) ? 0u : 2u);
    }
    set_mode_done(0);
}

/*----------------------
 | BOARD_TWIN
 | Description: Cell every board scan skips; game_board_fill copies cell 8 into it at the end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BOARD_TWIN 0x18u

/*----------------------
 | BOARD_CHOICE_SLOT
 | Description: First anim slot and cell type of the up to five choice cells; the scene cells use slots and types 2-4.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BOARD_CHOICE_SLOT 5u

/*----------------------
 | ANIM_SCENE_CELL
 | Description: Animation id that marks a scene cell.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ANIM_SCENE_CELL 0x16u

/*----------------------
 | next_arg
 | Description: Steps arg past the next set bit of set, shifting each tested bit out; with no set bit left it spins, as the original does.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: set -- remaining arg bits, shifted in place; arg -- the arg of the bit before set's bit 0
 | Returns: the arg of the set bit found
 ----------------------*/
static uint8_t next_arg(uint32_t *set, uint8_t arg) {
    uint32_t bit;
    do {
        arg++;
        bit = *set & 1u;
        *set >>= 1;
    } while (!bit);
    return arg;
}

/*----------------------
 | board_free_cell
 | Description: Rolls random(free * 16) / 16 and returns that free cell, stepping cells by step and skipping BOARD_TWIN. Free means cell_arg 0xFF, and cell_type 0xFF too when both is set.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: cell_arg, cell_type, seed
 | Params: free -- free cells to choose from; step -- 4, 2 or 1; both -- also require a free cell_type
 | Returns: the cell
 ----------------------*/
static uint8_t board_free_cell(uint8_t free, uint8_t step, uint8_t both) {
    uint8_t n = (uint8_t)(game_random((uint16_t)(free << 4)) >> 4);
    uint8_t i = 0;
    for (;;) {
        if (ram_cell_arg(i) == 0xFFu && (!both || ram_cell_type(i) == 0xFFu)) {
            if (!n) {
                return i;
            }
            n--;
        }
        i = (uint8_t)(i + step);
        if (i == BOARD_TWIN) {
            i = (uint8_t)(i + step);
        }
    }
}

/*----------------------
 | board_takes
 | Description: Whether the next arg gets a cell: always while left fits in free, else with odds 70%, 50% or 30% as left exceeds free by under 3, under 7 or more.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: seed
 | Params: left -- args still to place; free -- free even cells
 | Returns: 1 to place the arg, 0 to drop it
 ----------------------*/
static uint8_t board_takes(uint8_t left, uint8_t free) {
    uint8_t excess, odds;
    if (left <= free) {
        return 1u;
    }
    excess = (uint8_t)(left - free);
    odds = excess >= 7u ? 30u : excess >= 3u ? 50u : 70u;
    return game_random(100u) < odds;
}

/*----------------------
 | board_choice
 | Description: May make cell a choice cell: with odds n/(n+30), n the kinds still open among arg's board_arg_kinds bits, a random open kind gets the next choice slot and its anim.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: board_arg_kinds, cell_type, anim, seed
 | Params: cell -- the cell arg was placed on; arg -- the cell's arg, 7 or more; kinds -- choice kinds still open; choices -- choice cells made, counted up here
 | Returns: the kinds still open
 ----------------------*/
static uint8_t board_choice(uint8_t cell, uint8_t arg, uint8_t kinds, uint8_t *choices) {
    uint8_t open = (uint8_t)(ram_board_arg_kinds((uint8_t)(arg - 7u)) & kinds);
    uint8_t count = 0, n, bit = 0, mask = 1u, k;
    if (!ram_board_arg_kinds((uint8_t)(arg - 7u)) || *choices == 5u || !open) {
        return kinds;
    }
    for (k = 0; k < 8u; k++) {
        count = (uint8_t)(count + ((open >> k) & 1u));
    }
    n = (uint8_t)(game_random((uint16_t)((count + 30u) << 4)) >> 4);
    if (n >= count) {
        return kinds;
    }
    for (;;) {
        if (open & mask) {
            if (!n) {
                break;
            }
            n--;
        }
        bit++;
        mask = (uint8_t)(mask << 1);
    }
    set_cell_type(cell, (uint8_t)(*choices + BOARD_CHOICE_SLOT));
    game_anim_start((uint8_t)(*choices + BOARD_CHOICE_SLOT), bit, 0);
    (*choices)++;
    return (uint8_t)(kinds & ~mask);
}

/*----------------------
 | game_board_fill
 | Description: Lays out the board: clears every cell, puts arg 0 on cell 0 or 16, then args on every fourth and every even cell, choice cells and scene cells. Converted from the listing and unverified: sel_a 0, more args than free cells, choice cells and odds sets 1-2. Counts are bytes, not the ARM's 32 bits, which differs only for board_steps above 7 or board_args below it (the card holds 6 and 12).
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: cell_arg, cell_type, sel_a, odds_set, board_steps, board_args, board_arg_set, board_arg_kinds, anim, seed
 | Params: N/A
 | Returns: the choice kinds still open, which the adapter hands back only when the even-cell pass ran
 ----------------------*/
uint8_t game_board_fill(void) {
    uint8_t i, arg, free, left, placed, odds, type, kinds = 0xFFu, choices = 0;
    uint32_t set;
    for (i = 0; i < 32u; i++) {
        set_cell_arg(i, 0xFFu);
        set_cell_type(i, 0xFFu);
    }
    set_cell_arg(ram_sel_a() ? 0x10u : 0u, 0);
    placed = ram_board_steps();
    left = placed;
    if (left) {
        set = ram_board_arg_set();
        free = 6u;
        arg = 0;
        do {
            arg = next_arg(&set, arg);
            set_cell_arg(board_free_cell(free, 4u, 0), arg);
            free--;
        } while (--left);
    }
    left = (uint8_t)(ram_board_args() - placed);
    if (left) {
        set = ram_board_arg_set() >> 6;
        free = (uint8_t)(14u - placed);
        arg = 6u;
        for (;;) {
            arg = next_arg(&set, arg);
            if (board_takes(left, free)) {
                uint8_t cell = board_free_cell(free, 2u, 0);
                set_cell_arg(cell, arg);
                placed++;
                kinds = board_choice(cell, arg, kinds, &choices);
                if (!--free) {
                    break;
                }
            }
            if (!--left) {
                break;
            }
        }
    }
    odds = ram_odds_set() == 0u ? 20u : ram_odds_set() == 1u ? 30u : 35u;
    free = (uint8_t)(30u - placed);
    for (type = 2u; type < 5u; type++) {
        if ((game_random(100u << 4) >> 4) >= (type == 3u ? 80u : odds)) {
            continue;
        }
        set_cell_type(board_free_cell(free, 1u, 1u), type);
        game_anim_start(type, ANIM_SCENE_CELL, 0);
        free--;
    }
    set_cell_arg(BOARD_TWIN, ram_cell_arg(8u));
    set_cell_type(BOARD_TWIN, ram_cell_type(8u));
    return kinds;
}

/*----------------------
 | BOARD_LIST
 | Description: Address of board_list, which game_board_gather fills.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BOARD_LIST 0x464u

/*----------------------
 | game_board_draw
 | Description: Steps the animations and draws the board around the walker: lists the cells on screen from the walker's cell, offset by the step in progress (or the reel game's slide in mode 11), draws their tiles, then each cell's item, its character and the walker, which goes in front of the cells below its step.
 | Author: suinevere
 | Dependencies: game_anims_tick, game_nibble, game_mirror_direction, game_board_gather, game_sprite_info, game_sprite_draw, game_anim_draw
 | Globals: sel_c, sel_d, sel_e, mode, mode_done, mode_phase, scratch_lo, scratch_hi, walk_busy, walk_frame, walk_kind, cell_saved, cell_arg, cell_type, board_list
 | Params: N/A
 | Returns: the entry count in the low byte and the walker's front threshold, 16-bit signed, above it
 ----------------------*/
uint32_t game_board_draw(void) {
    static const uint8_t bits[4] = { 1u, 2u, 4u, 8u }, ahead[4] = { 8u, 4u, 2u, 1u };
    uint8_t exits, facing, turn = 0, count, i, cell, arg, type, step, kind, walker;
    int16_t x = 0, y = 0, below = 0;
    int8_t ex, ey;
    SpriteInfo tile, s;
    game_anims_tick();
    exits = game_nibble(ram_sel_c());
    if (ram_mode() == 11u && ram_mode_done() >= 10u) {
        x = (int8_t)ram_scratch_lo();
        y = (int8_t)ram_scratch_hi();
    } else if (ram_walk_busy() != 0u) {
        step = (uint8_t)(ram_walk_frame() >> 1);
        kind = ram_walk_kind();
        if (kind >= 2u) {
            y = step;
        } else {
            turn = ram_sel_d();
            if (kind == 1u) {
                turn = exits == 0x0Fu ? (uint8_t)game_mirror_direction(turn) : (uint8_t)(exits & ~turn);
            }
            if ((turn & 3u) == 0u) {
                y = (int16_t)-step;
                if (step >= 4u) {
                    below = (int16_t)(8 - step);
                }
            } else {
                y = step;
                if (step < 4u) {
                    below = step;
                }
            }
            x = (turn & 5u) ? (int16_t)(step * 2) : (int16_t)-(step * 2);
        }
    }
    set_board_list(0, (uint8_t)y);
    set_board_list(1, (uint8_t)x);
    set_board_list(2, ram_sel_c());
    count = 1;
    if (exits == 0x0Fu) {
        count = game_board_gather(x, y, 8u, 0x82u, 2u, count, BOARD_LIST);
        count = game_board_gather(x, y, 8u, 4u, 2u, count, BOARD_LIST);
        count = game_board_gather(x, y, 0x18u, 0x81u, 2u, count, BOARD_LIST);
        count = game_board_gather(x, y, 0x18u, 8u, 2u, count, BOARD_LIST);
    } else {
        facing = ram_sel_d();
        for (i = 0; i < 4u; i++) {
            if (exits & bits[i]) {
                count = game_board_gather(x, y, ram_sel_c(), (uint8_t)((facing & bits[i]) ? ahead[i] : (0x80u | ahead[i])),
                                          (uint8_t)((turn & bits[i]) ? 3u : 2u), count, BOARD_LIST);
            }
        }
    }
    tile = game_sprite_info(0, 0, 0);
    for (i = 0; i < count; i++) {
        game_sprite_draw((int16_t)(tile.x + (int8_t)ram_board_list((uint8_t)(i * 3u + 1u))),
                         (int16_t)(tile.y + (int8_t)ram_board_list((uint8_t)(i * 3u))), tile.width, tile.height, tile.bitmap, tile.flags);
    }
    walker = ram_sel_e();
    for (i = 0; i < count; i++) {
        ey = (int8_t)ram_board_list((uint8_t)(i * 3u));
        ex = (int8_t)ram_board_list((uint8_t)(i * 3u + 1u));
        cell = ram_board_list((uint8_t)(i * 3u + 2u));
        if (walker != 0u && ey > below) {
            game_anim_draw(0, 0, 0);
            walker = 0;
        }
        arg = ram_cell_arg(cell);
        if (arg != 0xFFu) {
            s = game_sprite_info(ex, ey, (uint16_t)(arg + 1u));
            game_sprite_draw(s.x, s.y, s.width, s.height, s.bitmap, s.flags);
        }
        type = ram_cell_type(cell);
        if (type != 0xFFu && type != 0x10u) {
            game_anim_draw(ex, ey, type);
        }
        if (ram_sel_e() == 0u) {
            below = ram_cell_saved();
            if (cell == ram_cell_saved()) {
                game_anim_draw(ex, ey, 0);
                if (ram_mode() == 11u && ram_mode_done() >= 10u) {
                    game_anim_draw(ex, (int16_t)(ey - ram_mode_phase()), 1);
                }
            }
        }
    }
    if (walker != 0u) {
        game_anim_draw(0, 0, 0);
    }
    return count | ((uint32_t)(uint16_t)below << 8);
}
