/*----------------------
 | vmu_screen.c
 | Description: Chooses what every VMU's screen shows from the party pet and the pet last seen in Ring Ring Land's file, and draws it through the store's hook only when the choice or the number of screens changes.
 | Author: suinevere
 | Dependencies: vmu_screen.h, vmu_format.h, string.h
 | Globals: s_draw, s_screens, s_screen_count, s_rrl, s_rrl_valid, s_wordmark, s_shown, s_lcds
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <string.h>
#include "vmu_screen.h"
#include "vmu_format.h"

/*----------------------
 | PET_SPECIES / PET_EGG_SPECIES / PET_STATUS_HIGH / PET_STATUS_EGG_BIT / SHOWN_NONE
 | Description: A PetRecord's species, egg species and the status byte holding its egg flag (bit 31 of the word at 0x44), and the shown choice before anything is drawn.
 | Author: suinevere
 | Dependencies: include/common/saved_game.h (PetRecord, PET_STATUS_EGG)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PET_SPECIES 0x15
#define PET_EGG_SPECIES 0x16
#define PET_STATUS_HIGH 0x47
#define PET_STATUS_EGG_BIT 0x80
#define SHOWN_NONE (-2)

/*----------------------
 | s_draw / s_screens / s_screen_count
 | Description: The store's draw hook and the generated pet and egg screens with how many there are.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static vmu_screen_draw_fn s_draw;
static const unsigned char (*s_screens)[VMU_SCREEN_BYTES];
static unsigned int s_screen_count;

/*----------------------
 | s_rrl / s_rrl_valid
 | Description: A copy of the pet last read from or written to Ring Ring Land's file, and whether one is held.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_rrl[VMU_SCREEN_PET_BYTES];
static int s_rrl_valid;

/*----------------------
 | s_wordmark / s_shown / s_lcds
 | Description: The wordmark screen, built on first use and 4-byte aligned for the draw; the choice on the screens now and how many screens it went to.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_wordmark[VMU_SCREEN_BYTES] __attribute__((aligned(4)));
static int s_shown = SHOWN_NONE;
static int s_lcds;

/*----------------------
 | vmu_screen_set_draw
 | Description: Installs the hook that draws a screen to every VMU.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_draw
 | Params: draw -- the hook
 | Returns: N/A
 ----------------------*/
void vmu_screen_set_draw(vmu_screen_draw_fn draw)
{
    s_draw = draw;
}

/*----------------------
 | vmu_screen_set_screens
 | Description: Installs the pet and egg screens; a choice past count shows the wordmark.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_screens, s_screen_count
 | Params: screens -- the screens; count -- how many
 | Returns: N/A
 ----------------------*/
void vmu_screen_set_screens(const unsigned char (*screens)[VMU_SCREEN_BYTES], unsigned int count)
{
    s_screens = screens;
    s_screen_count = count;
}

/*----------------------
 | vmu_screen_note_rrl
 | Description: Records the pet Ring Ring Land's file holds, or that it holds none.
 | Author: suinevere
 | Dependencies: memcpy
 | Globals: s_rrl, s_rrl_valid
 | Params: pet -- VMU_SCREEN_PET_BYTES of PetRecord, or NULL
 | Returns: N/A
 ----------------------*/
void vmu_screen_note_rrl(const unsigned char *pet)
{
    s_rrl_valid = pet != NULL && pet[0] != 0;
    if (s_rrl_valid)
    {
        memcpy(s_rrl, pet, sizeof s_rrl);
    }
}

/*----------------------
 | pet_screen
 | Description: A pet's screen: its egg's while it is an egg, else its species'; none for an empty record or one past the screens installed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_screen_count
 | Params: pet -- PetRecord bytes, or NULL
 | Returns: the screen index, or VMU_SCREEN_WORDMARK
 ----------------------*/
static int pet_screen(const unsigned char *pet)
{
    int index;

    if (pet == NULL || pet[0] == 0)
    {
        return VMU_SCREEN_WORDMARK;
    }
    if (pet[PET_STATUS_HIGH] & PET_STATUS_EGG_BIT)
    {
        index = pet[PET_EGG_SPECIES] < VMU_SCREEN_EGGS ? VMU_SCREEN_PETS + pet[PET_EGG_SPECIES] : VMU_SCREEN_WORDMARK;
    }
    else
    {
        index = pet[PET_SPECIES] < VMU_SCREEN_PETS ? pet[PET_SPECIES] : VMU_SCREEN_WORDMARK;
    }
    return index >= 0 && (unsigned int)index < s_screen_count ? index : VMU_SCREEN_WORDMARK;
}

/*----------------------
 | vmu_screen_choice
 | Description: The screen to show: the party pet's, else the Ring Ring Land pet's, else the wordmark.
 | Author: suinevere
 | Dependencies: pet_screen
 | Globals: s_rrl, s_rrl_valid
 | Params: party -- the party pet's PetRecord bytes, or NULL
 | Returns: a screen index or VMU_SCREEN_WORDMARK
 ----------------------*/
int vmu_screen_choice(const unsigned char *party)
{
    int index = pet_screen(party);

    if (index == VMU_SCREEN_WORDMARK && s_rrl_valid)
    {
        index = pet_screen(s_rrl);
    }
    return index;
}

/*----------------------
 | vmu_screen_update
 | Description: Draws the choice when it or the number of screens changed since the last draw; with no screens it only notes that, and a busy store is retried on the next call.
 | Author: suinevere
 | Dependencies: vmu_screen_choice, vmu_lcd_wordmark
 | Globals: s_draw, s_screens, s_wordmark, s_shown, s_lcds
 | Params: party -- the party pet's PetRecord bytes, or NULL; lcds -- VMU screens plugged in
 | Returns: N/A
 ----------------------*/
void vmu_screen_update(const unsigned char *party, int lcds)
{
    int index = vmu_screen_choice(party);
    const unsigned char *lcd;

    if (lcds <= 0)
    {
        s_lcds = 0;
        return;
    }
    if (s_draw == NULL || (index == s_shown && lcds == s_lcds))
    {
        return;
    }
    if (index == VMU_SCREEN_WORDMARK)
    {
        vmu_lcd_wordmark(s_wordmark);
        lcd = s_wordmark;
    }
    else
    {
        lcd = s_screens[index];
    }
    if (s_draw(lcd) == 0)
    {
        s_shown = index;
        s_lcds = lcds;
    }
}
