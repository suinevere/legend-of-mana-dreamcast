/*----------------------
 | app.c
 | Description: Ring Ring Land's entry, main loop and quit dialog: setting up work RAM and the kernel callbacks, running one mode frame per tick, counting idle frames toward standby, and asking before quitting.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: seed, cycle16, mode, btn_prev, btn_press, idle_lo, repeat_up
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | IDLE_FRAMES
 | Description: The idle counter's restart value: 0xF00 frames without a button before standby.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define IDLE_FRAMES 0x0F00u

/*----------------------
 | set_idle
 | Description: Stores the 16-bit idle counter, whose high byte is repeat_up as in the original.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: idle_lo, repeat_up
 | Params: n -- the count
 | Returns: N/A
 ----------------------*/
static void set_idle(uint16_t n) {
    set_idle_lo((uint8_t)n);
    set_repeat_up((uint8_t)(n >> 8));
}

/*----------------------
 | quit_requested
 | Description: Whether the system asks the game to end (the kernel's communication flags' bit 8 in the original).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 1 when asked, else 0
 ----------------------*/
static uint8_t quit_requested(void) {
    return (vmu_sys_flags() & VMU_SYS_QUIT) != 0u;
}

/*----------------------
 | game_quit_dialog
 | Description: Silences the sound and asks YES or NO with its two rows of text, the choice inverted; up or down toggles it with a beep, a press beeps and quits on YES or reads the buttons again on NO. A kernel quit request quits at once; the original jumps there without popping its return, which only the terminating kernel call makes harmless. Converted from the listing and unverified: no script holds fire long enough.
 | Author: suinevere
 | Dependencies: game_sound_stop, game_quit_sound, game_wait_ticks, game_read_buttons, quit_requested, game_quit, game_fb_clear, game_dock_screen, game_fb_put_rows, game_fb_present
 | Globals: btn_press
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_quit_dialog(void) {
    uint8_t yes = 0;
    game_sound_stop();
    game_quit_sound();
    game_wait_ticks(1u, 0);
    game_read_buttons(0);
    for (;;) {
        if (quit_requested()) {
            game_quit();
            return;
        }
        game_fb_clear();
        game_dock_screen();
        game_fb_put_rows(8, 7, 0x020064B0u, yes == 0u);
        game_fb_put_rows(0x11, 7, 0x020064CCu, yes != 0u);
        game_wait_ticks(1u, 1u);
        game_fb_present();
        game_read_buttons(0);
        if (ram_btn_press() & BTN_FIRE) {
            game_quit_sound();
            if (yes) {
                game_quit();
            } else {
                game_read_buttons(0);
            }
            return;
        }
        if (ram_btn_press() & (BTN_UP | BTN_DOWN)) {
            yes ^= 1u;
            game_quit_sound();
        }
    }
}

/*----------------------
 | game_main_loop
 | Description: Sets up the frame buffer, Timer 1's ticks, the sound and the state block, then runs a frame per tick: the current mode's handler, a seed stir, the present and the buttons, with a long fire hold opening the quit dialog, 0xF00 idle frames sending the app to standby and mode 0xFF restarting the game through the dialog. It ends only when the kernel asks for a quit. Never returns in the original, so only the drive and host runs check it.
 | Author: suinevere
 | Dependencies: game_dock_check, game_fb_clear, game_fb_present, game_ticks_start, game_timer_run, game_sound_init, game_state_reset, game_mode0_leave, game_buttons_init, game_lcd_mode, game_wait_ticks, game_read_buttons, quit_requested, game_quit, game_dock_screen, game_sound_play_waiting, game_mode_frame, game_random, game_quit_dialog, game_standby, set_idle
 | Globals: seed, cycle16, mode, btn_prev, idle_lo, repeat_up
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_main_loop(void) {
    uint32_t idle;
    uint8_t cycle;
    game_dock_check();
    game_fb_clear();
    game_fb_present();
    game_ticks_start(1u, 0x3D09u, 0);
    game_timer_run(0, 0);
    game_sound_init();
    game_state_reset();
    set_seed(0);
    set_cycle16(16);
    game_mode0_leave();
    for (;;) {
        set_idle(IDLE_FRAMES);
        game_buttons_init(0x60u);
        game_lcd_mode(0, 2);
        game_wait_ticks(1u, 0);
        game_read_buttons(0);
        game_wait_ticks(1u, 1u);
        game_read_buttons(0);
        for (;;) {
            if (quit_requested()) {
                game_quit();
                return;
            }
            game_fb_clear();
            game_dock_screen();
            game_sound_play_waiting();
            cycle = (uint8_t)(ram_cycle16() - 1u);
            set_cycle16(cycle == 0u ? 16u : cycle);
            game_mode_frame();
            game_random(0x100u);
            game_wait_ticks(1u, 1u);
            game_fb_present();
            if (game_read_buttons(1u)) {
                game_quit_dialog();
            }
            if (ram_btn_prev() & 0x1Fu) {
                idle = IDLE_FRAMES;
            } else {
                idle = (uint32_t)(ram_idle_lo() | (ram_repeat_up() << 8)) - 1u;
                if (idle == 0u) {
                    game_fb_clear();
                    game_fb_present();
                    game_standby();
                    break;
                }
            }
            set_idle((uint16_t)idle);
            if (ram_mode() == 0xFFu) {
                set_mode(0);
                game_mode0_leave();
                game_quit_dialog();
            }
        }
    }
}

/*----------------------
 | game_entry
 | Description: The app's entry: sets clock speed 5, clears vmu_ram (work RAM 0x200-0x7FF in the original), masks every interrupt, installs the IRQ and FIQ callbacks, unmasks lines 9-11 and runs the main loop.
 | Author: suinevere
 | Dependencies: game_main_loop
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_entry(void) {
    vmu_set_clock(5u);
    for (uint16_t at = 0; at < sizeof vmu_ram; at++) {
        vmu_w8(at, 0);
    }
    vmu_irq_disable(0x3FFFu);
    vmu_set_handlers(game_irq_callback, game_fiq_callback);
    vmu_irq_enable(0xE00u);
    game_main_loop();
}
