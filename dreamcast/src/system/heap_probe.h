/*----------------------
 | heap_probe.h
 | Description: Under LOM_HEAP_PROBE, walks the heap's free lists at named checkpoints so a corrupted chunk faults right after the code that damaged it; otherwise compiles to nothing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef HEAP_PROBE_H
#define HEAP_PROBE_H

#ifdef LOM_HEAP_PROBE

/*----------------------
 | heap_probe
 | Description: Records the checkpoint's tag, then walks the heap through mallinfo, which faults on a free chunk whose links were overwritten.
 | Author: suinevere
 | Dependencies: mallinfo
 | Globals: g_heap_probe_tag, g_heap_probe_passed, g_heap_probe_inside
 | Params: tag -- static string naming the checkpoint
 | Returns: N/A
 ----------------------*/
void heap_probe(const char *tag);

/*----------------------
 | heap_probe_last
 | Description: A line for the exception report: the checkpoint whose walk faulted and the last that passed, or just the last that passed.
 | Author: suinevere
 | Dependencies: snprintf
 | Globals: g_heap_probe_tag, g_heap_probe_passed, g_heap_probe_inside
 | Params: N/A
 | Returns: static text
 ----------------------*/
const char *heap_probe_last(void);

#else

#define heap_probe(tag) ((void)0)

#endif

#endif
