/*----------------------
 | gpu_census.c
 | Description: Tallies GP0 command codes and notable primitive traits the packet decoder observes, and prints a one-line summary via dc_log.
 | Author: suinevere
 | Dependencies: gpu_census.h, dc_system.c (dc_log)
 | Globals: s_code_count, s_flag_count, s_unhandled_seen
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>

#include "gpu_census.h"

/*----------------------
 | dc_log
 | Description: Declared locally so this file builds without KallistiOS headers; a host test supplies its own definition.
 | Author: suinevere
 | Dependencies: dc_system.c (dc_log)
 | Globals: N/A
 | Params: format -- printf-style format string, ... -- format arguments
 | Returns: N/A
 ----------------------*/
extern void dc_log(const char *format, ...);

/*----------------------
 | GPU_CENSUS_CODES
 | Description: Number of distinct GP0 command codes the census can tally, one per byte value.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GPU_CENSUS_CODES 256

/*----------------------
 | GPU_CENSUS_FLAGS
 | Description: Number of distinct trait flags the census tallies.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GPU_CENSUS_FLAGS 4

/*----------------------
 | s_code_count
 | Description: Sightings of each GP0 command code, indexed by code byte.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_code_count[GPU_CENSUS_CODES];

/*----------------------
 | s_flag_count
 | Description: Sightings of each trait flag, indexed by GpuCensusFlag.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_flag_count[GPU_CENSUS_FLAGS];

/*----------------------
 | s_unhandled_seen
 | Description: Whether gpu_census_unhandled has already logged a given code.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_unhandled_seen[GPU_CENSUS_CODES];

/*----------------------
 | s_flag_names
 | Description: Short names for each GpuCensusFlag, used by gpu_census_report.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const char *const s_flag_names[GPU_CENSUS_FLAGS] = { "subtract", "brighten", "tex_window", "dither" };

/*----------------------
 | gpu_census_note
 | Description: Counts one more sighting of a GP0 command code.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_code_count
 | Params: code -- GP0 command byte
 | Returns: N/A
 ----------------------*/
void gpu_census_note(unsigned code)
{
    s_code_count[code & 0xFFu]++;
}

/*----------------------
 | gpu_census_unhandled
 | Description: Logs "LOM GPU: unhandled 0xNN" for a code the decoder could not interpret, once per code.
 | Author: suinevere
 | Dependencies: dc_log
 | Globals: s_unhandled_seen
 | Params: code -- GP0 command byte
 | Returns: N/A
 ----------------------*/
void gpu_census_unhandled(unsigned code)
{
    unsigned c = code & 0xFFu;

    if (!s_unhandled_seen[c])
    {
        s_unhandled_seen[c] = 1;
        dc_log("GPU: unhandled 0x%02X", c);
    }
}

/*----------------------
 | gpu_census_flag
 | Description: Counts one more sighting of a notable primitive trait.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_flag_count
 | Params: flag -- trait observed
 | Returns: N/A
 ----------------------*/
void gpu_census_flag(GpuCensusFlag flag)
{
    s_flag_count[flag]++;
}

/*----------------------
 | gpu_census_count
 | Description: Returns how many times a GP0 command code has been noted.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_code_count
 | Params: code -- GP0 command byte
 | Returns: sightings of code
 ----------------------*/
unsigned gpu_census_count(unsigned code)
{
    return s_code_count[code & 0xFFu];
}

/*----------------------
 | gpu_census_flag_count
 | Description: Returns how many times a notable primitive trait has been flagged.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_flag_count
 | Params: flag -- trait to read
 | Returns: sightings of flag
 ----------------------*/
unsigned gpu_census_flag_count(GpuCensusFlag flag)
{
    return s_flag_count[flag];
}

/*----------------------
 | gpu_census_report
 | Description: Logs one "LOM: census ..." line listing every non-zero command code count and trait flag seen so far.
 | Author: suinevere
 | Dependencies: dc_log
 | Globals: s_code_count, s_flag_count
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void gpu_census_report(void)
{
    char line[512];
    size_t pos = 0;
    int i;

    line[0] = '\0';
    for (i = 0; i < GPU_CENSUS_CODES; i++)
    {
        if (s_code_count[i] != 0 && pos < sizeof(line) - 32)
        {
            int written = snprintf(line + pos, sizeof(line) - pos, "0x%02X=%u ", (unsigned)i, s_code_count[i]);

            pos += (written > 0) ? (size_t)written : 0;
        }
    }
    for (i = 0; i < GPU_CENSUS_FLAGS; i++)
    {
        if (s_flag_count[i] != 0 && pos < sizeof(line) - 32)
        {
            int written = snprintf(line + pos, sizeof(line) - pos, "%s=%u ", s_flag_names[i], s_flag_count[i]);

            pos += (written > 0) ? (size_t)written : 0;
        }
    }
    dc_log("census %s", line);
}
