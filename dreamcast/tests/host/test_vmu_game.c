/*----------------------
 | test_vmu_game.c
 | Description: Host test for vmu_game.c: a VMU classified as missing, holding ours, holding another minigame or without room for Ring Ring Land's GAME, from its FAT and directory.
 | Author: suinevere
 | Dependencies: vmu_game.c
 ----------------------*/
#include <string.h>
#include "vmu_game.h"

int printf(const char *format, ...);
int puts(const char *text);

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); return 1; } } while (0)

static unsigned short fat[200];

static void reset(void)
{
    for (int i = 0; i < 200; i++) fat[i] = VMU_GAME_FAT_FREE;
}

static void chain(int first, int n)
{
    for (int i = 0; i < n; i++) fat[first + i] = i + 1 < n ? (unsigned short)(first + i + 1) : VMU_GAME_FAT_END;
}

static void entry(vmu_game_dirent *d, const char *name, unsigned char type, int first, int blocks)
{
    memset(d, 0, sizeof *d);
    strcpy(d->name, name);
    d->type = type;
    d->first = (unsigned short)first;
    d->blocks = (unsigned short)blocks;
}

int main(void)
{
    vmu_game_dirent dir[2];
    int found;

    reset();
    CHECK(vmu_game_classify(fat, 200, dir, 0, 121, 5, &found) == VMU_GAME_MISSING && found == -1);

    reset();
    chain(60, 10);
    entry(&dir[0], "BASLUSP01013", 0x33, 60, 10);
    CHECK(vmu_game_classify(fat, 200, dir, 1, 121, 0, &found) == VMU_GAME_NO_ROOM);

    reset();
    chain(0, 50);
    entry(&dir[0], "TETRIS", VMU_GAME_TYPE, 0, 50);
    CHECK(vmu_game_classify(fat, 200, dir, 1, 121, 5, &found) == VMU_GAME_OTHER && found == 0);

    reset();
    chain(0, 121);
    entry(&dir[0], "RINGRINGLAND", VMU_GAME_TYPE, 0, 121);
    CHECK(vmu_game_classify(fat, 200, dir, 1, 121, 5, &found) == VMU_GAME_OURS && found == 0);

    reset();
    chain(121, 75);
    entry(&dir[0], "FILLER", 0x33, 121, 75);
    CHECK(vmu_game_classify(fat, 200, dir, 1, 121, 5, &found) == VMU_GAME_NO_ROOM);
    CHECK(vmu_game_classify(fat, 200, dir, 1, 121, 4, &found) == VMU_GAME_MISSING);

    reset();
    CHECK(vmu_game_classify(fat, 200, dir, 0, 201, 0, &found) == VMU_GAME_NO_ROOM);

    puts("vmu_game ok");
    return 0;
}
