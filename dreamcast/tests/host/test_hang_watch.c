/*----------------------
 | test_hang_watch.c
 | Description: Host test for the hang check: overdue only once more than the limit of vblanks passed since the last VSync, across counter wraparound, never before the first VSync, and its halt text names both threads' PC and PR.
 | Author: suinevere
 | Dependencies: hang_watch.h, stdio.h, string.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>
#include "hang_watch.h"

static int s_failures;

#define CHECK(cond)                                                                                                                   \
    do                                                                                                                                \
    {                                                                                                                                 \
        if (!(cond))                                                                                                                  \
        {                                                                                                                             \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);                                                                     \
            s_failures++;                                                                                                             \
        }                                                                                                                             \
    } while (0)

int main(void)
{
    char text[160];

    CHECK(!hang_watch_overdue(1000, 100, 900, 1));
    CHECK(hang_watch_overdue(1001, 100, 900, 1));
    CHECK(hang_watch_overdue(5, 0xFFFFFF00u, 200, 1));
    CHECK(!hang_watch_overdue(5, 0xFFFFFF00u, 300, 1));
    CHECK(!hang_watch_overdue(100000, 100, 900, 0));
    CHECK(!hang_watch_overdue(100000, 100, 0, 1));
    hang_watch_format(text, sizeof text, "mdec", 0x8C0123A4ul, 0x8C0456B8ul, 0x8C0789CCul, 0x8C0ABCD0ul, 900);
    CHECK(strstr(text, "HANG 900 VBL") != NULL);
    CHECK(strstr(text, "mdec PC=8C0123A4 PR=8C0456B8") != NULL);
    CHECK(strstr(text, "MAIN PC=8C0789CC PR=8C0ABCD0") != NULL);
    if (s_failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
