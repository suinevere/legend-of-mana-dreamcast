/*----------------------
 | vmu_screen_party.c
 | Description: The party pet's record for vmu_screen, kept apart from the KallistiOS files because the game's headers clash with kos.h.
 | Author: suinevere
 | Dependencies: common/saved_game.h, vmu_screen.h
 | Globals: g_saved_game
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "common/saved_game.h"
#include "vmu_screen.h"

/*----------------------
 | vmu_screen_party
 | Description: The PetRecord of the pet in the party, or NULL when none has joined.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_saved_game
 | Params: N/A
 | Returns: the record's bytes, or NULL
 ----------------------*/
const unsigned char *vmu_screen_party(void)
{
    s32 slot = g_saved_game.layout.joined_pet;

    return slot >= 0 && slot < PET_RECORD_COUNT ? (const unsigned char *)&g_saved_game.layout.pets[slot] : NULL;
}
