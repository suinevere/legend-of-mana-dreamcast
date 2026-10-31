/*----------------------
 | heap_probe.c
 | Description: Heap checkpoints for LOM_HEAP_PROBE builds; the exception report names the checkpoint that was walking the heap, if any, and the last one that passed.
 | Author: suinevere
 | Dependencies: heap_probe.h, malloc.h, stdio.h
 | Globals: g_heap_probe_tag, g_heap_probe_passed, g_heap_probe_inside
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "heap_probe.h"

#ifdef LOM_HEAP_PROBE

#include <malloc.h>
#include <stdio.h>

/*----------------------
 | g_heap_probe_tag / g_heap_probe_passed / g_heap_probe_inside
 | Description: The checkpoint being walked, the last one whose walk finished, and whether a walk is under way.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const char *volatile g_heap_probe_tag = "none";
static const char *volatile g_heap_probe_passed = "none";
static volatile int g_heap_probe_inside;

void heap_probe(const char *tag)
{
    volatile struct mallinfo info;

    g_heap_probe_tag = tag;
    g_heap_probe_inside = 1;
    info = mallinfo();
    (void)info;
    g_heap_probe_inside = 0;
    g_heap_probe_passed = tag;
}

const char *heap_probe_last(void)
{
    static char text[96];

    if (g_heap_probe_inside)
    {
        snprintf(text, sizeof text, "HEAP BAD at %s, ok at %s", g_heap_probe_tag, g_heap_probe_passed);
    }
    else
    {
        snprintf(text, sizeof text, "HEAP ok at %s", g_heap_probe_passed);
    }
    return text;
}

#endif
