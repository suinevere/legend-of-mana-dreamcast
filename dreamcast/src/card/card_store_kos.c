/*----------------------
 | card_store_kos.c
 | Description: Keeps each PS1 card file as one LZSS-compressed VMS package on a VMU through KallistiOS vmufs: slot n is the nth VMU found, a file's VMU name comes from vmu_name_for_ps1 and its long description holds the full PS1 name.
 | Author: suinevere
 | Dependencies: kos.h, dc/vmufs.h, dc/vmu_pkg.h, dc/maple/vmu.h, card_store_kos.h, card_vmu.c, vmu_format.h, save_pack.h
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
#include "save_pack.h"

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
 | KOS_VMS_DESC_LONG_OFFSET / KOS_VMS_DESC_LONG_BYTES / KOS_VMS_CRC_OFFSET / KOS_VMS_DATA_LEN_OFFSET / KOS_VMS_RAW_LEN_OFFSET
 | Description: Where a VMS header keeps its long description, CRC and stored data length, and the first reserved word, which holds the PS1 file's uncompressed size.
 |              Data whose stored length equals that size is kept uncompressed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define KOS_VMS_DESC_LONG_OFFSET 0x10
#define KOS_VMS_DESC_LONG_BYTES 32
#define KOS_VMS_CRC_OFFSET 0x46
#define KOS_VMS_DATA_LEN_OFFSET 0x48
#define KOS_VMS_RAW_LEN_OFFSET 0x4C

/*----------------------
 | KOS_VMU_TRIES / KOS_VMU_RETRY_MS
 | Description: How often a failed VMU call is tried and the pause between tries; a VMU's maple frame is busy for up to a frame while its screen is drawn, and KallistiOS fails a block access then rather than waiting.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define KOS_VMU_TRIES 10
#define KOS_VMU_RETRY_MS 10

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
 | vmu_retry
 | Description: Counts a failed VMU call and, while tries remain, waits before the next.
 | Author: suinevere
 | Dependencies: thd_sleep
 | Globals: N/A
 | Params: attempt -- tries made so far, advanced
 | Returns: nonzero to try again, 0 to give up
 ----------------------*/
static int vmu_retry(int *attempt)
{
    if (++*attempt >= KOS_VMU_TRIES)
    {
        return 0;
    }
    thd_sleep(KOS_VMU_RETRY_MS);
    return 1;
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
 | Dependencies: vmufs_read, vmu_retry, vmu_pkg_parse
 | Globals: N/A
 | Params: dev -- VMU, vmu_name -- VMU file name, pkg -- parsed package out, raw -- file bytes out, freed by the caller
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int read_package(maple_device_t *dev, const char *vmu_name, vmu_pkg_t *pkg, void **raw)
{
    int size = 0;
    int attempt = 0;

    *raw = NULL;
    while (vmufs_read(dev, vmu_name, raw, &size) < 0)
    {
        if (!vmu_retry(&attempt))
        {
            return -1;
        }
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
 | Description: Lists the slot's Legend of Mana files, reading only each package's header block for its uncompressed size, and for hashed names its long description, so a listing never pulls whole packages over the Maple bus.
 | Author: suinevere
 | Dependencies: slot_device, vmufs_readdir, vmu_block_read, vmu_retry, vmu_ps1_name_from_desc
 | Globals: N/A
 | Params: slot -- 0 or 1, out -- entries out, max -- capacity of out
 | Returns: entry count, or -1 when the VMU is gone or cannot be read
 ----------------------*/
static int kos_list(int slot, card_store_entry *out, int max)
{
    maple_device_t *dev = slot_device(slot);
    vmu_dir_t *dir = NULL;
    int count = 0;
    int n = 0;
    int attempt = 0;
    int i;

    if (dev == NULL)
    {
        return -1;
    }
    while (vmufs_readdir(dev, &dir, &count) < 0)
    {
        if (!vmu_retry(&attempt))
        {
            return -1;
        }
    }
    for (i = 0; i < count && n < max; i++)
    {
        char vmu_name[VMU_NAME_LEN + 1];
        static uint8 header[VMU_BLOCK_BYTES] __attribute__((aligned(4)));
        unsigned int raw_len;

        memcpy(vmu_name, dir[i].filename, VMU_NAME_LEN);
        vmu_name[VMU_NAME_LEN] = 0;
        if (dir[i].filetype != 0x33 || strncmp(vmu_name, "LOM", 3) != 0 || dir[i].hdroff != 0)
        {
            continue;
        }
        attempt = 0;
        while (vmu_block_read(dev, dir[i].firstblk, header) != MAPLE_EOK)
        {
            if (!vmu_retry(&attempt))
            {
                free(dir);
                return -1;
            }
        }
        memcpy(&raw_len, header + KOS_VMS_RAW_LEN_OFFSET, sizeof raw_len);
        if (raw_len == 0 || raw_len % PS1_BLOCK_BYTES != 0 || raw_len > PS1_CARD_BLOCKS * PS1_BLOCK_BYTES)
        {
            continue;
        }
        if (strncmp(vmu_name, KOS_VMU_HASH_PREFIX, 4) == 0)
        {
            vmu_ps1_name_from_desc((const char *)header + KOS_VMS_DESC_LONG_OFFSET, KOS_VMS_DESC_LONG_BYTES, out[n].ps1_name);
        }
        else
        {
            strcpy(out[n].ps1_name, LOM_PS1_PRODUCT);
            strncat(out[n].ps1_name, vmu_name + 3, sizeof out[n].ps1_name - 1 - strlen(LOM_PS1_PRODUCT));
        }
        out[n].size = raw_len;
        n++;
    }
    free(dir);
    return n;
}

/*----------------------
 | kos_read
 | Description: Reads a PS1 file's bytes out of its VMS package, decompressing them.
 | Author: suinevere
 | Dependencies: slot_device, vmu_name_for_ps1, read_package, save_unpack
 | Globals: N/A
 | Params: slot -- 0 or 1, ps1_name -- PS1 file name, buf -- destination, max -- capacity of buf
 | Returns: bytes copied, or -1 when missing or corrupt
 ----------------------*/
static int kos_read(int slot, const char *ps1_name, unsigned char *buf, unsigned int max)
{
    maple_device_t *dev = slot_device(slot);
    char vmu_name[VMU_NAME_LEN + 1];
    vmu_pkg_t pkg;
    void *raw;
    unsigned char *file;
    unsigned int raw_len;
    unsigned int count;
    int result;

    if (dev == NULL)
    {
        return -1;
    }
    vmu_name_for_ps1(ps1_name, vmu_name);
    if (read_package(dev, vmu_name, &pkg, &raw) != 0)
    {
        return -1;
    }
    memcpy(&raw_len, (const uint8 *)raw + KOS_VMS_RAW_LEN_OFFSET, sizeof raw_len);
    file = raw_len > PS1_CARD_BLOCKS * PS1_BLOCK_BYTES ? NULL : malloc(raw_len);
    if (file == NULL)
    {
        free(raw);
        return -1;
    }
    if ((unsigned int)pkg.data_len == raw_len)
    {
        memcpy(file, pkg.data, raw_len);
        result = 0;
    }
    else
    {
        result = save_unpack(pkg.data, (unsigned int)pkg.data_len, file, raw_len);
    }
    free(raw);
    count = raw_len < max ? raw_len : max;
    if (result == 0)
    {
        memcpy(buf, file, count);
    }
    free(file);
    return result == 0 ? (int)count : -1;
}

/*----------------------
 | build_package
 | Description: Builds the VMS package a PS1 file is stored as: the PS1 icon converted, the full PS1 name in the long description, the data LZSS-compressed unless that saves nothing,
 |              and the uncompressed size in the header's reserved word under a refreshed CRC.
 | Author: suinevere
 | Dependencies: vmu_icon_from_ps1, save_pack_bound, save_pack, vmu_pkg_build, vmu_pkg_crc
 | Globals: N/A
 | Params: ps1_name -- PS1 file name, buf -- file bytes, size -- byte count, out -- the package, malloc'd, out_size -- its bytes
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int build_package(const char *ps1_name, const unsigned char *buf, unsigned int size, uint8 **out, int *out_size)
{
    static unsigned char frames[3 * 512];
    vmu_pkg_t pkg;
    unsigned char *packed = malloc(save_pack_bound(size));
    int packed_size;
    unsigned short crc;
    int result;

    *out = NULL;
    if (packed == NULL)
    {
        return -1;
    }
    packed_size = save_pack(buf, size, packed, save_pack_bound(size));
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
    pkg.data_len = packed_size >= 0 && (unsigned int)packed_size < size ? packed_size : (int)size;
    pkg.icon_data = frames;
    pkg.data = pkg.data_len == (int)size ? buf : packed;
    result = vmu_pkg_build(&pkg, out, out_size) < 0 ? -1 : 0;
    free(packed);
    if (result == 0)
    {
        memcpy(*out + KOS_VMS_RAW_LEN_OFFSET, &size, sizeof size);
        memset(*out + KOS_VMS_CRC_OFFSET, 0, 2);
        crc = vmu_pkg_crc(*out, *out_size);
        memcpy(*out + KOS_VMS_CRC_OFFSET, &crc, sizeof crc);
    }
    return result;
}

/*----------------------
 | kos_write
 | Description: Stores a PS1 file as a VMS package, replacing any file of the same name.
 | Author: suinevere
 | Dependencies: slot_device, vmu_name_for_ps1, build_package, vmufs_write, vmu_retry
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
    int attempt = 0;
    int result;

    if (dev == NULL || build_package(ps1_name, buf, size, &out, &out_size) != 0)
    {
        return -1;
    }
    vmu_name_for_ps1(ps1_name, vmu_name);
    while ((result = vmufs_write(dev, vmu_name, out, out_size, VMUFS_OVERWRITE)) < 0 && vmu_retry(&attempt))
    {
    }
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
 | kos_remove
 | Description: Deletes a PS1 file's VMU file.
 | Author: suinevere
 | Dependencies: slot_device, vmu_name_for_ps1, vmufs_delete, vmu_retry
 | Globals: N/A
 | Params: slot -- 0 or 1, ps1_name -- PS1 file name
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int kos_remove(int slot, const char *ps1_name)
{
    maple_device_t *dev = slot_device(slot);
    char vmu_name[VMU_NAME_LEN + 1];
    int attempt = 0;
    int result;

    if (dev == NULL)
    {
        return -1;
    }
    vmu_name_for_ps1(ps1_name, vmu_name);
    while ((result = vmufs_delete(dev, vmu_name)) < 0 && vmu_retry(&attempt))
    {
    }
    return result < 0 ? -1 : 0;
}

/*----------------------
 | kos_free_blocks
 | Description: Free user blocks on the slot's VMU.
 | Author: suinevere
 | Dependencies: slot_device, vmufs_free_blocks, vmu_retry
 | Globals: N/A
 | Params: slot -- 0 or 1
 | Returns: free blocks, 0 with no VMU
 ----------------------*/
static int kos_free_blocks(int slot)
{
    maple_device_t *dev = slot_device(slot);
    int blocks;
    int attempt = 0;

    if (dev == NULL)
    {
        return 0;
    }
    while ((blocks = vmufs_free_blocks(dev)) < 0 && vmu_retry(&attempt))
    {
    }
    return blocks < 0 ? 0 : blocks;
}

/*----------------------
 | card_store_kos
 | Description: The VMU store's operations; no in-place rewrite, since compressed data cannot be patched block by block.
 | Author: suinevere
 | Dependencies: kos_present, kos_list, kos_read, kos_write, kos_remove, kos_free_blocks, kos_write_start, kos_job_poll
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
const card_store_ops card_store_kos = {kos_present, kos_list, kos_read, kos_write, kos_remove, kos_free_blocks, kos_write_start, kos_job_poll, NULL};

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
