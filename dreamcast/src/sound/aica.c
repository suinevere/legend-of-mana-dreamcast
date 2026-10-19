/*----------------------
 | aica.c
 | Description: SpuBackend on the AICA: uploads into sound RAM, channel key on/off and live register changes over G2, DSP reverb program load and live control, EG, play-position and sample readback through the monitor-select register, and the uploaded noise table.
 | Author: suinevere
 | Dependencies: aica.h, aica_regs.h, aica_reverb.h, aica_reverb_program.h, aica_stream.h, spu_noise.h, dc_system.h, dc/g2bus.h, dc/spu.h, arch/irq.h, string.h
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
#include "aica_reverb.h"
#define AICA_REVERB_PROGRAM_ARRAYS
#include "aica_reverb_program.h"
#include "aica_stream.h"
#include "dc_system.h"
#include "spu_noise.h"

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
 | REVERB_RING_REG
 | Description: Common register holding the reverb ring's RBL/RBP.
 | Author: suinevere
 | Dependencies: COMMON_REG
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define REVERB_RING_REG COMMON_REG(0x04u)

/*----------------------
 | COEF_REG / MADRS_REG / MPRO_REG / EFFECT_REG
 | Description: SH-4 addresses of a DSP COEF, MADRS and MPRO (linear step*4+word) word, and an effect output's EFSDL/EFPAN word.
 | Author: suinevere
 | Dependencies: AICA_REG_BASE
 | Globals: N/A
 | Params: index -- table index
 | Returns: address
 ----------------------*/
#define COEF_REG(index) (AICA_REG_BASE + 0x3000u + (unsigned)(index) * 4u)
#define MADRS_REG(index) (AICA_REG_BASE + 0x3200u + (unsigned)(index) * 4u)
#define MPRO_REG(index) (AICA_REG_BASE + 0x3400u + (unsigned)(index) * 4u)
#define EFFECT_REG(index) (AICA_REG_BASE + 0x2000u + (unsigned)(index) * 4u)

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
 | s_pcm_offset / s_pcm8
 | Description: Each channel's cached PCM start address (bytes) and sample width, set at key-on, read back by sample().
 | Author: suinevere
 | Dependencies: AICA_CHANNELS
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_pcm_offset[AICA_CHANNELS];
static int s_pcm8[AICA_CHANNELS];

/*----------------------
 | s_reverb_on / s_depth_left / s_depth_right
 | Description: Live reverb enable flag and output depth (vLOUT/vROUT), reapplied to the DSP's COEF table on every change.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_reverb_on;
static int s_depth_left;
static int s_depth_right;

/*----------------------
 | s_noise_table
 | Description: The rendered PS1 SPU noise sequence, uploaded once to SPU_NOISE_RAM at aica_init.
 | Author: suinevere
 | Dependencies: SPU_NOISE_FRAMES
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static signed char s_noise_table[SPU_NOISE_FRAMES];

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
 | write_reverb_coefs
 | Description: Computes and writes the eight live reverb COEF words for the current on/depth state.
 | Author: suinevere
 | Dependencies: aica_reverb_coefs, write_reg, COEF_REG, AICA_REVERB_STEP_*
 | Globals: N/A
 | Params: on -- reverb enabled, left -- left output depth, right -- right output depth
 | Returns: N/A
 ----------------------*/
static void write_reverb_coefs(int on, int left, int right)
{
    static const int steps[8] = {AICA_REVERB_STEP_INPUT_L0, AICA_REVERB_STEP_INPUT_L1,
                                  AICA_REVERB_STEP_INPUT_R0, AICA_REVERB_STEP_INPUT_R1,
                                  AICA_REVERB_STEP_LEFT_DEPTH, AICA_REVERB_STEP_LEFT_APF,
                                  AICA_REVERB_STEP_RIGHT_DEPTH, AICA_REVERB_STEP_RIGHT_APF};
    unsigned short coefs[8];
    int i;

    aica_reverb_coefs(on, left, right, coefs);
    for (i = 0; i < 8; i++)
        write_reg(COEF_REG(steps[i]), coefs[i]);
}

/*----------------------
 | aica_init
 | Description: Holds the ARM7 in reset, keys every channel off, sets the master volume to full stereo, loads the Studio Large reverb program onto the DSP with reverb off, and renders and uploads the noise table.
 | Author: suinevere
 | Dependencies: spu_disable, spu_reset_chans, spu_master_mixer, spu_memset, spu_memload, write_reg, write_reverb_coefs, aica_reverb_coef, aica_reverb_madrs, aica_reverb_mpro, spu_noise_render
 | Globals: s_keyed, s_pcm_offset, s_pcm8, s_reverb_on, s_depth_left, s_depth_right, s_noise_table
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void aica_init(void)
{
    int i;

    spu_disable();
    spu_reset_chans();
    spu_master_mixer(15, 1);
    memset(s_keyed, 0, sizeof(s_keyed));
    memset(s_pcm_offset, 0, sizeof(s_pcm_offset));
    memset(s_pcm8, 0, sizeof(s_pcm8));
    s_reverb_on = 0;
    s_depth_left = 0;
    s_depth_right = 0;
    spu_memset(AICA_REVERB_RAM, 0x60006000u, AICA_REVERB_RAM_BYTES);
    for (i = 0; i < 128; i++)
        write_reg(COEF_REG(i), aica_reverb_coef[i]);
    for (i = 0; i < 64; i++)
        write_reg(MADRS_REG(i), aica_reverb_madrs[i]);
    for (i = 0; i < 512; i++)
        write_reg(MPRO_REG(i), aica_reverb_mpro[i]);
    write_reg(REVERB_RING_REG, AICA_REVERB_RING);
    write_reg(EFFECT_REG(0), 0x0F1Fu);
    write_reg(EFFECT_REG(1), 0x0F0Fu);
    write_reverb_coefs(s_reverb_on, s_depth_left, s_depth_right);
    spu_noise_render(s_noise_table, SPU_NOISE_FRAMES);
    spu_memload(SPU_NOISE_RAM, s_noise_table, SPU_NOISE_FRAMES);
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
 | Globals: s_keyed, s_pcm_offset, s_pcm8
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
    s_pcm_offset[voice] = setup->span.pcm_offset;
    s_pcm8[voice] = setup->pcm8;
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
 | set_send
 | Description: Rewrites a sounding channel's DSP send (ISEL/IMXL) word.
 | Author: suinevere
 | Dependencies: write_reg
 | Globals: N/A
 | Params: voice -- channel, isel -- MIXS bus, imxl -- send level
 | Returns: N/A
 ----------------------*/
static void set_send(int voice, unsigned char isel, unsigned char imxl)
{
    write_reg(CHANNEL_REG(voice, 8), ((unsigned)imxl << 4) | isel);
}

/*----------------------
 | reverb_enable / reverb_depth
 | Description: Update the live reverb state and rewrite the DSP's eight live COEF words to match.
 | Author: suinevere
 | Dependencies: write_reverb_coefs
 | Globals: s_reverb_on, s_depth_left, s_depth_right
 | Params: on -- reverb enabled; left, right -- output depth (vLOUT/vROUT)
 | Returns: N/A
 ----------------------*/
static void reverb_enable(int on)
{
    s_reverb_on = on;
    write_reverb_coefs(s_reverb_on, s_depth_left, s_depth_right);
}

static void reverb_depth(int left, int right)
{
    s_depth_left = left;
    s_depth_right = right;
    write_reverb_coefs(s_reverb_on, s_depth_left, s_depth_right);
}

/*----------------------
 | reverb_clear
 | Description: Clears the reverb ring to packed zero.
 | Author: suinevere
 | Dependencies: spu_memset, AICA_REVERB_RAM, AICA_REVERB_RAM_BYTES
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void reverb_clear(void)
{
    spu_memset(AICA_REVERB_RAM, 0x60006000u, AICA_REVERB_RAM_BYTES);
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
 | read_pcm_byte / read_pcm_halfword
 | Description: Reads one 8- or 16-bit PCM sample from sound RAM at a byte offset, through a word-aligned G2 read.
 | Author: suinevere
 | Dependencies: g2_read_32, AICA_RAM_BASE
 | Globals: N/A
 | Params: offset -- sound RAM byte offset
 | Returns: signed sample
 ----------------------*/
static int read_pcm_byte(unsigned offset)
{
    uintptr_t word = AICA_RAM_BASE + (offset & ~3u);
    unsigned shift = (offset & 3u) * 8u;

    return (int)(signed char)(g2_read_32(word) >> shift);
}

static int read_pcm_halfword(unsigned offset)
{
    uintptr_t word = AICA_RAM_BASE + (offset & ~3u);
    unsigned shift = (offset & 2u) ? 16u : 0u;

    return (int)(short)(g2_read_32(word) >> shift);
}

/*----------------------
 | sample
 | Description: A channel's current PCM sample at its cached start address plus its play position, as a signed value scaled to 16 bits.
 | Author: suinevere
 | Dependencies: monitor_read, read_pcm_byte, read_pcm_halfword
 | Globals: s_pcm_offset, s_pcm8
 | Params: voice -- channel
 | Returns: signed sample
 ----------------------*/
static int sample(int voice)
{
    unsigned ca = monitor_read(voice, MONITOR_POSITION);

    if (s_pcm8[voice])
        return read_pcm_byte(s_pcm_offset[voice] + ca) << 8;
    return read_pcm_halfword(s_pcm_offset[voice] + 2u * ca);
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
 | Dependencies: upload, key_on, key_off, set_pitch, set_level, set_envelope, envelope, position, aica_stream_volume, log_line, set_send, reverb_enable, reverb_depth, reverb_clear, sample
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
const SpuBackend aica_backend = {upload, key_on, key_off, set_pitch, set_level, set_envelope, envelope, position, aica_stream_volume, log_line,
                                  set_send, reverb_enable, reverb_depth, reverb_clear, sample};
