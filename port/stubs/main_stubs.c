/*----------------------
 | main_stubs.c
 | Description: Saturn and Dreamcast stand-ins for the PS1 main executable's libc copy and AKAO sound services.
 | Author: suinevere
 | Dependencies: lom_prefix.h, decomp main/, common/ and audio headers
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "common/saved_game.h"
#include "main/main.h"
#include "main/game_state.h"
#include "main/audio/akao.h"
#include "main/audio/akao_cmd.h"
#include "main/audio/game_audio.h"

/*----------------------
 | bcopy
 | Description: Overlapping-safe byte copy with the Psy-Q argument order (source first).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: src -- source, dst -- destination, count -- bytes
 | Returns: dst
 ----------------------*/
void *bcopy(const unsigned char *src, unsigned char *dst, int count)
{
    int i;

    if (dst < src)
    {
        for (i = 0; i < count; i++)
        {
            dst[i] = src[i];
        }
    }
    else
    {
        for (i = count - 1; i >= 0; i--)
        {
            dst[i] = src[i];
        }
    }
    return dst;
}

/*----------------------
 | akao_* sound stubs
 | Description: Sound is out of scope for the title proof of concept; every AKAO call is a silent no-op.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: as declared in akao.h / akao_cmd.h
 | Returns: 0 where a value is expected
 ----------------------*/
void akao_stop_song(s32 stop_mode)
{
    (void)stop_mode;
}

s32 akao_fade_song_volume(s32 song_handle, s32 ticks, s32 volume)
{
    (void)song_handle;
    (void)ticks;
    (void)volume;
    return 0;
}

void akao_play_sfx(s32 sound_id, s32 tag, s32 pan, s32 volume)
{
    (void)sound_id;
    (void)tag;
    (void)pan;
    (void)volume;
}

void akao_set_mono_output(s32 mono)
{
    (void)mono;
}

s32 akao_register_bank(AkaoHeader *bank)
{
    (void)bank;
    return 0;
}

s32 akao_play_song(AkaoHeader *sequence)
{
    (void)sequence;
    return 0;
}

s32 akao_set_song_volume(s32 song_handle, s32 volume)
{
    (void)song_handle;
    (void)volume;
    return 0;
}

void akao_upload_bank_blocking(AkaoBankHeader *bank, s32 wait_for_completion)
{
    (void)bank;
    (void)wait_for_completion;
}

s32 akao_init(void)
{
    return 0;
}

s32 akao_stop_all_songs(void)
{
    return 0;
}

s32 akao_release_all_sfx(void)
{
    return 0;
}

s32 akao_fade_song_volume_from(s32 song_handle, s32 ticks, s32 start_volume, s32 volume)
{
    (void)song_handle;
    (void)ticks;
    (void)start_volume;
    (void)volume;
    return 0;
}

uintptr_t akao_play_sfx_from_buffer(AkaoHeader *buffer, s32 tag, s32 pan, s32 volume)
{
    (void)buffer;
    (void)tag;
    (void)pan;
    (void)volume;
    return 0;
}

void akao_fade_all_sfx_volume(s32 ticks, s32 volume)
{
    (void)ticks;
    (void)volume;
}

s32 akao_set_master_pan(s32 pan)
{
    (void)pan;
    return 0;
}

void load_and_play_song(s32 song_index)
{
    (void)song_index;
}

/*----------------------
 | fade_out_current_song
 | Description: Songs are not ported; returns at once.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void fade_out_current_song(void)
{
}
