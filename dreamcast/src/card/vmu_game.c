/*----------------------
 | vmu_game.c
 | Description: Classifies a VMU for Ring Ring Land's GAME: the BIOS runs a GAME from block 0 straight through flash, so blocks 0 to N-1 must be free or the replaced GAME's, with room left for the pet.
 | Author: suinevere
 | Dependencies: vmu_game.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "vmu_game.h"

/*----------------------
 | vmu_game_classify
 | Description: Finds the VMU's GAME entry and reports missing, ours or other when blocks 0 to game_blocks-1 are free or that GAME's and the free blocks cover the GAME and pet, else no room.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: fat -- user_blocks FAT words; user_blocks -- user block count (at most 256); dir, count -- the used directory entries; game_blocks -- the GAME's size in blocks; pet_blocks -- 0 when the pet is on the VMU, else its size; found -- receives the GAME entry's index, or -1
 | Returns: VMU_GAME_MISSING, VMU_GAME_OURS, VMU_GAME_OTHER or VMU_GAME_NO_ROOM
 ----------------------*/
int vmu_game_classify(const unsigned short *fat, int user_blocks, const vmu_game_dirent *dir, int count,
                      int game_blocks, int pet_blocks, int *found)
{
    static unsigned char held[256];
    int free_total = 0;
    int b;

    *found = -1;
    for (int i = 0; i < count; i++)
    {
        if (dir[i].type == VMU_GAME_TYPE)
        {
            *found = i;
            break;
        }
    }
    if (game_blocks > user_blocks || user_blocks > 256)
    {
        return VMU_GAME_NO_ROOM;
    }
    for (b = 0; b < 256; b++)
    {
        held[b] = 0;
    }
    if (*found >= 0)
    {
        for (b = dir[*found].first; b >= 0 && b < user_blocks && !held[b]; b = fat[b] == VMU_GAME_FAT_END ? -1 : fat[b])
        {
            held[b] = 1;
        }
    }
    for (b = 0; b < user_blocks; b++)
    {
        if (fat[b] == VMU_GAME_FAT_FREE || held[b])
        {
            free_total++;
        }
    }
    for (b = 0; b < game_blocks; b++)
    {
        if (fat[b] != VMU_GAME_FAT_FREE && !held[b])
        {
            return VMU_GAME_NO_ROOM;
        }
    }
    if (free_total < game_blocks + pet_blocks)
    {
        return VMU_GAME_NO_ROOM;
    }
    if (*found < 0)
    {
        return VMU_GAME_MISSING;
    }
    for (b = 0; b < 12; b++)
    {
        if (dir[*found].name[b] != VMU_GAME_FILE[b])
        {
            return VMU_GAME_OTHER;
        }
    }
    return VMU_GAME_OURS;
}
