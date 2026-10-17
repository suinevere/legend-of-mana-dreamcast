/*----------------------
 | test_vmu_format.c
 | Description: Host test for vmu_format.c: VMU names for PS1 files, icon conversion, block math, the virtual PS1 free-block count and the LCD wordmark.
 | Author: suinevere
 | Dependencies: vmu_format.c
 ----------------------*/
#include <string.h>
#include "vmu_format.h"

int printf(const char *format, ...);
int puts(const char *text);

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); return 1; } } while (0)

static int lcd_bit(const unsigned char *lcd, int x, int y)
{
    return (lcd[y * 6 + x / 8] >> (7 - x % 8)) & 1;
}

int main(void)
{
    char name[VMU_NAME_LEN + 1];
    unsigned char ps1[0x200];
    unsigned short pal[16];
    unsigned char frames[3 * 512];
    unsigned char lcd[192];
    int x;
    int y;
    int set = 0;
    int min_x = 48;
    int max_x = -1;

    vmu_name_for_ps1("BASLUS-01013LOM00", name);
    CHECK(strcmp(name, "LOMLOM00") == 0);
    vmu_name_for_ps1("BASLUS-01013dummy", name);
    CHECK(strcmp(name, "LOMDUMMY") == 0);
    vmu_name_for_ps1("BASLUS-01013ABCDEFGHIJK", name);
    CHECK(strcmp(name, "LOMABCDEFGHI") == 0);
    vmu_name_for_ps1("BESLES-00000ABCDEFGHIJK", name);
    CHECK(strncmp(name, "LOMX", 4) == 0 && strlen(name) == 12);

    memset(ps1, 0, sizeof ps1);
    CHECK(vmu_icon_from_ps1(ps1, pal, frames) == 0);
    ps1[0] = 'S';
    ps1[1] = 'C';
    ps1[2] = 0x12;
    ps1[0x60 + 2] = 0x1F;
    ps1[0x60 + 3] = 0x00;
    ps1[0x80] = 0x01;
    CHECK(vmu_icon_from_ps1(ps1, pal, frames) == 2);
    CHECK(pal[0] == 0x0000);
    CHECK(pal[1] == 0xFF00);
    CHECK((frames[0] >> 4) == 1 && (frames[0] & 0xF) == 1 && (frames[16] >> 4) == 1);
    CHECK((frames[1] >> 4) == 0);

    CHECK(vmu_blocks_for_file(0x4000, 1) == 34);
    CHECK(vmu_blocks_for_file(0x4000, 3) == 36);
    CHECK(ps1_blocks_free(200, 0) == 12);
    CHECK(ps1_blocks_free(200, 4) == 11);
    CHECK(ps1_blocks_free(3, 0) == 0);
    CHECK(ps1_blocks_free(255, 14) == 1);

    vmu_lcd_wordmark(lcd);
    for (y = 0; y < 32; y++)
    {
        for (x = 0; x < 48; x++)
        {
            if (lcd_bit(lcd, x, y))
            {
                set++;
                min_x = x < min_x ? x : min_x;
                max_x = x > max_x ? x : max_x;
                CHECK(x >= 2 && x < 46 && y >= 9 && y < 23);
            }
        }
    }
    CHECK(set > 0 && min_x == 2 && max_x == 44);
    for (y = 0; y < 32; y++)
    {
        for (x = 0; x < 10; x++)
        {
            CHECK(lcd_bit(lcd, 13 + x, y) == lcd_bit(lcd, 35 + x, y));
        }
    }
    puts("OK");
    return 0;
}
