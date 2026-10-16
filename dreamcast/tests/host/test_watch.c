/*----------------------
 | test_watch.c
 | Description: Host test for the watcher: first-poll and change printing, and watch_seen's substring search over recently printed lines.
 | Author: suinevere
 | Dependencies: watch.h, watch.c
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "watch.h"

static int32_t a = 0;
static char seen[8][64];
static int n;

static void capture(const char *s)
{
    strncpy(seen[n++], s, 63);
}

int main(void)
{
    WatchEntry e[1] = { { "scene", &a, 0, 0 } };

    watch_init(e, 1, capture);
    watch_poll();
    a = 857;
    watch_poll();
    watch_poll();
    if (n != 2 || strcmp(seen[0], "LOM: field scene 0") || strcmp(seen[1], "LOM: field scene 857"))
    {
        return 1;
    }
    if (!watch_seen("scene 857") || watch_seen("scene 16"))
    {
        return 2;
    }
    puts("OK");
    return 0;
}
