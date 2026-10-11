/*----------------------
 | gpu_census.h
 | Description: Tallies GP0 command codes and notable primitive traits the packet decoder observes, for a one-line summary report.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_GPU_CENSUS_H
#define LOM_GPU_CENSUS_H

/*----------------------
 | GpuCensusFlag
 | Description: Notable primitive traits the census tracks separately from per-code counts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef enum
{
    CENSUS_SUBTRACT = 0,
    CENSUS_BRIGHTEN = 1,
    CENSUS_TEX_WINDOW = 2,
    CENSUS_DITHER = 3
} GpuCensusFlag;

/*----------------------
 | gpu_census_note
 | Description: Counts one more sighting of a GP0 command code.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: code -- GP0 command byte
 | Returns: N/A
 ----------------------*/
void gpu_census_note(unsigned code);

/*----------------------
 | gpu_census_unhandled
 | Description: Logs a code the decoder could not interpret, once per code.
 | Author: suinevere
 | Dependencies: dc_log
 | Globals: N/A
 | Params: code -- GP0 command byte
 | Returns: N/A
 ----------------------*/
void gpu_census_unhandled(unsigned code);

/*----------------------
 | gpu_census_flag
 | Description: Counts one more sighting of a notable primitive trait.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: flag -- trait observed
 | Returns: N/A
 ----------------------*/
void gpu_census_flag(GpuCensusFlag flag);

/*----------------------
 | gpu_census_count
 | Description: Returns how many times a GP0 command code has been noted.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: code -- GP0 command byte
 | Returns: sightings of code
 ----------------------*/
unsigned gpu_census_count(unsigned code);

/*----------------------
 | gpu_census_flag_count
 | Description: Returns how many times a notable primitive trait has been flagged.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: flag -- trait to read
 | Returns: sightings of flag
 ----------------------*/
unsigned gpu_census_flag_count(GpuCensusFlag flag);

/*----------------------
 | gpu_census_report
 | Description: Logs one line listing every non-zero command code count and trait flag seen so far.
 | Author: suinevere
 | Dependencies: dc_log
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void gpu_census_report(void);

#endif
