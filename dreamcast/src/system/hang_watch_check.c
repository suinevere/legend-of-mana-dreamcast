/*----------------------
 | hang_watch_check.c
 | Description: The hang check's decision and halt text, free of KallistiOS so the host can test them.
 | Author: suinevere
 | Dependencies: hang_watch.h, stdio.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>

#include "hang_watch.h"

int hang_watch_overdue(unsigned now, unsigned fed, unsigned limit, int armed)
{
    return armed && limit != 0 && now - fed > limit;
}

char *hang_watch_format(char *out, int out_size, const char *label, unsigned long pc, unsigned long pr, unsigned long main_pc, unsigned long main_pr,
                        unsigned vblanks)
{
    snprintf(out, (size_t)out_size, "HANG %u VBL\n%s PC=%08lX PR=%08lX\nMAIN PC=%08lX PR=%08lX", vblanks, label, pc, pr, main_pc, main_pr);
    return out;
}
