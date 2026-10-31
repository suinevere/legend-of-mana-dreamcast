/*----------------------
 | card_store_kos.c
 | Description: Keeps each PS1 card file as one LZSS-compressed VMS package on a VMU through KallistiOS vmufs: slot n is the nth VMU found, a file's VMU name comes from vmu_name_for_ps1 and its long description holds the full PS1 name; the pet transfer also puts Ring Ring Land's VMU GAME at block 0.
 | Author: suinevere
 | Dependencies: kos.h, dc/vmufs.h, dc/vmu_pkg.h, dc/maple/vmu.h, card_store_kos.h, card_vmu.c, vmu_format.h, vmu_game.h, save_pack.h, heap_probe.h
 | Globals: s_job, s_job_state, s_game, s_game_size, s_replace_ok, s_root, s_fat, s_dir, s_dirents, s_dirent_count, s_pet_present
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
#include "vmu_game.h"
#include "vmu_screen.h"
#include "save_pack.h"
#include "heap_probe.h"

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
 | lom_vmu_pet_screens / lom_vmu_pet_screen_count
 | Description: The pet and egg portraits as VMU screens, which the pre-build step generates from the user's disc.
 | Author: suinevere
 | Dependencies: gen/vmu_pet_screens.c (port/tools/vmu_portraits.py)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern const unsigned char lom_vmu_pet_screens[][VMU_SCREEN_BYTES];
extern const unsigned int lom_vmu_pet_screen_count;

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
 | card_usable
 | Description: Reads a VMU's root block and says whether vmufs may touch the card, since vmufs sizes its FAT buffer from the root and a blank card's zero FAT size would overrun the heap.
 | Author: suinevere
 | Dependencies: vmufs_mutex_lock, vmufs_root_read, vmufs_mutex_unlock, vmu_retry, vmu_root_usable
 | Globals: N/A
 | Params: dev -- VMU, or NULL
 | Returns: nonzero when the card is formatted with tables vmufs can hold
 ----------------------*/
static int card_usable(maple_device_t *dev)
{
    vmu_root_t root;
    int attempt = 0;
    int rv;

    if (dev == NULL)
    {
        return 0;
    }
    for (;;)
    {
        vmufs_mutex_lock();
        rv = vmufs_root_read(dev, &root);
        vmufs_mutex_unlock();
        if (rv >= 0)
        {
            return vmu_root_usable((const unsigned char *)&root);
        }
        if (!vmu_retry(&attempt))
        {
            return 0;
        }
    }
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
 | Dependencies: slot_device, card_usable, vmufs_readdir, vmu_block_read, vmu_retry, vmu_ps1_name_from_desc
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

    if (!card_usable(dev))
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
 | Description: Reads a PS1 file's bytes out of its VMS package, decompressing them; a kept prefix (Ring Ring Land) comes back zero-filled to the file's size.
 | Author: suinevere
 | Dependencies: slot_device, card_usable, vmu_name_for_ps1, read_package, vmu_kept_bytes, save_unpack
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
    unsigned int kept;
    int result;

    if (!card_usable(dev))
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
    kept = vmu_kept_bytes(ps1_name, raw_len);
    if ((unsigned int)pkg.data_len == raw_len)
    {
        memcpy(file, pkg.data, raw_len);
        result = 0;
    }
    else if (kept != 0 && (unsigned int)pkg.data_len == kept)
    {
        memcpy(file, pkg.data, kept);
        memset(file + kept, 0, raw_len - kept);
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
 |              and the uncompressed size in the header's reserved word under a refreshed CRC. Ring Ring Land keeps only its uncompressed prefix, which the VMU game patches in place.
 | Author: suinevere
 | Dependencies: vmu_icon_from_ps1, vmu_kept_bytes, save_pack_bound, save_pack, vmu_pkg_build, vmu_pkg_crc
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
    unsigned int kept;
    int result;

    *out = NULL;
    if (packed == NULL)
    {
        return -1;
    }
    kept = vmu_kept_bytes(ps1_name, size);
    packed_size = kept != 0 ? -1 : save_pack(buf, size, packed, save_pack_bound(size));
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
    pkg.data_len = kept != 0 ? (int)kept : packed_size >= 0 && (unsigned int)packed_size < size ? packed_size : (int)size;
    pkg.icon_data = frames;
    pkg.data = kept != 0 || pkg.data_len == (int)size ? buf : packed;
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
 | KOS_PET_PS1_NAME / KOS_GAME_PATH / KOS_DIR_ENTRIES / KOS_FAT_WORDS
 | Description: The pet transfer's PS1 name, whose write carries Ring Ring Land's GAME, the GAME's path on the disc, and the directory entries and FAT words a VMU has at most.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define KOS_PET_PS1_NAME "BASLUSP01013"
#define KOS_GAME_PATH "/cd/VMU/RRL.VMS"
#define KOS_DIR_ENTRIES 256
#define KOS_FAT_WORDS 256

/*----------------------
 | s_game / s_game_size / s_replace_ok / s_root / s_fat / s_dir / s_dirents / s_dirent_count / s_pet_present
 | Description: The GAME read from the disc once, whether each slot's next pet write may replace another minigame, and the last VMU layout read_layout took.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void *s_game;
static int s_game_size;
static int s_replace_ok[2];
static vmu_root_t s_root;
static uint16 s_fat[KOS_FAT_WORDS];
static vmu_dir_t s_dir[KOS_DIR_ENTRIES];
static vmu_game_dirent s_dirents[KOS_DIR_ENTRIES];
static int s_dirent_count;
static int s_pet_present;

/*----------------------
 | game_image
 | Description: Loads Ring Ring Land's GAME from the disc on first use.
 | Author: suinevere
 | Dependencies: fs_load
 | Globals: s_game, s_game_size
 | Params: N/A
 | Returns: its size in blocks, or -1 when the disc has none
 ----------------------*/
static int game_image(void)
{
    if (s_game == NULL)
    {
        ssize_t size = fs_load(KOS_GAME_PATH, &s_game);

        if (size <= 0)
        {
            s_game = NULL;
            return -1;
        }
        s_game_size = (int)size;
    }
    return (s_game_size + VMU_BLOCK_BYTES - 1) / VMU_BLOCK_BYTES;
}

/*----------------------
 | read_tables
 | Description: One try at a VMU's root, directory and FAT under vmufs's mutex, which vmufs_dir_read and vmufs_fat_read expect held; vmufs_dir_blocks and vmufs_fat_blocks give bytes, not blocks.
 | Author: suinevere
 | Dependencies: vmufs_mutex_lock, vmufs_root_read, vmu_root_usable, vmufs_dir_blocks, vmufs_fat_blocks, vmufs_dir_read, vmufs_fat_read, vmufs_mutex_unlock
 | Globals: s_root, s_dir, s_fat
 | Params: dev -- VMU
 | Returns: 0, or -1 when a read fails or the tables are larger than a VMU's
 ----------------------*/
static int read_tables(maple_device_t *dev)
{
    int rv = -1;

    vmufs_mutex_lock();
    if (vmufs_root_read(dev, &s_root) >= 0 && vmu_root_usable((const unsigned char *)&s_root) && vmufs_dir_blocks(&s_root) <= (int)sizeof s_dir && vmufs_fat_blocks(&s_root) <= (int)sizeof s_fat
        && s_root.blk_cnt <= KOS_FAT_WORDS && vmufs_dir_read(dev, &s_root, s_dir) >= 0 && vmufs_fat_read(dev, &s_root, s_fat) >= 0)
    {
        rv = 0;
    }
    vmufs_mutex_unlock();
    return rv;
}

/*----------------------
 | read_layout
 | Description: Reads a VMU's root, directory and FAT and lists its used entries for vmu_game_classify, noting whether the pet transfer file is among them.
 | Author: suinevere
 | Dependencies: read_tables, vmufs_dir_blocks, vmu_retry, vmu_name_for_ps1
 | Globals: s_root, s_fat, s_dir, s_dirents, s_dirent_count, s_pet_present
 | Params: dev -- VMU
 | Returns: 0, or -1 when the VMU cannot be read or its tables are larger than a VMU's
 ----------------------*/
static int read_layout(maple_device_t *dev)
{
    char pet_name[VMU_NAME_LEN + 1];
    int attempt = 0;
    int entries;

    while (read_tables(dev) != 0)
    {
        if (!vmu_retry(&attempt))
        {
            return -1;
        }
    }
    entries = vmufs_dir_blocks(&s_root) / (int)sizeof(vmu_dir_t);
    vmu_name_for_ps1(KOS_PET_PS1_NAME, pet_name);
    s_dirent_count = 0;
    s_pet_present = 0;
    for (int i = 0; i < entries; i++)
    {
        vmu_game_dirent *d = &s_dirents[s_dirent_count];

        if (s_dir[i].filetype == 0)
        {
            continue;
        }
        memcpy(d->name, s_dir[i].filename, 12);
        d->name[12] = 0;
        d->type = s_dir[i].filetype;
        d->first = s_dir[i].firstblk;
        d->blocks = s_dir[i].filesize;
        if (strcmp(d->name, pet_name) == 0)
        {
            s_pet_present = 1;
        }
        s_dirent_count++;
    }
    return 0;
}

/*----------------------
 | layout_state
 | Description: What a VMU holds for the GAME, from a fresh read_layout and vmu_game_classify.
 | Author: suinevere
 | Dependencies: game_image, read_layout, vmu_game_classify
 | Globals: s_root, s_fat, s_dirents, s_dirent_count, s_pet_present
 | Params: dev -- VMU; found -- receives the GAME entry's index in s_dirents, or -1
 | Returns: a VMU_GAME_* value; VMU_GAME_UNSUPPORTED when the disc has no GAME or the VMU cannot be read
 ----------------------*/
static int layout_state(maple_device_t *dev, int *found)
{
    int blocks = game_image();

    *found = -1;
    if (dev == NULL || blocks < 0 || read_layout(dev) != 0)
    {
            return VMU_GAME_UNSUPPORTED;
    }
    return vmu_game_classify(s_fat, s_root.blk_cnt, s_dirents, s_dirent_count, blocks, s_pet_present ? 0 : VMU_GAME_PET_BLOCKS, found);
}

/*----------------------
 | kos_game_state / kos_game_allow_replace
 | Description: The store's game_state and game_allow_replace: what the slot's VMU holds for the GAME, and leave to replace another minigame on the next pet write.
 | Author: suinevere
 | Dependencies: layout_state, slot_device
 | Globals: s_replace_ok
 | Params: slot -- 0 or 1
 | Returns: kos_game_state -- a VMU_GAME_* value; kos_game_allow_replace -- N/A
 ----------------------*/
static int kos_game_state(int slot)
{
    int found;

    return layout_state(slot_device(slot), &found);
}

static void kos_game_allow_replace(int slot)
{
    s_replace_ok[slot & 1] = 1;
}

/*----------------------
 | game_matches
 | Description: Whether the VMU's Ring Ring Land GAME holds the disc's bytes.
 | Author: suinevere
 | Dependencies: vmufs_read
 | Globals: s_game, s_game_size
 | Params: dev -- VMU
 | Returns: nonzero when it does
 ----------------------*/
static int game_matches(maple_device_t *dev)
{
    void *data = NULL;
    int size = 0;
    int same;

    if (vmufs_read(dev, VMU_GAME_FILE, &data, &size) < 0 || data == NULL)
    {
        return 0;
    }
    same = size >= s_game_size && memcmp(data, s_game, (size_t)s_game_size) == 0;
    free(data);
    return same;
}

/*----------------------
 | game_placed
 | Description: Whether the GAME just written sits from block 0 in one unbroken chain of its size, as the VMU BIOS runs it.
 | Author: suinevere
 | Dependencies: read_layout, game_image
 | Globals: s_fat, s_dirents, s_dirent_count
 | Params: dev -- VMU
 | Returns: nonzero when it does
 ----------------------*/
static int game_placed(maple_device_t *dev)
{
    int blocks = game_image();

    if (read_layout(dev) != 0)
    {
        return 0;
    }
    for (int i = 0; i < s_dirent_count; i++)
    {
        if (strcmp(s_dirents[i].name, VMU_GAME_FILE) != 0)
        {
            continue;
        }
        if (s_dirents[i].first != 0 || s_dirents[i].blocks != blocks)
        {
            return 0;
        }
        for (int b = 0; b < blocks; b++)
        {
            if (s_fat[b] != (b + 1 < blocks ? (uint16)(b + 1) : (uint16)VMU_GAME_FAT_END))
            {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

/*----------------------
 | game_prepare
 | Description: Before the pet is written: installs the GAME when missing or older, replaces another minigame only when allowed, and refuses when it cannot fit; a disc or VMU without GAME support lets the pet go alone.
 | Author: suinevere
 | Dependencies: layout_state, game_matches, vmufs_delete, vmufs_write, vmu_retry, game_placed
 | Globals: s_replace_ok, s_dirents, s_game, s_game_size
 | Params: slot -- 0 or 1; dev -- its VMU
 | Returns: 0 to go on with the pet, -1 to fail the write
 ----------------------*/
static int game_prepare(int slot, maple_device_t *dev)
{
    int found;
    int state = layout_state(dev, &found);
    int allowed = s_replace_ok[slot & 1];
    int attempt = 0;

    s_replace_ok[slot & 1] = 0;
    if (state == VMU_GAME_UNSUPPORTED)
    {
        return 0;
    }
    if (state == VMU_GAME_NO_ROOM || (state == VMU_GAME_OTHER && !allowed))
    {
        return -1;
    }
    if (state == VMU_GAME_OURS && game_matches(dev))
    {
        return 0;
    }
    if (state == VMU_GAME_OTHER && vmufs_delete(dev, s_dirents[found].name) < 0)
    {
        return -1;
    }
    while (vmufs_write(dev, VMU_GAME_FILE, s_game, s_game_size, VMUFS_VMUGAME | VMUFS_OVERWRITE) < 0)
    {
        if (!vmu_retry(&attempt))
        {
            return -1;
        }
    }
    if (!game_placed(dev))
    {
        vmufs_delete(dev, VMU_GAME_FILE);
        return -1;
    }
    return 0;
}

/*----------------------
 | kos_write
 | Description: Stores a PS1 file as a VMS package, replacing any file of the same name; the pet transfer file goes only after game_prepare has put Ring Ring Land's GAME on the VMU.
 | Author: suinevere
 | Dependencies: slot_device, card_usable, game_prepare, vmu_name_for_ps1, build_package, vmufs_write, vmu_retry
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

    if (!card_usable(dev) || (strcmp(ps1_name, KOS_PET_PS1_NAME) == 0 && game_prepare(slot, dev) != 0) || build_package(ps1_name, buf, size, &out, &out_size) != 0)
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
    char replaced[CARD_STORE_NAME_BYTES];
    const unsigned char *buf;
    unsigned int size;
} s_job;
static volatile int s_job_state;

static int kos_remove(int slot, const char *ps1_name);

/*----------------------
 | run_job
 | Description: The background thread: runs kos_write for s_job, then removes the file it replaces, leaving the game thread to draw while the Maple bus carries the blocks.
 | Author: suinevere
 | Dependencies: kos_write, kos_remove, heap_probe
 | Globals: s_job, s_job_state
 | Params: param -- unused
 | Returns: NULL
 ----------------------*/
static void *run_job(void *param)
{
    (void)param;
    heap_probe("job write");
    if (kos_write(s_job.slot, s_job.ps1_name, s_job.buf, s_job.size) != 0)
    {
        s_job_state = JOB_FAILED;
        return NULL;
    }
    if (s_job.replaced[0] != 0)
    {
        kos_remove(s_job.slot, s_job.replaced);
    }
    heap_probe("job done");
    s_job_state = JOB_DONE;
    return NULL;
}

/*----------------------
 | kos_write_start / kos_job_poll
 | Description: Start kos_write, then the removal of the file it replaces, on a detached thread, one at a time, and report it: 1 while busy (yielding to it), then 0 or -1 once.
 | Author: suinevere
 | Dependencies: thd_create, thd_pass, run_job
 | Globals: s_job, s_job_state
 | Params: slot, ps1_name, buf, size -- as kos_write; buf must stay put until the poll stops returning 1; replaced -- PS1 name to remove after the write, or NULL
 | Returns: kos_write_start -- 0 once started, -1 when busy or no thread; kos_job_poll -- 1, 0 or -1
 ----------------------*/
static int kos_write_start(int slot, const char *ps1_name, const unsigned char *buf, unsigned int size, const char *replaced)
{
    if (s_job_state != JOB_IDLE || slot_device(slot) == NULL)
    {
        return -1;
    }
    s_job.slot = slot;
    strncpy(s_job.ps1_name, ps1_name, sizeof s_job.ps1_name - 1);
    s_job.ps1_name[sizeof s_job.ps1_name - 1] = 0;
    s_job.replaced[0] = 0;
    if (replaced != NULL && strcmp(replaced, ps1_name) != 0)
    {
        strncpy(s_job.replaced, replaced, sizeof s_job.replaced - 1);
        s_job.replaced[sizeof s_job.replaced - 1] = 0;
    }
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

/*----------------------
 | card_store_kos_busy
 | Description: Whether a background VMU write is still running, which the soft reset waits out so it never cuts a save short.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_job_state
 | Params: N/A
 | Returns: 1 while a write runs, else 0
 ----------------------*/
int card_store_kos_busy(void)
{
    return s_job_state == JOB_BUSY;
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
 | Dependencies: slot_device, card_usable, vmu_name_for_ps1, vmufs_delete, vmu_retry
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

    if (!card_usable(dev))
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
 | Description: Free user blocks on the slot's VMU, read from its root and FAT here rather than through vmufs_free_blocks, so a card whose root vmufs cannot hold counts as full instead of overrunning the heap.
 | Author: suinevere
 | Dependencies: slot_device, vmufs_mutex_lock, vmufs_root_read, vmu_root_usable, vmufs_fat_read, vmufs_fat_free, vmufs_mutex_unlock, vmu_retry
 | Globals: N/A
 | Params: slot -- 0 or 1
 | Returns: free blocks, 0 with no usable VMU
 ----------------------*/
static int kos_free_blocks(int slot)
{
    static uint16 fat[KOS_FAT_WORDS];
    maple_device_t *dev = slot_device(slot);
    vmu_root_t root;
    int attempt = 0;
    int blocks;

    if (dev == NULL)
    {
        return 0;
    }
    for (;;)
    {
        vmufs_mutex_lock();
        blocks = -1;
        if (vmufs_root_read(dev, &root) >= 0)
        {
            blocks = !vmu_root_usable((const unsigned char *)&root) ? 0 : vmufs_fat_read(dev, &root, fat) >= 0 ? vmufs_fat_free(&root, fat) : -1;
        }
        vmufs_mutex_unlock();
        if (blocks >= 0 || !vmu_retry(&attempt))
        {
            return blocks < 0 ? 0 : blocks;
        }
    }
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
#ifdef LOM_HEAP_PROBE
/*----------------------
 | probe_present / probe_list / probe_read / probe_write / probe_remove / probe_free_blocks / probe_write_start / probe_job_poll / probe_game_state / probe_game_allow_replace
 | Description: The store's operations with a heap checkpoint on entry and on return, so a fault names the operation that damaged the heap.
 | Author: suinevere
 | Dependencies: heap_probe, kos_present, kos_list, kos_read, kos_write, kos_remove, kos_free_blocks, kos_write_start, kos_job_poll, kos_game_state, kos_game_allow_replace
 | Globals: N/A
 | Params: as the wrapped operation
 | Returns: as the wrapped operation
 ----------------------*/
#define PROBED(name, call)     do     {         int result_;         heap_probe(name " in");         result_ = (call);         heap_probe(name " out");         return result_;     } while (0)

static int probe_present(int slot) { PROBED("present", kos_present(slot)); }
static int probe_list(int slot, card_store_entry *out, int max) { PROBED("list", kos_list(slot, out, max)); }
static int probe_read(int slot, const char *ps1_name, unsigned char *buf, unsigned int max) { PROBED("read", kos_read(slot, ps1_name, buf, max)); }
static int probe_write(int slot, const char *ps1_name, const unsigned char *buf, unsigned int size) { PROBED("write", kos_write(slot, ps1_name, buf, size)); }
static int probe_remove(int slot, const char *ps1_name) { PROBED("remove", kos_remove(slot, ps1_name)); }
static int probe_free_blocks(int slot) { PROBED("free", kos_free_blocks(slot)); }
static int probe_write_start(int slot, const char *ps1_name, const unsigned char *buf, unsigned int size, const char *replaced) { PROBED("start", kos_write_start(slot, ps1_name, buf, size, replaced)); }
static int probe_job_poll(void) { PROBED("poll", kos_job_poll()); }
static int probe_game_state(int slot) { PROBED("game", kos_game_state(slot)); }

static void probe_game_allow_replace(int slot)
{
    heap_probe("allow");
    kos_game_allow_replace(slot);
}

const card_store_ops card_store_kos = {probe_present, probe_list, probe_read, probe_write, probe_remove, probe_free_blocks, probe_write_start, probe_job_poll, NULL, probe_game_state, probe_game_allow_replace};
#else
const card_store_ops card_store_kos = {kos_present, kos_list, kos_read, kos_write, kos_remove, kos_free_blocks, kos_write_start, kos_job_poll, NULL, kos_game_state, kos_game_allow_replace};
#endif

/*----------------------
 | kos_lcd_draw
 | Description: vmu_screen's draw hook: puts a screen on every VMU, or reports busy while the background thread writes a file, so the LCD command never shares the bus with a transfer, and when a VMU refuses the frame (its maple frame still taken), so the next VSync tries again.
 | Author: suinevere
 | Dependencies: maple_enum_type, vmu_draw_lcd_rotated, heap_probe
 | Globals: s_job_state
 | Params: lcd -- 192 bytes, 4-byte aligned
 | Returns: 0 once drawn on every VMU, 1 when busy
 ----------------------*/
static int kos_lcd_draw(const unsigned char *lcd)
{
    maple_device_t *dev;
    int i;
    int busy = 0;

    if (s_job_state == JOB_BUSY)
    {
        return 1;
    }
    heap_probe("lcd in");
    for (i = 0; (dev = maple_enum_type(i, MAPLE_FUNC_LCD)) != NULL; i++)
    {
        if (vmu_draw_lcd_rotated(dev, (void *)lcd) < 0)
        {
            busy = 1;
        }
    }
    heap_probe("lcd out");
    return busy;
}

/*----------------------
 | card_store_kos_lcds
 | Description: How many VMU screens maple finds now, so vmu_screen redraws for one plugged in later.
 | Author: suinevere
 | Dependencies: maple_enum_type
 | Globals: N/A
 | Params: N/A
 | Returns: the count
 ----------------------*/
int card_store_kos_lcds(void)
{
    int i = 0;

    while (maple_enum_type(i, MAPLE_FUNC_LCD) != NULL)
    {
        i++;
    }
    return i;
}

/*----------------------
 | card_store_kos_init
 | Description: Installs the VMU store behind the card calls, gives vmu_screen the pet screens and the draw hook, and puts the first choice (the wordmark until a pet is known) on every VMU found at boot.
 | Author: suinevere
 | Dependencies: card_vmu_set_store, vmu_screen_set_screens, vmu_screen_set_draw, vmu_screen_update, kos_lcd_draw, card_store_kos_lcds
 | Globals: lom_vmu_pet_screens, lom_vmu_pet_screen_count
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void card_store_kos_init(void)
{
    card_vmu_set_store(&card_store_kos);
    vmu_screen_set_screens(lom_vmu_pet_screens, lom_vmu_pet_screen_count);
    vmu_screen_set_draw(kos_lcd_draw);
    vmu_screen_update(NULL, card_store_kos_lcds());
}
