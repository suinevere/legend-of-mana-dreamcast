/*----------------------
 | test_vmu_screen.c
 | Description: Host test of vmu_screen.c: the party pet's screen (its egg's while an egg), else the Ring Ring Land pet's, else the wordmark, drawn only when the choice or the VMU count changes and retried while the store is busy.
 | Author: suinevere
 | Dependencies: vmu_screen.h, vmu_format.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>
#include "vmu_screen.h"
#include "vmu_format.h"

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); return 1; } } while (0)

static unsigned char s_screens[77][192];
static const unsigned char *s_drawn;
static int s_draws;
static int s_busy;

static int fake_draw(const unsigned char *lcd)
{
    if (s_busy)
    {
        return 1;
    }
    s_drawn = lcd;
    s_draws++;
    return 0;
}

static void pet(unsigned char *rec, int species, int egg_species, int egg)
{
    memset(rec, 0, VMU_SCREEN_PET_BYTES);
    rec[0] = 'A';
    rec[0x15] = (unsigned char)species;
    rec[0x16] = (unsigned char)egg_species;
    rec[0x47] = egg ? 0x80 : 0;
}

int main(void)
{
    unsigned char party[VMU_SCREEN_PET_BYTES], away[VMU_SCREEN_PET_BYTES], word[192];
    int k;

    for (k = 0; k < 77; k++)
    {
        memset(s_screens[k], k, sizeof s_screens[k]);
    }
    vmu_lcd_wordmark(word);
    CHECK(vmu_screen_choice(NULL) == VMU_SCREEN_WORDMARK);

    vmu_screen_set_screens(s_screens, 77);
    vmu_screen_set_draw(fake_draw);
    pet(party, 12, 3, 0);
    CHECK(vmu_screen_choice(party) == 12);
    pet(party, 12, 3, 1);
    CHECK(vmu_screen_choice(party) == 65 + 3);
    pet(party, 99, 3, 0);
    CHECK(vmu_screen_choice(party) == VMU_SCREEN_WORDMARK);
    party[0] = 0;
    CHECK(vmu_screen_choice(party) == VMU_SCREEN_WORDMARK);

    pet(away, 40, 0, 0);
    vmu_screen_note_rrl(away);
    away[0x15] = 41;
    CHECK(vmu_screen_choice(NULL) == 40);
    pet(party, 7, 0, 0);
    CHECK(vmu_screen_choice(party) == 7);
    vmu_screen_note_rrl(NULL);
    CHECK(vmu_screen_choice(NULL) == VMU_SCREEN_WORDMARK);

    vmu_screen_update(NULL, 1);
    CHECK(s_draws == 1 && memcmp(s_drawn, word, 192) == 0);
    vmu_screen_update(NULL, 1);
    CHECK(s_draws == 1);
    vmu_screen_update(party, 1);
    CHECK(s_draws == 2 && s_drawn == s_screens[7]);
    vmu_screen_update(party, 2);
    CHECK(s_draws == 3 && s_drawn == s_screens[7]);
    vmu_screen_update(party, 0);
    CHECK(s_draws == 3);
    s_busy = 1;
    vmu_screen_update(party, 1);
    CHECK(s_draws == 3);
    s_busy = 0;
    vmu_screen_update(party, 1);
    CHECK(s_draws == 4 && s_drawn == s_screens[7]);

    vmu_screen_set_screens(s_screens, 10);
    vmu_screen_update(party, 1);
    CHECK(s_draws == 4);
    pet(party, 30, 0, 0);
    vmu_screen_update(party, 1);
    CHECK(s_draws == 5 && memcmp(s_drawn, word, 192) == 0);

    printf("OK\n");
    return 0;
}
