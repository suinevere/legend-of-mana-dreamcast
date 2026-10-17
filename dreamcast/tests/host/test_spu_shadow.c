/*----------------------
 | test_spu_shadow.c
 | Description: Host tests that AKAO's SPU register macros land in lom_spu_shadow at their PS1 offsets and that spu_shadow_take_dirty reports each change once.
 | Author: suinevere
 | Dependencies: spu_shadow.c, spu_shadow.h, include/main/audio/spu_registers.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>

#include "spu_shadow.h"

typedef unsigned short u16;
typedef signed short s16;
typedef unsigned char u8;

#include "main/audio/spu_registers.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)
#define PS1_INDEX(address) (((address) - 0x1F801C00u) / 2u)
#define BIT(bits, index) (((bits)[(index) / 32u] >> ((index) % 32u)) & 1u)

static int count_bits(const unsigned bits[8])
{
    int count = 0;
    int i;
    for (i = 0; i < 8 * 32; i++)
        count += (int)BIT(bits, (unsigned)i);
    return count;
}

int main(void)
{
    unsigned bits[8];
    unsigned pitch_index = PS1_INDEX(0x1F801C00u + 3u * 16u + 4u);
    unsigned key_index = PS1_INDEX(0x1F801D88u);

    spu_shadow_reset();
    CHECK(spu_shadow_take_dirty(bits) == 0);
    SPU_VOICE_REG(3, pitch) = 0x1234;
    SPU_KEY_ON_LOW = 0x8;
    CHECK(lom_spu_shadow[pitch_index] == 0x1234);
    CHECK(lom_spu_shadow[key_index] == 0x8);
    CHECK(spu_shadow_take_dirty(bits) != 0);
    CHECK(BIT(bits, pitch_index) && BIT(bits, key_index) && count_bits(bits) == 2);
    CHECK(lom_spu_shadow[pitch_index] == 0x1234);
    CHECK(spu_shadow_key_events[0] == 0x8);
    CHECK(spu_shadow_take_dirty(bits) == 0 && count_bits(bits) == 0);
    SPU_KEY_ON_LOW = 0x8;
    CHECK(spu_shadow_take_dirty(bits) != 0 && BIT(bits, key_index) && count_bits(bits) == 1);
    SPU_VOICE_REG(3, pitch) = 0x1234;
    CHECK(spu_shadow_take_dirty(bits) == 0);
    SPU_CD_VOLUME_RIGHT = 0x7FFF;
    CHECK(lom_spu_shadow[PS1_INDEX(0x1F801DB2u)] == 0x7FFF);
    CHECK(*(unsigned short *)SPU_CONTROL_ADDRESS == lom_spu_shadow[PS1_INDEX(0x1F801DAAu)]);
    if (failures == 0)
        printf("OK\n");
    return failures != 0;
}
