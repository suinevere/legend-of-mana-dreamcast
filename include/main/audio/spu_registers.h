/*----------------------
 | spu_registers.h
 | Description: The PS1 SPU registers AKAO writes directly, all relative to SPU_REGISTER_BASE so a port can point them at a RAM shadow.
 | Author: suinevere
 | Dependencies: u16/s16 from common.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_SPU_REGISTERS_H
#define LOM_SPU_REGISTERS_H

/*----------------------
 | SPU_REGISTER_BASE
 | Description: Address of the first SPU voice register; a port defines it first to redirect every register below.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef SPU_REGISTER_BASE
#define SPU_REGISTER_BASE 0x1F801C00
#endif

/*----------------------
 | SpuVoiceRegisters
 | Description: One 16-byte SPU voice register block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    s16 volume_left;
    s16 volume_right;
    u16 pitch;
    u16 start_addr;
    u16 adsr_low;
    u16 adsr_high;
    u16 envelope_level;
    u16 repeat_addr;
} SpuVoiceRegisters;

/*----------------------
 | SPU_VOICE_REGS / SPU_VOICE_REG
 | Description: The voice register blocks, and one register of a voice addressed as voice 0's register plus the block stride.
 | Author: suinevere
 | Dependencies: SPU_REGISTER_BASE, SpuVoiceRegisters
 | Globals: N/A
 | Params: voice -- voice index, field -- SpuVoiceRegisters member
 | Returns: N/A
 ----------------------*/
#define SPU_VOICE_REGS ((SpuVoiceRegisters*)SPU_REGISTER_BASE)
#define SPU_VOICE_REG(voice, field) (*(u16*)((u8*)&SPU_VOICE_REGS[0].field + (voice) * sizeof(SpuVoiceRegisters)))

/*----------------------
 | SPU_MASTER_VOLUME_LEFT / SPU_MASTER_VOLUME_RIGHT
 | Description: SPU main volume registers.
 | Author: suinevere
 | Dependencies: SPU_REGISTER_BASE
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_MASTER_VOLUME_LEFT (*(s16*)(SPU_REGISTER_BASE + 0x180))
#define SPU_MASTER_VOLUME_RIGHT (*(s16*)(SPU_REGISTER_BASE + 0x182))

/*----------------------
 | SPU voice bitmap registers
 | Description: Key on/off, pitch modulation, noise and reverb bitmaps, each split into voices 0-15 (low) and 16-23 (high).
 | Author: suinevere
 | Dependencies: SPU_REGISTER_BASE
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_KEY_ON_LOW (*(u16*)(SPU_REGISTER_BASE + 0x188))
#define SPU_KEY_ON_HIGH (*(u16*)(SPU_REGISTER_BASE + 0x18A))
#define SPU_KEY_OFF_LOW (*(u16*)(SPU_REGISTER_BASE + 0x18C))
#define SPU_KEY_OFF_HIGH (*(u16*)(SPU_REGISTER_BASE + 0x18E))
#define SPU_PITCH_MOD_LOW (*(u16*)(SPU_REGISTER_BASE + 0x190))
#define SPU_PITCH_MOD_HIGH (*(u16*)(SPU_REGISTER_BASE + 0x192))
#define SPU_NOISE_LOW (*(u16*)(SPU_REGISTER_BASE + 0x194))
#define SPU_NOISE_HIGH (*(u16*)(SPU_REGISTER_BASE + 0x196))
#define SPU_REVERB_LOW (*(u16*)(SPU_REGISTER_BASE + 0x198))
#define SPU_REVERB_HIGH (*(u16*)(SPU_REGISTER_BASE + 0x19A))

/*----------------------
 | SPU_CONTROL_ADDRESS
 | Description: Address of the SPU control register (SPUCNT).
 | Author: suinevere
 | Dependencies: SPU_REGISTER_BASE
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_CONTROL_ADDRESS (SPU_REGISTER_BASE + 0x1AA)

/*----------------------
 | SPU_CD_VOLUME_LEFT / SPU_CD_VOLUME_RIGHT
 | Description: SPU CD-audio input volume registers.
 | Author: suinevere
 | Dependencies: SPU_REGISTER_BASE
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_CD_VOLUME_LEFT (*(s16*)(SPU_REGISTER_BASE + 0x1B0))
#define SPU_CD_VOLUME_RIGHT (*(s16*)(SPU_REGISTER_BASE + 0x1B2))

#endif
