/*----------------------
 | sound.c
 | Description: Ring Ring Land's sample player: setting it up, starting, queueing and stopping a sound, and the Timer 2 interrupt that feeds one sample per tick to the DAC and chains to the next sound.
 | Author: suinevere
 | Dependencies: game.h, psk_cpu.h, psk_swi.h
 | Globals: snd_start, snd_pos, snd_len, snd_playing, snd_next, snd_mode, sound_req, docked, com_flags
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "psk_cpu.h"
#include "psk_swi.h"

/*----------------------
 | ROM_SOUNDS
 | Description: Sound sample bounds by id: start address, then the next id's start is this one's end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_SOUNDS 0x0200A7C8u

/*----------------------
 | TIMER2_MODE
 | Description: Timer 2's mode register; bit 2 runs it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define TIMER2_MODE 0x0A800028u

/*----------------------
 | DAC_OUT
 | Description: The DAC sample register.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DAC_OUT 0x0D800014u

/*----------------------
 | sample_at
 | Description: The DAC value for sample pos: in mode 1 a signed byte scaled by 64 into ten bits, else a byte in the high half.
 | Author: suinevere
 | Dependencies: psk_cpu.h
 | Globals: snd_mode
 | Params: start -- sample data; pos -- sample index
 | Returns: the DAC value
 ----------------------*/
static uint32_t sample_at(uint32_t start, uint32_t pos) {
    uint32_t h = psk_ldrh(start + (pos & ~1u));
    if (ram_snd_mode() == 1) {
        h = (pos & 1u) ? h << 16 : h << 24;
        return 0xFFC0u & psk_asr(h, 18);
    }
    if (!(pos & 1u)) {
        h <<= 8;
    }
    return h & 0xFF00u;
}

/*----------------------
 | game_sound_fiq
 | Description: Timer 2 interrupt: plays the next sample, starts the queued sound when one ends, or stops Timer 2 and the sound output when nothing is queued.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: snd_start, snd_pos, snd_len, snd_playing, snd_next
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_fiq(void) {
    uint32_t start, pos = ram_snd_pos();
    if (ram_snd_len() == pos) {
        uint8_t next = ram_snd_next();
        if (next == 0xFFu) {
            set_snd_playing(0xFFu);
            set_snd_next(0xFFu);
            psk_w32(TIMER2_MODE, psk_ldr(TIMER2_MODE) & ~4u);
            psk_w32(INT_MASK_CLR, 0x2000u);
            psk_w32(INT_ACK, 0x2000u);
            game_sound_off();
            return;
        }
        set_snd_playing(next);
        set_snd_next(0xFFu);
        start = psk_ldr(ROM_SOUNDS + next * 4u);
        set_snd_start(start);
        set_snd_len(psk_ldr(ROM_SOUNDS + next * 4u + 4u) - start);
        pos = 0;
    } else {
        start = ram_snd_start();
    }
    psk_w32(DAC_OUT, sample_at(start, pos));
    set_snd_pos(pos + 1u);
}

/*----------------------
 | game_sound_start
 | Description: Unless snd_mode is 2, cuts the playing sound short and queues id after it, or with none playing starts id and Timer 2's sample interrupt. Sample addresses and lengths are 32-bit. The snd_mode 2 and queueing paths are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h, psk_cpu.h
 | Globals: snd_mode, snd_pos, snd_len, snd_next, snd_playing, snd_start
 | Params: id -- sound id
 | Returns: N/A
 ----------------------*/
void game_sound_start(uint8_t id) {
    uint32_t start;
    if (ram_snd_mode() == 2u) {
        return;
    }
    set_snd_pos(ram_snd_len());
    set_snd_next(id);
    if (ram_snd_playing() != 0xFFu) {
        return;
    }
    set_snd_next(0xFFu);
    set_snd_playing(id);
    start = psk_ldr(ROM_SOUNDS + id * 4u);
    set_snd_start(start);
    set_snd_len(psk_ldr(ROM_SOUNDS + id * 4u + 4u) - start);
    set_snd_pos(0);
    game_sound_on();
    game_timer_irq(2u, 1u);
    game_timer_run(2u, 1u);
}

/*----------------------
 | game_sound_stop
 | Description: Stops Timer 2 and its interrupt and clears the playing and queued sounds; the speaker is switched off only when nothing was playing, as in the original. That switch-off is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: snd_playing, snd_next
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_stop(void) {
    uint8_t playing;
    game_timer_run(2u, 0u);
    game_timer_irq(2u, 0u);
    playing = ram_snd_playing();
    set_snd_playing(0xFFu);
    set_snd_next(0xFFu);
    if (playing == 0xFFu) {
        game_sound_off();
    }
}

/*----------------------
 | SOUND_FIQ_ENTRY
 | Description: Address of the Timer 2 sample interrupt, which game_sound_init installs as handler 13.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SOUND_FIQ_ENTRY 0x02003B64u

/*----------------------
 | game_sound_init
 | Description: Takes snd_mode from bits 18-19 of the kernel's user-interface status, clears the playing, queued and requested sounds, switches the speaker off and readies Timer 2 with its sample interrupt.
 | Author: suinevere
 | Dependencies: psk_swi.h, game_sound_off, game_timer_reset, game_set_irq_handler
 | Globals: snd_mode, snd_playing, snd_next, sound_req
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_init(void) {
    uint32_t status = psk_ldr(psk_swi(0x13u, 0, 0, 0));
    set_snd_mode((uint8_t)((status >> 18) & 3u));
    set_snd_playing(0xFFu);
    set_snd_next(0xFFu);
    set_sound_req(0xFFu);
    game_sound_off();
    game_timer_reset(2u, 0x47u, 0);
    game_set_irq_handler(13u, SOUND_FIQ_ENTRY);
}

/*----------------------
 | game_sound_begin
 | Description: Stops all sound when docked or the kernel's flag bit 0 is set; otherwise readies Timer 2 at reload 500 when nothing is playing and starts or queues sound id.
 | Author: suinevere
 | Dependencies: game_sound_stop, game_timer_setup, game_sound_start
 | Globals: docked, com_flags, snd_playing
 | Params: id -- sound id
 | Returns: N/A
 ----------------------*/
void game_sound_begin(uint8_t id) {
    if (ram_docked() != 0u || (psk_ldr(ram_com_flags()) & 1u) != 0u) {
        game_sound_stop();
        return;
    }
    if (ram_snd_playing() == 0xFFu) {
        game_timer_setup(2u, 500u, 0, 0);
    }
    game_sound_start(id);
}

/*----------------------
 | game_sound_play_waiting
 | Description: Plays the requested sound to its end at clock speed 6 before going on: stops all sound when docked or the kernel's flag bit 0 is set, cuts any sound playing, readies Timer 2 at reload 0x47 and waits, dropping back to clock speed 5 when it ends or stopping it if docked meanwhile.
 | Author: suinevere
 | Dependencies: psk_swi.h, game_sound_stop, game_timer_setup, game_sound_start
 | Globals: sound_req, docked, com_flags, snd_playing
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_play_waiting(void) {
    uint8_t id = ram_sound_req();
    set_sound_req(0xFFu);
    if (id == 0xFFu) {
        return;
    }
    if (ram_docked() != 0u || (psk_ldr(ram_com_flags()) & 1u) != 0u) {
        game_sound_stop();
        return;
    }
    if (ram_snd_playing() != 0xFFu) {
        game_sound_stop();
    }
    psk_swi(0x04u, 6u, 0, 0);
    game_timer_setup(2u, 0x47u, 0, 0);
    game_sound_start(id);
    while (ram_docked() == 0u) {
        if (ram_snd_playing() == 0xFFu) {
            psk_swi(0x04u, 5u, 0, 0);
            return;
        }
        psk_idle();
    }
    game_sound_stop();
}
