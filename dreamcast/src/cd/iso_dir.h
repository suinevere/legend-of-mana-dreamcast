/*----------------------
 | iso_dir.h
 | Description: ISO9660 primary volume descriptor and directory record parsing, free of KallistiOS so the host tests can run it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_ISO_DIR_H
#define LOM_ISO_DIR_H

/*----------------------
 | ISO_SECTOR_BYTES
 | Description: Bytes of user data in one ISO9660 logical sector.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ISO_SECTOR_BYTES 2048u

/*----------------------
 | IsoEntry
 | Description: Where a file or directory lives: its first logical sector, its byte size and whether it is a directory.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct IsoEntry
{
    unsigned extent;
    unsigned size;
    int is_dir;
} IsoEntry;

/*----------------------
 | iso_pvd_root
 | Description: Reads the root directory's entry out of a primary volume descriptor sector.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: pvd -- one 2048-byte sector, root -- receives the root directory
 | Returns: 0, or -1 when the sector is not a primary volume descriptor
 ----------------------*/
int iso_pvd_root(const unsigned char *pvd, IsoEntry *root);

/*----------------------
 | iso_dir_find
 | Description: Searches one directory sector for a name, ignoring case, a ";N" version and a trailing dot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: sector -- one 2048-byte directory sector, name -- component to find, length -- its length, found -- receives the entry
 | Returns: 1 when found, else 0
 ----------------------*/
int iso_dir_find(const unsigned char *sector, const char *name, unsigned length, IsoEntry *found);

#endif
