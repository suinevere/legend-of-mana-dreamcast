/*----------------------
 | libpress_dc.h
 | Description: Dreamcast-only additions to libpress: decode timing for the probe and the movie log.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_LIBPRESS_DC_H
#define LOM_LIBPRESS_DC_H

/*----------------------
 | libpress_dc_stats
 | Description: Reports decode timing since the previous call and starts a new period.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: frames, us_avg, us_max -- outputs
 | Returns: N/A
 ----------------------*/
void libpress_dc_stats(unsigned *frames, unsigned *us_avg, unsigned *us_max);

#endif
