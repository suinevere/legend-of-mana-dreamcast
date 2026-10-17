/*----------------------
 | aica.c
 | Description: SpuBackend on the AICA: uploads into sound RAM, channel key on/off and live register changes over G2, and EG and play-position readback through the monitor-select register.
 | Author: suinevere
 | Dependencies: aica.h, aica_regs.h, aica_stream.h, dc_system.h, dc/g2bus.h, dc/spu.h, arch/irq.h, string.h
 | Globals: aica_backend
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <string.h>

#include <arch/irq.h>
#include <dc/g2bus.h>
#include <dc/spu.h>

#include "aica.h"
#include "aica_regs.h"
#include "aica_stream.h"
#include "dc_system.h"

/*----------------------
 | AICA_CHANNELS
 | Description: AICA channels.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AICA_CHANNELS 64

/*----------------------
 | CHANNEL_REG / COMMON_REG
 | Description: SH-4 address of a channel register word and of a common register.
 | Author: suinevere
 | Dependencies: AICA_REG_BASE, AICA_CHANNEL_STRIDE, AICA_COMMON
 | Globals: N/A
 | Params: channel -- 0..63, word -- register word index, offset -- common register offset
 | Returns: address
 ----------------------*/
#define CHANNEL_REG(channel, word) (AICA_REG_BASE + (unsigned)(channel) * AICA_CHANNEL_STRIDE + (unsigned)(word) * 4u)
#define COMMON_REG(offset) (AICA_REG_BASE + AICA_COMMON + (offset))

/*----------------------
 | MONITOR_SELECT / MONITOR_EG / MONITOR_POSITION
 | Description: Common registers: monitor channel select (MSLC in bits 13:8), and the selected channel's EG and play position (CA).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MONITOR_SELECT 0x0Cu
#define MONITOR_EG 0x10u
#define MONITOR_POSITION 0x14u

/*----------------------
 | s_keyed
 | Description: Channels keyed on and not yet seen silent.
 | Author: suinevere
 | Dependencies: AICA_CHANNELS
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_keyed[AICA_CHANNELS];

/*----------------------
 | write_reg
 | Description: Writes one AICA register once the G2 write FIFO has room.
 | Author: suinevere
 | Dependencies: g2_fifo_wait, g2_write_32
 | Globals: N/A
 | Params: address -- register, value -- word
 | Returns: N/A
 ----------------------*/
static void write_reg(uintptr_t address, unsigned value)
{
    g2_fifo_wait();
    g2_write_32(address, value);
}

/*----------------------
 | monitor_read
 | Description: Selects a channel on the monitor and reads one of its monitor registers, with interrupts off so the select and read stay paired.
 | Author: suinevere
 | Dependencies: write_reg, g2_read_32, irq_disable, irq_restore
 | Globals: N/A
 | Params: channel -- 0..63, offset -- MONITOR_EG or MONITOR_POSITION
 | Returns: the register's low 16 bits
 ----------------------*/
static unsigned monitor_read(int channel, unsigned offset)
{
    int irq = irq_disable();
    unsigned value;

    write_reg(COMMON_REG(MONITOR_SELECT), (unsigned)channel << 8);
    value = g2_read_32(COMMON_REG(offset)) & 0xFFFFu;
    irq_restore(irq);
    return value;
}

/*----------------------
 | aica_init
 | Description: Holds the ARM7 in reset, keys every channel off and sets the master volume to full stereo.
 | Author: suinevere
 | Dependencies: spu_disable, spu_reset_chans, spu_master_mixer
 | Globals: s_keyed
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void aica_init(void)
{
    spu_disable();
    spu_reset_chans();
    spu_master_mixer(15, 1);
    memset(s_keyed, 0, sizeof(s_keyed));
}

/*----------------------
 | upload
 | Description: Copies PCM into sound RAM: whole aligned words through spu_memload, anything starting or ending on a halfword word by word with read-modify-write at the edges.
 | Author: suinevere
 | Dependencies: spu_memload, g2_read_32, write_reg
 | Globals: N/A
 | Params: aica_offset -- sound RAM byte offset, pcm -- frames, bytes -- byte count
 | Returns: N/A
 ----------------------*/
static void upload(unsigned aica_offset, const short *pcm, unsigned bytes)
{
    unsigned count = bytes / 2u;
    unsigned i = 0;

    if (((aica_offset | bytes | (unsigned)(uintptr_t)pcm) & 3u) == 0)
    {
        spu_memload(aica_offset, (void *)pcm, bytes);
        return;
    }
    while (i < count)
    {
        unsigned offset = aica_offset + 2u * i;
        uintptr_t word = AICA_RAM_BASE + (offset & ~3u);

        if ((offset & 2u) == 0 && i + 1u < count)
        {
            write_reg(word, (unsigned short)pcm[i] | ((unsigned)(unsigned short)pcm[i + 1u] << 16));
            i += 2u;
            continue;
        }
        if (offset & 2u)
            write_reg(word, (g2_read_32(word) & 0xFFFFu) | ((unsigned)(unsigned short)pcm[i] << 16));
        else
            write_reg(word, (g2_read_32(word) & 0xFFFF0000u) | (unsigned short)pcm[i]);
        i++;
    }
}

/*----------------------
 | key_on
 | Description: Keys a channel off, writes the voice's registers and keys it on, so a voice already sounding restarts like on the PS1.
 | Author: suinevere
 | Dependencies: aica_channel_words, write_reg
 | Globals: s_keyed
 | Params: voice -- channel, setup -- voice setup
 | Returns: N/A
 ----------------------*/
static void key_on(int voice, const SpuVoiceSetup *setup)
{
    unsigned short words[AICA_CHANNEL_WORDS];
    int w;

    aica_channel_words(setup, words);
    write_reg(CHANNEL_REG(voice, 0), words[0] | AICA_KYONEX);
    for (w = 1; w < AICA_CHANNEL_WORDS; w++)
        write_reg(CHANNEL_REG(voice, w), words[w]);
    write_reg(CHANNEL_REG(voice, 0), words[0] | AICA_KYONB | AICA_KYONEX);
    s_keyed[voice] = 1;
}

/*----------------------
 | key_off
 | Description: Clears a channel's key-on bit and executes it, starting its release.
 | Author: suinevere
 | Dependencies: g2_read_32, write_reg
 | Globals: N/A
 | Params: voice -- channel
 | Returns: N/A
 ----------------------*/
static void key_off(int voice)
{
    unsigned word = g2_read_32(CHANNEL_REG(voice, 0)) & 0xFFFFu;

    write_reg(CHANNEL_REG(voice, 0), (word & ~AICA_KYONB) | AICA_KYONEX);
}

/*----------------------
 | set_pitch / set_level / set_envelope
 | Description: Rewrite a sounding channel's pitch word, pan and TL words, or envelope words.
 | Author: suinevere
 | Dependencies: write_reg
 | Globals: N/A
 | Params: voice -- channel, and the new register values
 | Returns: N/A
 ----------------------*/
static void set_pitch(int voice, unsigned short pitch)
{
    write_reg(CHANNEL_REG(voice, 6), pitch);
}

static void set_level(int voice, unsigned char tl, unsigned char dipan)
{
    write_reg(CHANNEL_REG(voice, 9), 0x0F00u | dipan);
    write_reg(CHANNEL_REG(voice, 10), ((unsigned)tl << 8) | 0x20u);
}

static void set_envelope(int voice, unsigned short eg10, unsigned short eg14)
{
    write_reg(CHANNEL_REG(voice, 4), eg10);
    write_reg(CHANNEL_REG(voice, 5), eg14);
}

/*----------------------
 | envelope
 | Description: A channel's PS1 envelope level from its EG readback; a channel found silent stops counting as keyed and has its key-on bit cleared so another channel's key execute cannot restart it.
 | Author: suinevere
 | Dependencies: monitor_read, aica_envelope_level, g2_read_32, write_reg
 | Globals: s_keyed
 | Params: voice -- channel
 | Returns: 0..0x7FFF
 ----------------------*/
static int envelope(int voice)
{
    int level = aica_envelope_level((unsigned short)monitor_read(voice, MONITOR_EG), s_keyed[voice]);

    if (level == 0 && s_keyed[voice])
    {
        s_keyed[voice] = 0;
        write_reg(CHANNEL_REG(voice, 0), g2_read_32(CHANNEL_REG(voice, 0)) & 0xFFFFu & ~AICA_KYONB);
    }
    return level;
}

/*----------------------
 | position
 | Description: A channel's play position (CA) in frames from its start address.
 | Author: suinevere
 | Dependencies: monitor_read
 | Globals: N/A
 | Params: voice -- channel
 | Returns: frame index
 ----------------------*/
static unsigned position(int voice)
{
    return monitor_read(voice, MONITOR_POSITION);
}

/*----------------------
 | log_line
 | Description: Shim log lines go to the Dreamcast log.
 | Author: suinevere
 | Dependencies: dc_log
 | Globals: N/A
 | Params: message -- text
 | Returns: N/A
 ----------------------*/
static void log_line(const char *message)
{
    dc_log("%s", message);
}

/*----------------------
 | aica_backend
 | Description: SpuBackend answered by the AICA; SPU CD volume writes set the stream channel volume.
 | Author: suinevere
 | Dependencies: upload, key_on, key_off, set_pitch, set_level, set_envelope, envelope, position, aica_stream_volume, log_line
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
const SpuBackend aica_backend = {upload, key_on, key_off, set_pitch, set_level, set_envelope, envelope, position, aica_stream_volume, log_line};
