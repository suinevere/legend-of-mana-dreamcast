/*----------------------
 | fb.c
 | Description: Ring Ring Land's 32x32 area of the VMU frame buffer, the bit reversal table for the original's rows, and the 128-byte state block read from the pet save at start.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: the VMU frame buffer
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | REV2 / REV4 / REV6
 | Description: Build game_bit_rev four entries, sixteen and sixty-four at a time from the bit pairs already placed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: n -- the entries' reversed low bits
 | Returns: N/A
 ----------------------*/
#define REV2(n) (n), (n) + 2 * 64, (n) + 1 * 64, (n) + 3 * 64
#define REV4(n) REV2(n), REV2((n) + 2 * 16), REV2((n) + 1 * 16), REV2((n) + 3 * 16)
#define REV6(n) REV4(n), REV4((n) + 2 * 4), REV4((n) + 1 * 4), REV4((n) + 3 * 4)

/*----------------------
 | game_bit_rev
 | Description: Every byte with its bits end for end: the original's rows keep the leftmost pixel in bit 0, the VMU's in bit 7.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
const uint8_t game_bit_rev[256] = {REV6(0), REV6(2), REV6(1), REV6(3)};

/*----------------------
 | game_fb_put_rows
 | Description: Copies count row words from src into the frame buffer from row on, inverted when asked.
 | Author: suinevere
 | Dependencies: set_fb_row, rev32
 | Globals: the VMU frame buffer
 | Params: row -- first row; count -- rows; src -- source address; invert -- nonzero to invert
 | Returns: N/A
 ----------------------*/
void game_fb_put_rows(uint8_t row, uint8_t count, uint32_t src, uint8_t invert) {
    do {
        uint32_t w = rev32(rom32(src));
        set_fb_row(row, invert ? ~w : w);
        src += 4u;
        row++;
    } while (--count);
}

/*----------------------
 | game_fb_present
 | Description: Where the original copied its frame buffer to the LCD; the game draws straight into the VMU frame buffer and game_wait_ticks marks the frame, so nothing is left to do.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_fb_present(void) {
}

/*----------------------
 | game_fb_clear
 | Description: Clears the game's 32x32 area of the frame buffer.
 | Author: suinevere
 | Dependencies: set_fb_row
 | Globals: the VMU frame buffer
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_fb_clear(void) {
    for (uint8_t row = 0; row < FB_ROWS; row++) {
        set_fb_row(row, 0);
    }
}

/*----------------------
 | game_state_reset
 | Description: Reads the 128-byte state block at 0x4C4 from the pet save's SAVE_SECTOR, where the original copied its flash at 0x02000300, retrying until it succeeds.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: the block 0x4C4-0x543
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_state_reset(void) {
    while (vmu_save_read(SAVE_SECTOR, STATE_BLOCK) != 0) {
    }
}
