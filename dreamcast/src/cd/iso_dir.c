/*----------------------
 | iso_dir.c
 | Description: ISO9660 primary volume descriptor and directory record parsing for the Dreamcast disc filesystem.
 | Author: suinevere
 | Dependencies: iso_dir.h, string.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <string.h>

#include "iso_dir.h"

/*----------------------
 | ISO_FLAG_DIRECTORY
 | Description: Directory record file-flags bit marking a directory.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ISO_FLAG_DIRECTORY 0x02u

/*----------------------
 | le32
 | Description: Reads the little-endian half of an ISO9660 both-endian 32-bit field.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- field
 | Returns: value
 ----------------------*/
static unsigned le32(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

/*----------------------
 | record_entry
 | Description: Fills an entry from a directory record.
 | Author: suinevere
 | Dependencies: le32
 | Globals: N/A
 | Params: record -- directory record, entry -- receives it
 | Returns: N/A
 ----------------------*/
static void record_entry(const unsigned char *record, IsoEntry *entry)
{
    entry->extent = le32(record + 2);
    entry->size = le32(record + 10);
    entry->is_dir = (record[25] & ISO_FLAG_DIRECTORY) != 0;
}

/*----------------------
 | fold
 | Description: Upper-cases an ASCII letter.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- character
 | Returns: the folded character
 ----------------------*/
static char fold(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

/*----------------------
 | name_matches
 | Description: Compares a record's identifier with a name, ignoring case, a ";N" version and a trailing dot.
 | Author: suinevere
 | Dependencies: fold
 | Globals: N/A
 | Params: id -- identifier, id_length -- its length, name -- wanted name, length -- its length
 | Returns: 1 on a match, else 0
 ----------------------*/
static int name_matches(const unsigned char *id, unsigned id_length, const char *name, unsigned length)
{
    unsigned i;

    for (i = 0; i < id_length; i++)
        if (id[i] == ';')
        {
            id_length = i;
            break;
        }
    if (id_length > 0 && id[id_length - 1] == '.')
        id_length--;
    if (id_length != length)
        return 0;
    for (i = 0; i < length; i++)
        if (fold((char)id[i]) != fold(name[i]))
            return 0;
    return 1;
}

int iso_pvd_root(const unsigned char *pvd, IsoEntry *root)
{
    if (pvd[0] != 1 || memcmp(pvd + 1, "CD001", 5) != 0)
        return -1;
    record_entry(pvd + 156, root);
    return 0;
}

int iso_dir_find(const unsigned char *sector, const char *name, unsigned length, IsoEntry *found)
{
    unsigned at = 0;

    while (at + 33u <= ISO_SECTOR_BYTES)
    {
        const unsigned char *record = sector + at;
        unsigned record_length = record[0];
        unsigned id_length = record[32];

        if (record_length == 0 || at + record_length > ISO_SECTOR_BYTES || 33u + id_length > record_length)
            return 0;
        if (name_matches(record + 33, id_length, name, length))
        {
            record_entry(record, found);
            return 1;
        }
        at += record_length;
    }
    return 0;
}
