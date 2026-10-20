/*----------------------
 | test_pet_transfer.c
 | Description: Host harness for card_prepare_pet_transfer: sets a fixed land state, runs the hook on a resource file with a given pet, and writes the result.
 | Author: suinevere
 | Dependencies: card_callbacks.c, saved_game.h
 ----------------------*/
#include <stdio.h>
#include "main/card_callbacks.h"
#include "common/saved_game.h"

SavedGame g_saved_game;

/*----------------------
 | PET_TRANSFER_RESOURCE_MAX
 | Description: Largest resource the harness reads, the six-block Ring Ring Land file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PET_TRANSFER_RESOURCE_MAX 0xC000

/*----------------------
 | set_land_state
 | Description: Places lands 1-3, 5, 6 (through record 32), 7, 9, 15, 24 (through record 33) and 31, and maxes some mana levels on 9, 10 and 31.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_saved_game
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void set_land_state(void)
{
    static const int placed[] = {1, 2, 3, 5, 7, 9, 15, 31};
    unsigned int i;

    for (i = 0; i < sizeof(placed) / sizeof(placed[0]); i++)
    {
        g_saved_game.layout.lands[placed[i]].flags = FIELD_LAND_FLAG_02;
    }
    g_saved_game.layout.lands[32].flags = FIELD_LAND_FLAG_02;
    g_saved_game.layout.lands[33].flags = FIELD_LAND_FLAG_04;
    g_saved_game.layout.lands[9].levels[0] = 6;
    g_saved_game.layout.lands[9].levels[2] = 6;
    g_saved_game.layout.lands[9].levels[7] = 6;
    g_saved_game.layout.lands[10].levels[1] = 6;
    for (i = 0; i < 8; i++)
    {
        g_saved_game.layout.lands[31].levels[i] = 6;
    }
}

/*----------------------
 | main
 | Description: Reads the resource and the 0x60-byte pet, runs the hook, and writes the resource out.
 | Author: suinevere
 | Dependencies: set_land_state, card_prepare_pet_transfer
 | Globals: g_saved_game
 | Params: argv -- resource path, pet path, output path
 | Returns: 0 on success, 1 on an I/O error
 ----------------------*/
int main(int argc, char **argv)
{
    static u8 resource[PET_TRANSFER_RESOURCE_MAX];
    PetRecord pet;
    FILE *file;
    size_t size;

    if (argc != 4 || (file = fopen(argv[1], "rb")) == NULL)
    {
        return 1;
    }
    size = fread(resource, 1, sizeof(resource), file);
    fclose(file);
    if ((file = fopen(argv[2], "rb")) == NULL || fread(&pet, 1, sizeof(pet), file) != sizeof(pet))
    {
        return 1;
    }
    fclose(file);
    set_land_state();
    card_prepare_pet_transfer(resource, &pet);
    if ((file = fopen(argv[3], "wb")) == NULL)
    {
        return 1;
    }
    fwrite(resource, 1, size, file);
    fclose(file);
    return 0;
}
