/*----------------------
 | mdec.h
 | Description: Software PS1 MDEC: run-length codes to 16x16 macroblocks in 24-bit or 15-bit colour, as psx-spx describes the hardware.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_MDEC_H
#define LOM_MDEC_H

#include <stdint.h>

/*----------------------
 | MDEC_EOB
 | Description: Run-length code ending a block; also the padding code between blocks.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MDEC_EOB 0xFE00

/*----------------------
 | MDEC_MB_BYTES_24 / MDEC_MB_BYTES_15
 | Description: Bytes of one decoded 16x16 macroblock in 24-bit and 15-bit output.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MDEC_MB_BYTES_24 768
#define MDEC_MB_BYTES_15 512

/*----------------------
 | MdecStream
 | Description: The run-length halfwords left in one MDEC input command and the output format chosen for it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    const uint16_t *src;
    const uint16_t *end;
    int depth24;
    int stp;
} MdecStream;

/*----------------------
 | mdec_init_tables
 | Description: Builds the PS1 default quant table in zigzag order and the IDCT scale table.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void mdec_init_tables(void);

/*----------------------
 | mdec_scale_table
 | Description: The 64-entry IDCT scale table mdec_init_tables built.
 | Author: suinevere
 | Dependencies: mdec_init_tables
 | Globals: N/A
 | Params: N/A
 | Returns: pointer to 64 signed entries
 ----------------------*/
const int16_t *mdec_scale_table(void);

/*----------------------
 | mdec_quant_table
 | Description: The 64-entry quant table in zigzag order mdec_init_tables built.
 | Author: suinevere
 | Dependencies: mdec_init_tables
 | Globals: N/A
 | Params: N/A
 | Returns: pointer to 64 entries
 ----------------------*/
const uint8_t *mdec_quant_table(void);

/*----------------------
 | mdec_begin
 | Description: Points a stream at the run-length halfwords after an MDEC decode command word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: s -- stream, command -- 0x3800xxxx word with xxxx data words following, depth24 -- 1 for 24-bit output, stp -- bit 15 of 15-bit pixels
 | Returns: N/A
 ----------------------*/
void mdec_begin(MdecStream *s, const uint32_t *command, int depth24, int stp);

/*----------------------
 | mdec_decode_macroblock
 | Description: Decodes Cr, Cb and four Y blocks into one row-major 16x16 macroblock.
 | Author: suinevere
 | Dependencies: mdec_init_tables
 | Globals: N/A
 | Params: s -- stream, out -- 768 or 512 bytes
 | Returns: 0, or -1 once the data runs out, leaving out black
 ----------------------*/
int mdec_decode_macroblock(MdecStream *s, uint8_t *out);

#endif
