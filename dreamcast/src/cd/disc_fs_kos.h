/*----------------------
 | disc_fs_kos.h
 | Description: The game's own read-only ISO9660 filesystem at /cd, replacing KallistiOS's so GD-ROM images mount their high-density track and reads go to the drive in multi-sector commands.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_DISC_FS_KOS_H
#define LOM_DISC_FS_KOS_H

/*----------------------
 | disc_fs_kos_init
 | Description: Unregisters KallistiOS's /cd filesystem, registers this one in its place, mounts the disc's data track and logs what the mount found.
 | Author: suinevere
 | Dependencies: fs_iso9660_shutdown, nmmgr_handler_add, dc_log
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void disc_fs_kos_init(void);

/*----------------------
 | disc_fs_kos_status
 | Description: The last mount's per-step results (drive init, status and type, then each area's TOC, data track, read and descriptor codes), for a halt message.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_status
 | Params: N/A
 | Returns: a static string
 ----------------------*/
const char *disc_fs_kos_status(void);

#endif
