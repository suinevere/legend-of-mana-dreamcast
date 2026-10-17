/*----------------------
 | card_store.h
 | Description: The storage interface card_vmu.c keeps PS1 card files behind, plus the lom_card_* file calls the overlays' open, read, write, close and rename link to.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_CARD_STORE_H
#define LOM_CARD_STORE_H

/*----------------------
 | CARD_STORE_NAME_BYTES
 | Description: Bytes a stored PS1 file name takes, its 20 characters plus a terminator.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CARD_STORE_NAME_BYTES 21

/*----------------------
 | card_store_entry
 | Description: One stored PS1 file: its name without the device prefix and its size in bytes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct card_store_entry
{
    char ps1_name[CARD_STORE_NAME_BYTES];
    unsigned int size;
} card_store_entry;

/*----------------------
 | card_store_ops
 | Description: A store for each of the two card slots: presence, listing, whole-file read, write and remove, and free space in VMU blocks.
 |              list returns a count or -1, read returns bytes or -1, write and remove return 0 or -1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct card_store_ops
{
    int (*present)(int slot);
    int (*list)(int slot, card_store_entry *out, int max);
    int (*read)(int slot, const char *ps1_name, unsigned char *buf, unsigned int max);
    int (*write)(int slot, const char *ps1_name, const unsigned char *buf, unsigned int size);
    int (*remove)(int slot, const char *ps1_name);
    int (*free_blocks)(int slot);
} card_store_ops;

void card_vmu_set_store(const card_store_ops *ops);
long lom_card_open(char *path, unsigned long mode);
long lom_card_read(long fd, void *buf, long n);
long lom_card_write(long fd, void *buf, long n);
long lom_card_close(long fd);
long lom_card_rename(char *from, char *to);

#endif
