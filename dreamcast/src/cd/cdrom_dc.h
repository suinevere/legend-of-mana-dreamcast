/*----------------------
 | cdrom_dc.h
 | Description: Dreamcast implementation of the decomp's cdrom.h API: stdio reads of converted resources from the disc, keyed by PS1 resource index.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_CDROM_DC_H
#define LOM_CDROM_DC_H

/*----------------------
 | LOM_CD_ROOT
 | Description: Disc mount point every resource path is built under; a host test overrides it with -D to point at a temp fixture directory.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_CD_ROOT
#define LOM_CD_ROOT "/cd"
#endif

#endif
