/*----------------------
 | card_store_kos.c
 | Description: Keeps each PS1 card file as one VMS package on a VMU through KallistiOS vmufs: slot n is the nth VMU found, a file's VMU name comes from vmu_name_for_ps1 and its long description holds the full PS1 name.
 | Author: suinevere
 | Dependencies: kos.h, dc/vmufs.h, dc/vmu_pkg.h, dc/maple/vmu.h, card_store_kos.h, card_vmu.c, vmu_format.h
 | Globals: s_lcd, s_job, s_job_state
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
 | KOS_VMS_DESC_LONG_OFFSET / KOS_VMS_DESC_LONG_BYTES / KOS_VMS_DATA_LEN_OFFSET
 | Description: Where a VMS header keeps its long description and data length, the two fields a listing needs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define KOS_VMS_DESC_LONG_OFFSET 0x10
#define KOS_VMS_DESC_LONG_BYTES 32
#define KOS_VMS_DATA_LEN_OFFSET 0x48

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
 | Description: Lists the slot's Legend of Mana files. Names and sizes come from the directory alone, since PS1 files fill whole 8 KB blocks; only hashed names need their package's long description and data length,
 |              read from the header block alone so a listing never pulls whole packages over the Maple bus.
 | Author: suinevere
 | Dependencies: slot_device, vmufs_readdir, vmu_block_read, vmu_ps1_name_from_desc
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
            static uint8 header[VMU_BLOCK_BYTES] __attribute__((aligned(4)));
            unsigned int data_len;

            if (dir[i].hdroff != 0 || vmu_block_read(dev, dir[i].firstblk, header) != MAPLE_EOK)
            {
                continue;
            }
            memcpy(&data_len, header + KOS_VMS_DATA_LEN_OFFSET, sizeof data_len);
            vmu_ps1_name_from_desc((const char *)header + KOS_VMS_DESC_LONG_OFFSET, KOS_VMS_DESC_LONG_BYTES, out[n].ps1_name);
            out[n].size = data_len;
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
 | build_package
 | Description: Builds the VMS package a PS1 file is stored as, with the PS1 icon converted and the full PS1 name in the long description.
 | Author: suinevere
 | Dependencies: vmu_icon_from_ps1, vmu_pkg_build
 | Globals: N/A
 | Params: ps1_name -- PS1 file name, buf -- file bytes, size -- byte count, out -- the package, malloc'd, out_size -- its bytes
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int build_package(const char *ps1_name, const unsigned char *buf, unsigned int size, uint8 **out, int *out_size)
{
    static unsigned char frames[3 * 512];
    vmu_pkg_t pkg;

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
    *out = NULL;
    return vmu_pkg_build(&pkg, out, out_size) < 0 ? -1 : 0;
}

/*----------------------
 | kos_write
 | Description: Stores a PS1 file as a VMS package, replacing any file of the same name.
 | Author: suinevere
 | Dependencies: slot_device, vmu_name_for_ps1, build_package, vmufs_write
 | Globals: N/A
 | Params: slot -- 0 or 1, ps1_name -- PS1 file name, buf -- file bytes, size -- byte count
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int kos_write(int slot, const char *ps1_name, const unsigned char *buf, unsigned int size)
{
    maple_device_t *dev = slot_device(slot);
    char vmu_name[VMU_NAME_LEN + 1];
    uint8 *out;
    int out_size = 0;
    int result;

    if (dev == NULL || build_package(ps1_name, buf, size, &out, &out_size) != 0)
    {
        return -1;
    }
    vmu_name_for_ps1(ps1_name, vmu_name);
    result = vmufs_write(dev, vmu_name, out, out_size, VMUFS_OVERWRITE);
    free(out);
    return result < 0 ? -1 : 0;
}

/*----------------------
 | s_job / s_job_state
 | Description: The one background write kos_write_start hands its thread, and how it stands: idle, busy, done or failed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum { JOB_IDLE = 0, JOB_BUSY = 1, JOB_DONE = 2, JOB_FAILED = 3 };
static struct
{
    int slot;
    char ps1_name[CARD_STORE_NAME_BYTES];
    const unsigned char *buf;
    unsigned int size;
} s_job;
static volatile int s_job_state;

/*----------------------
 | run_job
 | Description: The background thread: runs kos_write for s_job, leaving the game thread to draw while the Maple bus carries the blocks.
 | Author: suinevere
 | Dependencies: kos_write
 | Globals: s_job, s_job_state
 | Params: param -- unused
 | Returns: NULL
 ----------------------*/
static void *run_job(void *param)
{
    (void)param;
    s_job_state = kos_write(s_job.slot, s_job.ps1_name, s_job.buf, s_job.size) == 0 ? JOB_DONE : JOB_FAILED;
    return NULL;
}

/*----------------------
 | kos_write_start / kos_job_poll
 | Description: Start kos_write on a detached thread, one at a time, and report it: 1 while busy (yielding to it), then 0 or -1 once.
 | Author: suinevere
 | Dependencies: thd_create, thd_pass, run_job
 | Globals: s_job, s_job_state
 | Params: slot, ps1_name, buf, size -- as kos_write; buf must stay put until the poll stops returning 1
 | Returns: kos_write_start -- 0 once started, -1 when busy or no thread; kos_job_poll -- 1, 0 or -1
 ----------------------*/
static int kos_write_start(int slot, const char *ps1_name, const unsigned char *buf, unsigned int size)
{
    if (s_job_state != JOB_IDLE || slot_device(slot) == NULL)
    {
        return -1;
    }
    s_job.slot = slot;
    strncpy(s_job.ps1_name, ps1_name, sizeof s_job.ps1_name - 1);
    s_job.ps1_name[sizeof s_job.ps1_name - 1] = 0;
    s_job.buf = buf;
    s_job.size = size;
    s_job_state = JOB_BUSY;
    if (thd_create(true, run_job, NULL) == NULL)
    {
        s_job_state = JOB_IDLE;
        return -1;
    }
    return 0;
}

static int kos_job_poll(void)
{
    int state = s_job_state;

    if (state == JOB_BUSY)
    {
        thd_pass();
        return 1;
    }
    s_job_state = JOB_IDLE;
    return state == JOB_FAILED ? -1 : 0;
}

/*----------------------
 | write_file_blocks
 | Description: Writes the package blocks of a VMU file whose indices fall in [first, last], walking its FAT chain; the package's tail block is zero-padded.
 | Author: suinevere
 | Dependencies: vmu_block_write
 | Globals: N/A
 | Params: dev -- VMU, fat -- its FAT, block -- first block of the file, pkg / pkg_size -- package bytes, first, last -- package block range
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int write_file_blocks(maple_device_t *dev, const uint16 *fat, uint16 block, const uint8 *pkg, int pkg_size, int first, int last)
{
    static uint8 tail[VMU_BLOCK_BYTES] __attribute__((aligned(4)));
    int index;

    for (index = 0; index <= last && block < 0xFF00; index++, block = fat[block])
    {
        const uint8 *src = pkg + index * VMU_BLOCK_BYTES;

        if (index < first)
        {
            continue;
        }
        if ((index + 1) * VMU_BLOCK_BYTES > pkg_size)
        {
            memset(tail, 0, sizeof tail);
            memcpy(tail, src, (size_t)(pkg_size - index * VMU_BLOCK_BYTES));
            src = tail;
        }
        if (vmu_block_write(dev, block, src) != MAPLE_EOK)
        {
            return -1;
        }
    }
    return index > last ? 0 : -1;
}

/*----------------------
 | kos_rewrite
 | Description: Renames a stored package in its directory entry, replacing any file already under the new name, and rewrites only its header and icon blocks plus those holding data bytes [offset, offset + len).
 |              Replaces two whole-package writes per save with a handful of block writes.
 | Author: suinevere
 | Dependencies: slot_device, build_package, vmu_name_for_ps1, vmufs_mutex_lock, vmufs_root_read, vmufs_dir_read, vmufs_fat_read, vmufs_dir_find, vmufs_file_delete, write_file_blocks, vmufs_fat_write, vmufs_dir_write
 | Globals: N/A
 | Params: slot -- 0 or 1, old_name / new_name -- PS1 names, buf / size -- the file's current bytes, offset / len -- changed data range, len 0 for none
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int kos_rewrite(int slot, const char *old_name, const char *new_name, const unsigned char *buf, unsigned int size, unsigned int offset, unsigned int len)
{
    maple_device_t *dev = slot_device(slot);
    char old_vmu[VMU_NAME_LEN + 1];
    char new_vmu[VMU_NAME_LEN + 1];
    vmu_root_t root;
    vmu_dir_t *dir = NULL;
    uint16 *fat = NULL;
    uint8 *pkg = NULL;
    int pkg_size = 0;
    int data_off;
    int index;
    int result = -1;

    if (dev == NULL || build_package(new_name, buf, size, &pkg, &pkg_size) != 0)
    {
        return -1;
    }
    data_off = pkg_size - (int)size;
    vmu_name_for_ps1(old_name, old_vmu);
    vmu_name_for_ps1(new_name, new_vmu);
    vmufs_mutex_lock();
    if (vmufs_root_read(dev, &root) < 0)
    {
        goto out;
    }
    dir = malloc((size_t)vmufs_dir_blocks(&root));
    fat = malloc((size_t)vmufs_fat_blocks(&root));
    if (dir == NULL || fat == NULL || vmufs_dir_read(dev, &root, dir) < 0 || vmufs_fat_read(dev, &root, fat) < 0)
    {
        goto out;
    }
    index = vmufs_dir_find(&root, dir, old_vmu);
    if (index < 0 || (int)dir[index].filesize * VMU_BLOCK_BYTES < pkg_size)
    {
        goto out;
    }
    if (write_file_blocks(dev, fat, dir[index].firstblk, pkg, pkg_size, 0, (data_off - 1) / VMU_BLOCK_BYTES) != 0)
    {
        goto out;
    }
    if (len != 0 && write_file_blocks(dev, fat, dir[index].firstblk, pkg, pkg_size, (data_off + (int)offset) / VMU_BLOCK_BYTES,
                                      (data_off + (int)(offset + len) - 1) / VMU_BLOCK_BYTES) != 0)
    {
        goto out;
    }
    if (strcmp(old_vmu, new_vmu) != 0)
    {
        int fat_changed = vmufs_file_delete(&root, fat, dir, new_vmu) == 0;

        index = vmufs_dir_find(&root, dir, old_vmu);
        memset(dir[index].filename, 0, sizeof dir[index].filename);
        memcpy(dir[index].filename, new_vmu, strlen(new_vmu));
        dir[index].dirty = 1;
        if ((fat_changed && vmufs_fat_write(dev, &root, fat) < 0) || vmufs_dir_write(dev, &root, dir) < 0)
        {
            goto out;
        }
    }
    result = 0;
out:
    vmufs_mutex_unlock();
    free(dir);
    free(fat);
    free(pkg);
    return result;
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
 | Dependencies: kos_present, kos_list, kos_read, kos_write, kos_remove, kos_free_blocks, kos_write_start, kos_job_poll, kos_rewrite
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
const card_store_ops card_store_kos = {kos_present, kos_list, kos_read, kos_write, kos_remove, kos_free_blocks, kos_write_start, kos_job_poll, kos_rewrite};

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
