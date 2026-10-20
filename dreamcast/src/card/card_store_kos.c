/*----------------------
 | card_store_kos.c
 | Description: Keeps each PS1 card file as one VMS package on a VMU through KallistiOS vmufs: slot n is the nth VMU found, a file's VMU name comes from vmu_name_for_ps1 and its long description holds the full PS1 name.
 | Author: suinevere
 | Dependencies: kos.h, dc/vmufs.h, dc/vmu_pkg.h, dc/maple/vmu.h, card_store_kos.h, card_vmu.c, vmu_format.h
 | Globals: s_lcd
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdlib.h>
#include <string.h>
#include <kos.h>
#include <dc/vmufs.h>
#include <dc/vmu_pkg.h>
#include <dc/maple/vmu.h>
#include "card_store_kos.h"
#include "vmu_format.h"

/*----------------------
 | KOS_VMU_HASH_PREFIX / KOS_VMU_SHORT_DESC / KOS_VMU_APP_ID
 | Description: Prefix of VMU names hashed from foreign PS1 names, and the package's short description and application id.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define KOS_VMU_HASH_PREFIX "LOMX"
#define KOS_VMU_SHORT_DESC "LEGEND OF MANA"
#define KOS_VMU_APP_ID "LEGEND_OF_MANA"

/*----------------------
 | s_lcd
 | Description: The 48x32 wordmark sent to every VMU screen; vmu_draw_lcd_rotated needs it 4-byte aligned.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_lcd[192] __attribute__((aligned(4)));

/*----------------------
 | slot_device
 | Description: The VMU for a card slot: the slot-th memory card maple finds.
 | Author: suinevere
 | Dependencies: maple_enum_type
 | Globals: N/A
 | Params: slot -- 0 or 1
 | Returns: device, or NULL
 ----------------------*/
static maple_device_t *slot_device(int slot)
{
    return maple_enum_type(slot, MAPLE_FUNC_MEMCARD);
}

/*----------------------
 | kos_present
 | Description: Whether the slot has a VMU.
 | Author: suinevere
 | Dependencies: slot_device
 | Globals: N/A
 | Params: slot -- 0 or 1
 | Returns: nonzero when present
 ----------------------*/
static int kos_present(int slot)
{
    return slot_device(slot) != NULL;
}

/*----------------------
 | read_package
 | Description: Reads a VMU file and parses its VMS package, checking its CRC.
 | Author: suinevere
 | Dependencies: vmufs_read, vmu_pkg_parse
 | Globals: N/A
 | Params: dev -- VMU, vmu_name -- VMU file name, pkg -- parsed package out, raw -- file bytes out, freed by the caller
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int read_package(maple_device_t *dev, const char *vmu_name, vmu_pkg_t *pkg, void **raw)
{
    int size = 0;

    *raw = NULL;
    if (vmufs_read(dev, vmu_name, raw, &size) < 0)
    {
        return -1;
    }
    if (vmu_pkg_parse((uint8 *)*raw, pkg) < 0)
    {
        free(*raw);
        *raw = NULL;
        return -1;
    }
    return 0;
}

/*----------------------
 | kos_list
 | Description: Lists the slot's Legend of Mana files. Names and sizes come from the directory alone, since PS1 files fill whole 8 KB blocks; only hashed names need their package's long description.
 | Author: suinevere
 | Dependencies: slot_device, vmufs_readdir, read_package, vmu_ps1_name_from_desc
 | Globals: N/A
 | Params: slot -- 0 or 1, out -- entries out, max -- capacity of out
 | Returns: entry count, or -1
 ----------------------*/
static int kos_list(int slot, card_store_entry *out, int max)
{
    maple_device_t *dev = slot_device(slot);
    vmu_dir_t *dir = NULL;
    int count = 0;
    int n = 0;
    int i;

    if (dev == NULL || vmufs_readdir(dev, &dir, &count) < 0)
    {
        return dev == NULL ? -1 : 0;
    }
    for (i = 0; i < count && n < max; i++)
    {
        char vmu_name[VMU_NAME_LEN + 1];
        unsigned int bytes = (unsigned int)dir[i].filesize * VMU_BLOCK_BYTES;

        memcpy(vmu_name, dir[i].filename, VMU_NAME_LEN);
        vmu_name[VMU_NAME_LEN] = 0;
        if (dir[i].filetype != 0x33 || strncmp(vmu_name, "LOM", 3) != 0 || bytes < 128 + PS1_BLOCK_BYTES)
        {
            continue;
        }
        if (strncmp(vmu_name, KOS_VMU_HASH_PREFIX, 4) == 0)
        {
            vmu_pkg_t pkg;
            void *raw;

            if (read_package(dev, vmu_name, &pkg, &raw) != 0)
            {
                continue;
            }
            vmu_ps1_name_from_desc(pkg.desc_long, (int)sizeof pkg.desc_long, out[n].ps1_name);
            out[n].size = (unsigned int)pkg.data_len;
            free(raw);
        }
        else
        {
            strcpy(out[n].ps1_name, LOM_PS1_PRODUCT);
            strncat(out[n].ps1_name, vmu_name + 3, sizeof out[n].ps1_name - 1 - strlen(LOM_PS1_PRODUCT));
            out[n].size = (bytes - 128) / PS1_BLOCK_BYTES * PS1_BLOCK_BYTES;
        }
        n++;
    }
    free(dir);
    return n;
}

/*----------------------
 | kos_read
 | Description: Reads a PS1 file's bytes out of its VMS package.
 | Author: suinevere
 | Dependencies: slot_device, vmu_name_for_ps1, read_package
 | Globals: N/A
 | Params: slot -- 0 or 1, ps1_name -- PS1 file name, buf -- destination, max -- capacity of buf
 | Returns: bytes copied, or -1
 ----------------------*/
static int kos_read(int slot, const char *ps1_name, unsigned char *buf, unsigned int max)
{
    maple_device_t *dev = slot_device(slot);
    char vmu_name[VMU_NAME_LEN + 1];
    vmu_pkg_t pkg;
    void *raw;
    unsigned int count;

    if (dev == NULL)
    {
        return -1;
    }
    vmu_name_for_ps1(ps1_name, vmu_name);
    if (read_package(dev, vmu_name, &pkg, &raw) != 0)
    {
        return -1;
    }
    count = (unsigned int)pkg.data_len < max ? (unsigned int)pkg.data_len : max;
    memcpy(buf, pkg.data, count);
    free(raw);
    return (int)count;
}

/*----------------------
 | kos_write
 | Description: Stores a PS1 file as a VMS package with the PS1 icon converted, replacing any file of the same name.
 | Author: suinevere
 | Dependencies: slot_device, vmu_name_for_ps1, vmu_icon_from_ps1, vmu_pkg_build, vmufs_write
 | Globals: N/A
 | Params: slot -- 0 or 1, ps1_name -- PS1 file name, buf -- file bytes, size -- byte count
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int kos_write(int slot, const char *ps1_name, const unsigned char *buf, unsigned int size)
{
    static unsigned char frames[3 * 512];
    maple_device_t *dev = slot_device(slot);
    char vmu_name[VMU_NAME_LEN + 1];
    vmu_pkg_t pkg;
    uint8 *out = NULL;
    int out_size = 0;
    int result;

    if (dev == NULL)
    {
        return -1;
    }
    memset(&pkg, 0, sizeof pkg);
    strcpy(pkg.desc_short, KOS_VMU_SHORT_DESC);
    strncpy(pkg.desc_long, ps1_name, sizeof pkg.desc_long - 1);
    strcpy(pkg.app_id, KOS_VMU_APP_ID);
    memset(frames, 0, sizeof frames);
    pkg.icon_cnt = size >= 0x200 ? vmu_icon_from_ps1(buf, pkg.icon_pal, frames) : 0;
    if (pkg.icon_cnt == 0)
    {
        pkg.icon_cnt = 1;
    }
    pkg.icon_anim_speed = 8;
    pkg.eyecatch_type = VMUPKG_EC_NONE;
    pkg.data_len = (int)size;
    pkg.icon_data = frames;
    pkg.data = buf;
    vmu_name_for_ps1(ps1_name, vmu_name);
    if (vmu_pkg_build(&pkg, &out, &out_size) < 0)
    {
        return -1;
    }
    result = vmufs_write(dev, vmu_name, out, out_size, VMUFS_OVERWRITE);
    free(out);
    return result < 0 ? -1 : 0;
}

/*----------------------
 | kos_remove
 | Description: Deletes a PS1 file's VMU file.
 | Author: suinevere
 | Dependencies: slot_device, vmu_name_for_ps1, vmufs_delete
 | Globals: N/A
 | Params: slot -- 0 or 1, ps1_name -- PS1 file name
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int kos_remove(int slot, const char *ps1_name)
{
    maple_device_t *dev = slot_device(slot);
    char vmu_name[VMU_NAME_LEN + 1];

    if (dev == NULL)
    {
        return -1;
    }
    vmu_name_for_ps1(ps1_name, vmu_name);
    return vmufs_delete(dev, vmu_name) < 0 ? -1 : 0;
}

/*----------------------
 | kos_free_blocks
 | Description: Free user blocks on the slot's VMU.
 | Author: suinevere
 | Dependencies: slot_device, vmufs_free_blocks
 | Globals: N/A
 | Params: slot -- 0 or 1
 | Returns: free blocks, 0 with no VMU
 ----------------------*/
static int kos_free_blocks(int slot)
{
    maple_device_t *dev = slot_device(slot);
    int blocks;

    if (dev == NULL)
    {
        return 0;
    }
    blocks = vmufs_free_blocks(dev);
    return blocks < 0 ? 0 : blocks;
}

/*----------------------
 | card_store_kos
 | Description: The VMU store's operations.
 | Author: suinevere
 | Dependencies: kos_present, kos_list, kos_read, kos_write, kos_remove, kos_free_blocks
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
const card_store_ops card_store_kos = {kos_present, kos_list, kos_read, kos_write, kos_remove, kos_free_blocks};

/*----------------------
 | card_store_kos_init
 | Description: Installs the VMU store behind the card calls and puts the Mana wordmark on every VMU screen found at boot.
 | Author: suinevere
 | Dependencies: card_vmu_set_store, vmu_lcd_wordmark, maple_enum_type, vmu_draw_lcd_rotated
 | Globals: s_lcd
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void card_store_kos_init(void)
{
    maple_device_t *dev;
    int i;

    card_vmu_set_store(&card_store_kos);
    vmu_lcd_wordmark(s_lcd);
    for (i = 0; (dev = maple_enum_type(i, MAPLE_FUNC_LCD)) != NULL; i++)
    {
        vmu_draw_lcd_rotated(dev, s_lcd);
    }
}
