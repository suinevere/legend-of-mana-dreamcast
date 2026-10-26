/*----------------------
 | vmu_game.h
 | Description: Whether a VMU can take Ring Ring Land's GAME, decided from its FAT and directory alone.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_VMU_GAME_H
#define LOM_VMU_GAME_H

/*----------------------
 | VMU_GAME_FILE / VMU_GAME_PET_BLOCKS / VMU_GAME_FAT_FREE / VMU_GAME_FAT_END / VMU_GAME_TYPE
 | Description: The GAME's VMU file name, the blocks a pet package takes when it is not on the VMU yet, the FAT's free and chain-end marks, and a directory entry's GAME type.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_GAME_FILE "RINGRINGLAND"
#define VMU_GAME_PET_BLOCKS 5
#define VMU_GAME_FAT_FREE 0xFFFCu
#define VMU_GAME_FAT_END 0xFFFAu
#define VMU_GAME_TYPE 0xCC

/*----------------------
 | VMU_GAME_UNSUPPORTED / VMU_GAME_MISSING / VMU_GAME_OURS / VMU_GAME_OTHER / VMU_GAME_NO_ROOM
 | Description: What a VMU holds for the GAME: no store support, no GAME, Ring Ring Land, another minigame, or no room for Ring Ring Land and the pet.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum { VMU_GAME_UNSUPPORTED = -1, VMU_GAME_MISSING = 0, VMU_GAME_OURS = 1, VMU_GAME_OTHER = 2, VMU_GAME_NO_ROOM = 3 };

/*----------------------
 | vmu_game_dirent
 | Description: The parts of a VMU directory entry the classifier reads: NUL-terminated name, file type, first block and block count.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct vmu_game_dirent
{
    char name[13];
    unsigned char type;
    unsigned short first;
    unsigned short blocks;
} vmu_game_dirent;

int vmu_game_classify(const unsigned short *fat, int user_blocks, const vmu_game_dirent *dir, int count,
                      int game_blocks, int pet_blocks, int *found);

#endif
