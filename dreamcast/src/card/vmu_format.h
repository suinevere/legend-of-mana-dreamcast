/*----------------------
 | vmu_format.h
 | Description: Pure conversions between PS1 memory-card files and VMU files: names, icons, block counts, the package CRC, the virtual PS1 free space and the VMU screen wordmark.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_VMU_FORMAT_H
#define LOM_VMU_FORMAT_H

/*----------------------
 | VMU_NAME_LEN / PS1_BLOCK_BYTES / PS1_CARD_BLOCKS / VMU_BLOCK_BYTES / LOM_PS1_PRODUCT
 | Description: VMU file-name length, PS1 card block size and count, VMU block size, and the product code every Legend of Mana save name starts with.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_NAME_LEN 12
#define PS1_BLOCK_BYTES 8192
#define PS1_CARD_BLOCKS 15
#define VMU_BLOCK_BYTES 512
#define LOM_PS1_PRODUCT "BASLUS-01013"

void vmu_name_for_ps1(const char *ps1_name, char out[VMU_NAME_LEN + 1]);
void vmu_ps1_name_from_desc(const char *desc, int desc_bytes, char out[21]);
int vmu_icon_from_ps1(const unsigned char *ps1_file, unsigned short palette[16], unsigned char frames[3 * 512]);
unsigned int vmu_blocks_for_file(unsigned int data_bytes, int icon_frames);
unsigned short vmu_pkg_crc(const unsigned char *buf, int size);
int ps1_blocks_free(int vmu_free_blocks, int ps1_used_blocks);
void vmu_lcd_wordmark(unsigned char out[192]);

#endif
