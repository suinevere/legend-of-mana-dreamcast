/*----------------------
 | disc_fs_kos.c
 | Description: Read-only ISO9660 filesystem at /cd. It mounts a GD-ROM's high-density track (this KallistiOS only reads the low-density TOC), and reads through a per-file read-ahead buffer or straight into the caller in multi-sector PIO commands, since one command per sector starves movies on a real drive.
 | Author: suinevere
 | Dependencies: dc_system.h, disc_fs_kos.h, iso_dir.h, kos/fs.h, kos/mutex.h, kos/nmmgr.h, kos/thread.h, dc/cdrom.h, dc/fs_iso9660.h, malloc.h, errno.h, stdio.h, string.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <errno.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dc/cdrom.h>
#include <dc/fs_iso9660.h>
#include <kos/fs.h>
#include <kos/mutex.h>
#include <kos/nmmgr.h>
#include <kos/thread.h>

#include "dc_system.h"
#include "disc_fs_kos.h"
#include "iso_dir.h"

/*----------------------
 | FAD_BIAS / READ_AHEAD_SECTORS / COMMAND_SECTORS / DIR_CACHE_SECTORS
 | Description: Frame address of logical sector 0, sectors one buffered refill reads, the most sectors one drive command reads, and directory sectors kept.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FAD_BIAS 150u
#define READ_AHEAD_SECTORS 16u
#define COMMAND_SECTORS 64u
#define DIR_CACHE_SECTORS 16u

/*----------------------
 | GD_DATA_LBA / READY_WAIT_MS / READY_POLL_MS / MOUNT_ATTEMPTS
 | Description: Where every GD-ROM image's high-density data track starts, how long to wait for the drive to report a disc, how often to ask, and mount passes before giving up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GD_DATA_LBA 45000u
#define READY_WAIT_MS 10000
#define READY_POLL_MS 50
#define MOUNT_ATTEMPTS 3

/*----------------------
 | DiscFile
 | Description: One open file: where it lives, the read position, and its read-ahead buffer with the first sector and count it holds.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct DiscFile
{
    unsigned extent;
    unsigned size;
    unsigned pos;
    unsigned char *buffer;
    unsigned buffer_sector;
    unsigned buffer_count;
} DiscFile;

/*----------------------
 | s_mutex / s_mounted / s_root
 | Description: Guards the mount and directory cache, whether the data track is mounted, and its root directory.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static mutex_t s_mutex;
static int s_mounted;
static IsoEntry s_root;

/*----------------------
 | s_status
 | Description: What the last mount found at each step, for the halt screen when the disc cannot be read.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static char s_status[160] = "not mounted";

/*----------------------
 | s_dir_cache / s_dir_lba / s_dir_next
 | Description: Recently read directory sectors, their logical sector numbers (0 for empty), and the slot the next miss replaces.
 | Author: suinevere
 | Dependencies: DIR_CACHE_SECTORS, ISO_SECTOR_BYTES
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_dir_cache[DIR_CACHE_SECTORS][ISO_SECTOR_BYTES] __attribute__((aligned(32)));
static unsigned s_dir_lba[DIR_CACHE_SECTORS];
static unsigned s_dir_next;

/*----------------------
 | read_sectors
 | Description: Reads whole sectors by PIO, retrying once after waiting for the drive; DMA is left alone because this KallistiOS does no cache upkeep for it.
 | Author: suinevere
 | Dependencies: cdrom_read_sectors_ex, wait_ready
 | Globals: N/A
 | Params: dst -- destination, lba -- first logical sector, count -- sectors
 | Returns: 0, or -1 on a read error
 ----------------------*/
static int wait_ready(int *status, int *type);

static int read_sectors(void *dst, unsigned lba, unsigned count)
{
    int status;
    int type;

    if (cdrom_read_sectors_ex(dst, (int)(lba + FAD_BIAS), (int)count, CDROM_READ_PIO) == ERR_OK)
        return 0;
    wait_ready(&status, &type);
    return cdrom_read_sectors_ex(dst, (int)(lba + FAD_BIAS), (int)count, CDROM_READ_PIO) == ERR_OK ? 0 : -1;
}

/*----------------------
 | wait_ready
 | Description: Polls the drive until it reports a disc it can read (paused, standby or playing), sleeping between asks, for up to READY_WAIT_MS.
 | Author: suinevere
 | Dependencies: cdrom_get_status, thd_sleep
 | Globals: N/A
 | Params: status -- receives the last drive status, type -- receives the last disc type
 | Returns: milliseconds waited, or -1 when the drive never became ready
 ----------------------*/
static int wait_ready(int *status, int *type)
{
    int waited;

    for (waited = 0; waited <= READY_WAIT_MS; waited += READY_POLL_MS)
    {
        *status = -1;
        *type = -1;
        cdrom_get_status(status, type);
        if (*status == CD_STATUS_PAUSED || *status == CD_STATUS_STANDBY || *status == CD_STATUS_PLAYING)
            return waited;
        thd_sleep(READY_POLL_MS);
    }
    return -1;
}

/*----------------------
 | try_pvd
 | Description: Checks that a logical sector is an ISO9660 primary volume descriptor, taking its root directory if so.
 | Author: suinevere
 | Dependencies: read_sectors, iso_pvd_root
 | Globals: s_root
 | Params: lba -- the sector, read_rv -- receives the read result
 | Returns: iso_pvd_root's result, or -9 when the read failed
 ----------------------*/
static int try_pvd(unsigned lba, int *read_rv)
{
    static unsigned char pvd[ISO_SECTOR_BYTES] __attribute__((aligned(32)));

    *read_rv = read_sectors(pvd, lba, 1);
    return *read_rv == 0 ? iso_pvd_root(pvd, &s_root) : -9;
}

/*----------------------
 | try_area
 | Description: Reads one disc area's TOC and checks that its last data track starts with an ISO9660 volume descriptor, taking its root directory if so.
 | Author: suinevere
 | Dependencies: cdrom_read_toc, cdrom_locate_data_track, try_pvd, snprintf
 | Globals: N/A
 | Params: area -- 1 for a GD-ROM's high-density area, 0 for the first session, result -- receives the step codes, size -- its capacity
 | Returns: 0 when the area holds the filesystem, else -1
 ----------------------*/
static int try_area(int area, char *result, size_t size)
{
    CDROM_TOC toc;
    int toc_rv;
    int read_rv = -9;
    int pvd_rv = -9;
    unsigned fad = 0;

    toc_rv = cdrom_read_toc(&toc, area);
    if (toc_rv == ERR_OK)
        fad = cdrom_locate_data_track(&toc);
    if (fad >= FAD_BIAS)
        pvd_rv = try_pvd(fad - FAD_BIAS + 16u, &read_rv);
    snprintf(result, size, "A%d t%d f%u r%d p%d", area, toc_rv, fad, read_rv, pvd_rv);
    return pvd_rv == 0 ? 0 : -1;
}

/*----------------------
 | mount
 | Description: Waits for the drive to report a disc, then finds the filesystem: the high-density area's TOC, then the standard GD data track start read directly, then the first session (a MIL-CD).
 |              Tries a few passes, reinitialising the drive between them, and records every step in s_status.
 | Author: suinevere
 | Dependencies: wait_ready, cdrom_reinit, try_area, try_pvd, thd_sleep, snprintf
 | Globals: s_mounted, s_dir_lba, s_status
 | Params: N/A
 | Returns: 0, or -1 when no ISO9660 data track is readable
 ----------------------*/
static int mount(void)
{
    int attempt;

    for (attempt = 1; attempt <= MOUNT_ATTEMPTS; attempt++)
    {
        char high[40];
        char low[40] = "-";
        int status;
        int type;
        int reinit = attempt == 1 ? 0 : cdrom_reinit();
        int waited = wait_ready(&status, &type);
        int probe_read = -9;
        int probe_pvd = -9;
        int found = try_area(1, high, sizeof(high)) == 0;

        if (!found)
            found = (probe_pvd = try_pvd(GD_DATA_LBA + 16u, &probe_read)) == 0;
        if (!found)
            found = try_area(0, low, sizeof(low)) == 0;
        snprintf(s_status, sizeof(s_status), "try%d init%d wait%d st%d ty%02X\n%s\nGD r%d p%d\n%s", attempt, reinit, waited, status,
                 (unsigned)type & 0xFFu, high, probe_read, probe_pvd, low);
        if (found)
        {
            memset(s_dir_lba, 0, sizeof(s_dir_lba));
            s_mounted = 1;
            return 0;
        }
        thd_sleep(500);
    }
    return -1;
}

/*----------------------
 | dir_sector
 | Description: One directory sector, from the cache or read into the next slot; the caller holds s_mutex.
 | Author: suinevere
 | Dependencies: read_sectors
 | Globals: s_dir_cache, s_dir_lba, s_dir_next
 | Params: lba -- logical sector
 | Returns: the sector, or NULL on a read error
 ----------------------*/
static const unsigned char *dir_sector(unsigned lba)
{
    unsigned slot;

    for (slot = 0; slot < DIR_CACHE_SECTORS; slot++)
        if (s_dir_lba[slot] == lba)
            return s_dir_cache[slot];
    slot = s_dir_next;
    s_dir_next = (s_dir_next + 1u) % DIR_CACHE_SECTORS;
    s_dir_lba[slot] = 0;
    if (read_sectors(s_dir_cache[slot], lba, 1) != 0)
        return NULL;
    s_dir_lba[slot] = lba;
    return s_dir_cache[slot];
}

/*----------------------
 | lookup
 | Description: Walks a slash-separated path from the root directory; the caller holds s_mutex.
 | Author: suinevere
 | Dependencies: dir_sector, iso_dir_find
 | Globals: s_root
 | Params: path -- path under /cd, found -- receives the entry
 | Returns: 0, or -1 when a component is missing
 ----------------------*/
static int lookup(const char *path, IsoEntry *found)
{
    IsoEntry current = s_root;

    while (*path != 0)
    {
        const char *end;
        unsigned sector;
        int hit = 0;

        while (*path == '/')
            path++;
        if (*path == 0)
            break;
        end = path;
        while (*end != 0 && *end != '/')
            end++;
        if (!current.is_dir)
            return -1;
        for (sector = 0; !hit && sector * ISO_SECTOR_BYTES < current.size; sector++)
        {
            const unsigned char *data = dir_sector(current.extent + sector);

            if (data == NULL)
                return -1;
            hit = iso_dir_find(data, path, (unsigned)(end - path), &current);
        }
        if (!hit)
            return -1;
        path = end;
    }
    *found = current;
    return 0;
}

/*----------------------
 | disc_open
 | Description: Opens a file for reading, mounting the disc first if needed; directories and writes are refused.
 | Author: suinevere
 | Dependencies: mount, lookup, mutex_lock, mutex_unlock
 | Globals: s_mutex, s_mounted
 | Params: vfs -- unused, fn -- path under /cd, mode -- open flags
 | Returns: the file, or NULL
 ----------------------*/
static void *disc_open(vfs_handler_t *vfs, const char *fn, int mode)
{
    DiscFile *file;
    IsoEntry entry;
    int found;

    (void)vfs;
    if ((mode & O_MODE_MASK) != O_RDONLY || (mode & O_DIR))
    {
        errno = EROFS;
        return NULL;
    }
    mutex_lock(&s_mutex);
    found = (s_mounted || mount() == 0) && lookup(fn, &entry) == 0 && !entry.is_dir;
    mutex_unlock(&s_mutex);
    if (!found)
    {
        errno = ENOENT;
        return NULL;
    }
    file = (DiscFile *)calloc(1, sizeof(*file));
    if (file == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }
    file->extent = entry.extent;
    file->size = entry.size;
    return file;
}

/*----------------------
 | disc_close
 | Description: Frees a file and its read-ahead buffer.
 | Author: suinevere
 | Dependencies: free
 | Globals: N/A
 | Params: hnd -- file
 | Returns: 0
 ----------------------*/
static int disc_close(void *hnd)
{
    DiscFile *file = (DiscFile *)hnd;

    free(file->buffer);
    free(file);
    return 0;
}

/*----------------------
 | refill
 | Description: Reads up to READ_AHEAD_SECTORS of the file from a sector into its buffer, allocating the buffer the first time.
 | Author: suinevere
 | Dependencies: memalign, read_sectors
 | Globals: N/A
 | Params: file -- file, sector -- file-relative sector
 | Returns: 0, or -1 on an allocation or read error
 ----------------------*/
static int refill(DiscFile *file, unsigned sector)
{
    unsigned sectors = (file->size + ISO_SECTOR_BYTES - 1u) / ISO_SECTOR_BYTES;
    unsigned count = sectors - sector < READ_AHEAD_SECTORS ? sectors - sector : READ_AHEAD_SECTORS;

    if (file->buffer == NULL)
        file->buffer = (unsigned char *)memalign(32, READ_AHEAD_SECTORS * ISO_SECTOR_BYTES);
    if (file->buffer == NULL)
        return -1;
    file->buffer_count = 0;
    if (read_sectors(file->buffer, file->extent + sector, count) != 0)
        return -1;
    file->buffer_sector = sector;
    file->buffer_count = count;
    return 0;
}

/*----------------------
 | disc_read
 | Description: Reads from the position: whole sectors bound for an aligned destination go straight there, the rest through the read-ahead buffer.
 | Author: suinevere
 | Dependencies: read_sectors, refill
 | Globals: N/A
 | Params: hnd -- file, buffer -- destination, cnt -- bytes wanted
 | Returns: bytes read, 0 at the end, or -1 on a read error before any byte
 ----------------------*/
static ssize_t disc_read(void *hnd, void *buffer, size_t cnt)
{
    DiscFile *file = (DiscFile *)hnd;
    unsigned char *out = (unsigned char *)buffer;
    size_t done = 0;

    if (file->pos >= file->size)
        return 0;
    if (cnt > file->size - file->pos)
        cnt = file->size - file->pos;
    while (done < cnt)
    {
        unsigned sector = file->pos / ISO_SECTOR_BYTES;
        unsigned offset = file->pos % ISO_SECTOR_BYTES;
        size_t left = cnt - done;
        size_t run;

        if (offset == 0 && left >= ISO_SECTOR_BYTES && ((uintptr_t)(out + done) & 31u) == 0)
        {
            unsigned count = (unsigned)(left / ISO_SECTOR_BYTES);

            if (count > COMMAND_SECTORS)
                count = COMMAND_SECTORS;
            if (read_sectors(out + done, file->extent + sector, count) != 0)
                break;
            run = (size_t)count * ISO_SECTOR_BYTES;
        }
        else
        {
            size_t held;

            if ((file->buffer_count == 0 || sector < file->buffer_sector || sector >= file->buffer_sector + file->buffer_count)
                && refill(file, sector) != 0)
                break;
            held = (size_t)(file->buffer_sector + file->buffer_count - sector) * ISO_SECTOR_BYTES - offset;
            run = left < held ? left : held;
            memcpy(out + done, file->buffer + (size_t)(sector - file->buffer_sector) * ISO_SECTOR_BYTES + offset, run);
        }
        file->pos += (unsigned)run;
        done += run;
    }
    if (done == 0 && cnt != 0)
    {
        errno = EIO;
        return -1;
    }
    return (ssize_t)done;
}

/*----------------------
 | disc_seek / disc_tell / disc_total
 | Description: Move the position (clamped to the file), report it, and report the file's size.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: hnd -- file, offset / whence -- as lseek
 | Returns: the new position, the position, or the size
 ----------------------*/
static off_t disc_seek(void *hnd, off_t offset, int whence)
{
    DiscFile *file = (DiscFile *)hnd;
    long long target = whence == SEEK_SET ? 0 : whence == SEEK_CUR ? (long long)file->pos : (long long)file->size;

    target += (long long)offset;
    if (target < 0)
        target = 0;
    if (target > (long long)file->size)
        target = file->size;
    file->pos = (unsigned)target;
    return (off_t)file->pos;
}

static off_t disc_tell(void *hnd)
{
    return (off_t)((DiscFile *)hnd)->pos;
}

static size_t disc_total(void *hnd)
{
    return ((DiscFile *)hnd)->size;
}

/*----------------------
 | s_vfs
 | Description: The /cd handler table.
 | Author: suinevere
 | Dependencies: disc_open, disc_close, disc_read, disc_seek, disc_tell, disc_total
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static vfs_handler_t s_vfs = {
    .nmmgr = {.pathname = "/cd", .pid = 0, .version = 0x00010000, .flags = 0, .type = NMMGR_TYPE_VFS, .list_ent = NMMGR_LIST_INIT},
    .cache = 0,
    .privdata = NULL,
    .open = disc_open,
    .close = disc_close,
    .read = disc_read,
    .seek = disc_seek,
    .tell = disc_tell,
    .total = disc_total,
};

const char *disc_fs_kos_status(void)
{
    return s_status;
}

void disc_fs_kos_init(void)
{
    fs_iso9660_shutdown();
    mutex_init(&s_mutex, MUTEX_TYPE_NORMAL);
    nmmgr_handler_add(&s_vfs.nmmgr);
    mutex_lock(&s_mutex);
    mount();
    mutex_unlock(&s_mutex);
    dc_log("disc %s", s_status);
}
