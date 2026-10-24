/*----------------------
 | save_pack.h
 | Description: LZSS compression for PS1 card files stored on a VMU, so a 16 KB save takes a handful of blocks instead of 32.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_SAVE_PACK_H
#define LOM_SAVE_PACK_H

unsigned int save_pack_bound(unsigned int size);
int save_pack(const unsigned char *in, unsigned int size, unsigned char *out, unsigned int cap);
int save_unpack(const unsigned char *in, unsigned int size, unsigned char *out, unsigned int out_size);

#endif
