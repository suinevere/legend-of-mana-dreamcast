/*----------------------
 | vmu_screen.h
 | Description: What every VMU's screen shows: the party pet, else the pet away in Ring Ring Land, else the Mana wordmark, redrawn only when that changes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_VMU_SCREEN_H
#define LOM_VMU_SCREEN_H

/*----------------------
 | VMU_SCREEN_BYTES / VMU_SCREEN_PET_BYTES / VMU_SCREEN_PETS / VMU_SCREEN_EGGS / VMU_SCREEN_WORDMARK
 | Description: A 48x32 screen's bytes, a PetRecord's bytes, the pet screens (by species) before the egg screens (by egg species), and the choice that shows the wordmark.
 | Author: suinevere
 | Dependencies: port/tools/vmu_portraits.py (PET_SCREENS, EGG_SCREENS)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_SCREEN_BYTES 192
#define VMU_SCREEN_PET_BYTES 0x60
#define VMU_SCREEN_PETS 65
#define VMU_SCREEN_EGGS 12
#define VMU_SCREEN_WORDMARK (-1)

/*----------------------
 | vmu_screen_draw_fn
 | Description: Draws a screen to every VMU; returns 0 once drawn, nonzero when the store is busy and the draw should be retried.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: lcd -- VMU_SCREEN_BYTES bytes, 4-byte aligned
 | Returns: 0 or busy
 ----------------------*/
typedef int (*vmu_screen_draw_fn)(const unsigned char *lcd);

void vmu_screen_set_draw(vmu_screen_draw_fn draw);
void vmu_screen_set_screens(const unsigned char (*screens)[VMU_SCREEN_BYTES], unsigned int count);
void vmu_screen_note_rrl(const unsigned char *pet);
int vmu_screen_choice(const unsigned char *party);
void vmu_screen_update(const unsigned char *party, int lcds);
const unsigned char *vmu_screen_party(void);

#endif
