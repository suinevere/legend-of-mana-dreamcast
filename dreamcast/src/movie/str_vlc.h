/*----------------------
 | str_vlc.h
 | Description: Version-2 STR bitstream decoder writing MDEC run-length codes exactly as libpress DecDCTvlc2 lays them out.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_STR_VLC_H
#define LOM_STR_VLC_H

#include <stdint.h>

/*----------------------
 | str_vlc_init
 | Description: Builds the 16-bit-indexed lookup of the AC coefficient codes, EOB and escape.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void str_vlc_init(void);

/*----------------------
 | str_vlc_decode_frame
 | Description: Decodes one v2 frame into an MDEC command word (0x3800 << 16 | data words) and its run-length halfwords, padded with EOB.
 | Author: suinevere
 | Dependencies: str_vlc_init
 | Globals: N/A
 | Params: bitstream -- frame header then bits, bitstream_halfwords -- its length, dst -- output, dst_words -- output capacity
 | Returns: 0, or -1 on a bad header, an invalid code, or too small a dst, with the written part padded by EOB
 ----------------------*/
int str_vlc_decode_frame(const uint16_t *bitstream, unsigned bitstream_halfwords, uint32_t *dst, unsigned dst_words);

#endif
