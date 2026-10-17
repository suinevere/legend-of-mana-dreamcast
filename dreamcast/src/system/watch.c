/*----------------------
 | watch.c
 | Description: Change-printing watcher: polls a table of signed 32-bit values, prints each once on its first poll and again on every change, and rings the last 32 lines for marker checks.
 | Author: suinevere
 | Dependencies: watch.h, stdio.h
 | Globals: s_entries, s_count, s_printer, s_ring, s_ring_next, s_ring_filled
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "watch.h"

#include <stdio.h>

/* No <string.h>: its <strings.h> pulls a bcopy() prototype that conflicts with lom_prefix.h's PS1-shaped one on the Dreamcast cross toolchain. */

/*----------------------
 | WATCH_RING_SIZE
 | Description: Number of recent printed lines watch_seen can search.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define WATCH_RING_SIZE 32u

/*----------------------
 | WATCH_LINE_SIZE
 | Description: Byte size of one ring slot, including the NUL terminator.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define WATCH_LINE_SIZE 64u

/*----------------------
 | s_entries / s_count
 | Description: The watch table watch_init was given and its entry count.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static WatchEntry *s_entries;
static unsigned s_count;

/*----------------------
 | s_printer
 | Description: Line sink watch_init was given; receives each full printed line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void (*s_printer)(const char *);

/*----------------------
 | s_ring / s_ring_next / s_ring_filled
 | Description: Fixed ring of the last WATCH_RING_SIZE printed lines, the next write slot and the number of valid entries so far.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static char s_ring[WATCH_RING_SIZE][WATCH_LINE_SIZE];
static unsigned s_ring_next;
static unsigned s_ring_filled;

/*----------------------
 | ring_push
 | Description: Copies line into the ring's next slot, truncating to fit, and advances the ring.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_ring, s_ring_next, s_ring_filled
 | Params: line -- NUL-terminated line to remember
 | Returns: N/A
 ----------------------*/
static void ring_push(const char *line)
{
    char *slot = s_ring[s_ring_next];
    unsigned i = 0;

    while (line[i] != '\0' && i < WATCH_LINE_SIZE - 1u)
    {
        slot[i] = line[i];
        i++;
    }
    slot[i] = '\0';
    s_ring_next = (s_ring_next + 1u) % WATCH_RING_SIZE;
    if (s_ring_filled < WATCH_RING_SIZE)
    {
        s_ring_filled++;
    }
}

/*----------------------
 | watch_init
 | Description: Starts watching entries with printer as the line sink; resets every entry to unprinted and clears the recent-line ring.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_entries, s_count, s_printer, s_ring_next, s_ring_filled
 | Params: entries -- watch table, count -- number of entries, printer -- callback receiving each full "LOM: field <name> <value>" line
 | Returns: N/A
 ----------------------*/
void watch_init(WatchEntry *entries, unsigned count, void (*printer)(const char *))
{
    unsigned i;

    s_entries = entries;
    s_count = count;
    s_printer = printer;
    s_ring_next = 0;
    s_ring_filled = 0;
    for (i = 0; i < count; i++)
    {
        entries[i].primed = 0;
    }
}

/*----------------------
 | watch_poll
 | Description: Samples every watched entry, printing its line on the first poll after watch_init and again whenever its value changes.
 | Author: suinevere
 | Dependencies: ring_push
 | Globals: s_entries, s_count, s_printer
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void watch_poll(void)
{
    unsigned i;

    for (i = 0; i < s_count; i++)
    {
        WatchEntry *entry = &s_entries[i];
        int32_t value = *entry->value;

        if (!entry->primed || value != entry->last)
        {
            char line[WATCH_LINE_SIZE];

            snprintf(line, sizeof(line), "LOM: field %s %d", entry->name, (int)value);
            if (s_printer != NULL)
            {
                s_printer(line);
            }
            ring_push(line);
            entry->last = value;
            entry->primed = 1;
        }
    }
}

/*----------------------
 | contains
 | Description: Reports whether haystack holds needle as a substring, without relying on <string.h>'s strstr.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: haystack -- NUL-terminated line to search, needle -- NUL-terminated substring to find
 | Returns: 1 if haystack contains needle, else 0
 ----------------------*/
static int contains(const char *haystack, const char *needle)
{
    unsigned h;

    if (needle[0] == '\0')
    {
        return 1;
    }
    for (h = 0; haystack[h] != '\0'; h++)
    {
        unsigned n = 0;

        while (needle[n] != '\0' && haystack[h + n] == needle[n])
        {
            n++;
        }
        if (needle[n] == '\0')
        {
            return 1;
        }
    }
    return 0;
}

/*----------------------
 | watch_clear_seen
 | Description: Empties the ring of recently printed lines watch_seen searches.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_ring_next, s_ring_filled
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void watch_clear_seen(void)
{
    s_ring_next = 0;
    s_ring_filled = 0;
}

/*----------------------
 | watch_seen
 | Description: Reports whether any of the last 32 printed lines contains text as a substring.
 | Author: suinevere
 | Dependencies: contains
 | Globals: s_ring, s_ring_filled
 | Params: text -- substring to search recent printed lines for
 | Returns: 1 if a recent line contains text, else 0
 ----------------------*/
int watch_seen(const char *text)
{
    unsigned i;

    for (i = 0; i < s_ring_filled; i++)
    {
        if (contains(s_ring[i], text))
        {
            return 1;
        }
    }
    return 0;
}
