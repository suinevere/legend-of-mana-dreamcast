/*----------------------
 | frame_probe.h
 | Description: Per-second frame probe: counts PowerVR scenes and primitives and logs vblanks per frame with texture and palette pressure, plus a periodic GPU census.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_FRAME_PROBE_H
#define LOM_FRAME_PROBE_H

/*----------------------
 | frame_probe_scene
 | Description: Counts one PowerVR scene closed by pvr_submit_flip.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void frame_probe_scene(void);

/*----------------------
 | frame_probe_drawn / frame_probe_drawn_total
 | Description: Counts a closed scene that drew something, and returns how many have since start-up, for the hang check.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: frame_probe_drawn_total -- the count
 ----------------------*/
void frame_probe_drawn(void);
unsigned frame_probe_drawn_total(void);

/*----------------------
 | frame_probe_prim
 | Description: Counts one primitive pvr_submit_prim wrote to the PowerVR.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void frame_probe_prim(void);

/*----------------------
 | frame_probe_vblank
 | Description: Called from the VSync hook; every 60 vblanks logs the probe line, and every 600 the GPU census.
 | Author: suinevere
 | Dependencies: vsync_count, dc_log, gpu_census_report
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void frame_probe_vblank(void);

#endif
