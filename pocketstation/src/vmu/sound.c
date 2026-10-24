/*----------------------
 | sound.c
 | Description: Ring Ring Land's sound timing: setting it up, starting, queueing and stopping a sound, and the sound-end interrupt that chains to the next sound or stops.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: snd_start, snd_pos, snd_len, snd_playing, snd_next, snd_mode, sound_req, docked
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

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
 | game_sound_fiq
 | Description: Sound-end interrupt, the original's sample interrupt at pos == len: starts the queued sound at the slow rate, the only one a queue follows, its first sample counted as played as the original did, or stops the sound end and output.
 | Author: suinevere
 | Dependencies: rrl_ram.h, vmu_hw.h
 | Globals: snd_start, snd_pos, snd_len, snd_playing, snd_next
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_fiq(void) {
    uint32_t start;
    uint8_t next = ram_snd_next();
    if (next == 0xFFu) {
        set_snd_playing(0xFFu);
        set_snd_next(0xFFu);
        vmu_sound_stop();
        vmu_irq_disable(VMU_LINE_SOUND);
        game_sound_off();
        return;
    }
    set_snd_playing(next);
    set_snd_next(0xFFu);
    start = rom32(ROM_SOUNDS + next * 4u);
    set_snd_start((uint16_t)(start - ROM_BASE));
    set_snd_len(rom32(ROM_SOUNDS + next * 4u + 4u) - start);
    set_snd_pos(1u);
    vmu_sound_play(ram_snd_len() - 1u, VMU_SOUND_SLOW);
}

/*----------------------
 | game_sound_start
 | Description: Unless snd_mode is 2, cuts the playing sound short to end after one more sample and queues id after it, or with none playing starts id and times its end at counts per sample. Sample addresses and lengths are 32-bit; snd_start keeps the file offset. The snd_mode 2 and queueing paths are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: snd_mode, snd_pos, snd_len, snd_next, snd_playing, snd_start
 | Params: id -- sound id; counts -- VMU_SOUND_* sample reload
 | Returns: N/A
 ----------------------*/
void game_sound_start(uint8_t id, uint16_t counts) {
    uint32_t start;
    if (ram_snd_mode() == 2u) {
        return;
    }
    set_snd_pos(ram_snd_len());
    set_snd_next(id);
    if (ram_snd_playing() != 0xFFu) {
        vmu_sound_play(0, counts);
        return;
    }
    set_snd_next(0xFFu);
    set_snd_playing(id);
    start = rom32(ROM_SOUNDS + id * 4u);
    set_snd_start((uint16_t)(start - ROM_BASE));
    set_snd_len(rom32(ROM_SOUNDS + id * 4u + 4u) - start);
    set_snd_pos(0);
    game_sound_on();
    game_timer_irq(2u, 1u);
    vmu_sound_play(ram_snd_len(), counts);
}

/*----------------------
 | game_sound_stop
 | Description: Stops the sound end and its interrupt and clears the playing and queued sounds; the speaker is switched off only when nothing was playing, as in the original. That switch-off is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: snd_playing, snd_next
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_stop(void) {
    uint8_t playing;
    vmu_sound_stop();
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
 | Description: Handler id of the sound-end interrupt, which game_sound_init installs as handler 13.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SOUND_FIQ_ENTRY HANDLER_SOUND

/*----------------------
 | game_sound_init
 | Description: Takes snd_mode from the system flags (bits 18-19 of the kernel's user-interface status in the original), clears the playing, queued and requested sounds, switches the speaker off, stops the sound end and its interrupt and installs the sound-end handler.
 | Author: suinevere
 | Dependencies: game_sound_off, game_timer_irq, game_set_irq_handler
 | Globals: snd_mode, snd_playing, snd_next, sound_req
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_init(void) {
    set_snd_mode((uint8_t)((vmu_sys_flags() & VMU_SYS_SND_MODE) >> 2));
    set_snd_playing(0xFFu);
    set_snd_next(0xFFu);
    set_sound_req(0xFFu);
    game_sound_off();
    vmu_sound_stop();
    game_timer_irq(2u, 0);
    game_set_irq_handler(13u, SOUND_FIQ_ENTRY);
}

/*----------------------
 | game_sound_begin
 | Description: Stops all sound when docked or the kernel's flag bit 0 is set; otherwise starts or queues sound id at the slow sample rate, the original's Timer 2 reload 500.
 | Author: suinevere
 | Dependencies: game_sound_stop, game_sound_start
 | Globals: docked, snd_playing
 | Params: id -- sound id
 | Returns: N/A
 ----------------------*/
void game_sound_begin(uint8_t id) {
    if (ram_docked() != 0u || (vmu_sys_flags() & VMU_SYS_MUTE) != 0u) {
        game_sound_stop();
        return;
    }
    game_sound_start(id, VMU_SOUND_SLOW);
}

/*----------------------
 | game_sound_play_waiting
 | Description: Plays the requested sound to its end with fast ticks before going on: stops all sound when docked or the kernel's flag bit 0 is set, cuts any sound playing, starts it at the fast sample rate and waits, dropping back to frame ticks when it ends or stopping it if docked meanwhile. Fast ticks and rate are the original's clock speed 6 and Timer 2 reload 0x47.
 | Author: suinevere
 | Dependencies: game_sound_stop, game_sound_start
 | Globals: sound_req, docked, snd_playing
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_sound_play_waiting(void) {
    uint8_t id = ram_sound_req();
    set_sound_req(0xFFu);
    if (id == 0xFFu) {
        return;
    }
    if (ram_docked() != 0u || (vmu_sys_flags() & VMU_SYS_MUTE) != 0u) {
        game_sound_stop();
        return;
    }
    if (ram_snd_playing() != 0xFFu) {
        game_sound_stop();
    }
    vmu_tick_mode(VMU_TICK_FAST);
    game_sound_start(id, VMU_SOUND_FAST);
    while (ram_docked() == 0u) {
        if (ram_snd_playing() == 0xFFu) {
            vmu_tick_mode(VMU_TICK_FRAME);
            return;
        }
        vmu_idle();
    }
    game_sound_stop();
}
