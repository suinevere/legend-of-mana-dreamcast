/*----------------------
 | test_card_vmu.c
 | Description: Host test for card_vmu.c over an in-memory store: absent and empty slots, the temp-file-then-rename save, overwrite, load, title patch, card header, card full, a pulled card, erase, libmcx answers and the GAME calls CARDA makes.
 | Author: suinevere
 | Dependencies: card_vmu.c, vmu_format.c, card_store.h, vmu_game.h, libapi.h, libmcx.h
 ----------------------*/
#include <string.h>
#include <libapi.h>
#include <libmcx.h>
#include "card_store.h"
#include "vmu_game.h"

int printf(const char *format, ...);
int puts(const char *text);
int func_80034648(long chan, long block, long count);
long func_8001680C(char *path, long mode);
long func_8001681C(long fd, void *buf, long n);
long func_8001683C(long fd);
char *func_80016F9C(char *dst, const char *src);
char *func_800170BC(char *dst, const char *src);

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); return 1; } } while (0)

#define F_READ 0x0001
#define F_WRITE 0x0002
#define F_CREAT 0x0200
#define F_ASYNC 0x8000
#define F_BLOCKS(n) ((n) << 16)
#define FAKE_FILES 16
#define FAKE_BYTES 0xC000

static struct
{
    int used;
    int slot;
    char name[CARD_STORE_NAME_BYTES];
    unsigned int size;
    int corrupt;
    unsigned char bytes[FAKE_BYTES];
} s_files[FAKE_FILES];
static int s_present[2];
static int s_free[2];
static int s_writes;
static int s_reads;

static int fake_find(int slot, const char *name)
{
    int i;

    for (i = 0; i < FAKE_FILES; i++)
    {
        if (s_files[i].used && s_files[i].slot == slot && strcmp(s_files[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int fake_count(int slot)
{
    int i;
    int n = 0;

    for (i = 0; i < FAKE_FILES; i++)
    {
        n += s_files[i].used && s_files[i].slot == slot;
    }
    return n;
}

static int fake_present(int slot)
{
    return s_present[slot];
}

static int fake_list(int slot, card_store_entry *out, int max)
{
    int i;
    int n = 0;

    if (!s_present[slot])
    {
        return -1;
    }
    for (i = 0; i < FAKE_FILES && n < max; i++)
    {
        if (s_files[i].used && s_files[i].slot == slot)
        {
            strcpy(out[n].ps1_name, s_files[i].name);
            out[n].size = s_files[i].size;
            n++;
        }
    }
    return n;
}

static int fake_read(int slot, const char *name, unsigned char *buf, unsigned int max)
{
    int i = fake_find(slot, name);

    s_reads++;
    if (!s_present[slot] || i < 0 || s_files[i].corrupt)
    {
        return -1;
    }
    memcpy(buf, s_files[i].bytes, s_files[i].size < max ? s_files[i].size : max);
    return (int)(s_files[i].size < max ? s_files[i].size : max);
}

static int fake_write(int slot, const char *name, const unsigned char *buf, unsigned int size)
{
    int i = fake_find(slot, name);

    if (!s_present[slot] || size > FAKE_BYTES)
    {
        return -1;
    }
    if (i < 0)
    {
        for (i = 0; i < FAKE_FILES && s_files[i].used; i++)
        {
        }
        if (i == FAKE_FILES)
        {
            return -1;
        }
    }
    s_files[i].used = 1;
    s_files[i].corrupt = 0;
    s_files[i].slot = slot;
    strcpy(s_files[i].name, name);
    s_files[i].size = size;
    memcpy(s_files[i].bytes, buf, size);
    s_writes++;
    return 0;
}

static int fake_remove(int slot, const char *name)
{
    int i = fake_find(slot, name);

    if (!s_present[slot] || i < 0)
    {
        return -1;
    }
    s_files[i].used = 0;
    return 0;
}

static int fake_free_blocks(int slot)
{
    return s_free[slot];
}

/*----------------------
 | s_job / s_busy_polls / s_rewrites / fake_write_start / fake_job_poll / fake_rewrite
 | Description: A background write that reports busy for s_busy_polls polls before landing through fake_write, and a rewrite that renames and refreshes a stored file in place.
 | Author: suinevere
 | Dependencies: fake_find, fake_write, fake_remove
 | Globals: s_job, s_busy_polls, s_rewrites, s_files
 | Params: slot, name, buf, size -- the file; old_name, new_name, offset, len -- the rewrite
 | Returns: 0 on success, -1 on failure; fake_job_poll -- 1 while busy
 ----------------------*/
static struct
{
    int active;
    int slot;
    char name[CARD_STORE_NAME_BYTES];
    const unsigned char *buf;
    unsigned int size;
} s_job;
static int s_busy_polls;
static int s_rewrites;

static int fake_write_start(int slot, const char *name, const unsigned char *buf, unsigned int size)
{
    if (s_job.active || !s_present[slot])
    {
        return -1;
    }
    s_job.active = 1;
    s_job.slot = slot;
    strcpy(s_job.name, name);
    s_job.buf = buf;
    s_job.size = size;
    return 0;
}

static int fake_job_poll(void)
{
    if (!s_job.active)
    {
        return 0;
    }
    if (s_busy_polls > 0)
    {
        s_busy_polls--;
        return 1;
    }
    s_job.active = 0;
    return fake_write(s_job.slot, s_job.name, s_job.buf, s_job.size);
}

static int fake_rewrite(int slot, const char *old_name, const char *new_name, const unsigned char *buf, unsigned int size, unsigned int offset, unsigned int len)
{
    int i = fake_find(slot, old_name);

    (void)offset;
    (void)len;
    if (!s_present[slot] || i < 0 || size != s_files[i].size)
    {
        return -1;
    }
    if (strcmp(old_name, new_name) != 0 && fake_find(slot, new_name) >= 0)
    {
        fake_remove(slot, new_name);
    }
    strcpy(s_files[i].name, new_name);
    memcpy(s_files[i].bytes, buf, size);
    s_rewrites++;
    return 0;
}

static const card_store_ops s_fake = {fake_present, fake_list, fake_read, fake_write, fake_remove, fake_free_blocks, fake_write_start, fake_job_poll, fake_rewrite};

static int s_game_state = VMU_GAME_MISSING;
static int s_game_allowed[2];

static int fake_game_state(int slot)
{
    return slot == 0 ? s_game_state : VMU_GAME_NO_ROOM;
}

static void fake_game_allow_replace(int slot)
{
    s_game_allowed[slot]++;
}

static const card_store_ops s_game_fake = {fake_present, fake_list, fake_read, fake_write, fake_remove, fake_free_blocks, fake_write_start, fake_job_poll, fake_rewrite,
                                           fake_game_state, fake_game_allow_replace};

static unsigned char s_blob[0x4000];

static int save(unsigned char fill)
{
    long fd;
    long ioe = OpenEvent(SwCARD, EvSpIOE, EvMdNOINTR, 0);
    int i;

    for (i = 0; i < (int)sizeof s_blob; i++)
    {
        s_blob[i] = fill ? fill : (unsigned char)i;
    }
    fd = lom_card_open("bu00:BASLUS-01013DUMMY", F_CREAT | F_BLOCKS(2));
    if (fd < 0 || lom_card_close(fd) != 0)
    {
        return 1;
    }
    fd = lom_card_open("bu00:BASLUS-01013DUMMY", F_ASYNC | F_WRITE);
    if (fd < 0 || lom_card_write(fd, s_blob, sizeof s_blob) != (long)sizeof s_blob || TestEvent(ioe) != 1)
    {
        return 2;
    }
    lom_card_close(fd);
    if (fake_find(0, "BASLUS-01013DUMMY") < 0)
    {
        return 3;
    }
    if (lom_card_rename("bu00:BASLUS-01013DUMMY", "bu00:BASLUS-01013LOM00") != 0)
    {
        return 4;
    }
    return 0;
}

int main(void)
{
    long sw_ioe = OpenEvent(SwCARD, EvSpIOE, EvMdNOINTR, 0);
    long sw_err = OpenEvent(SwCARD, EvSpERROR, EvMdNOINTR, 0);
    long sw_tmo = OpenEvent(SwCARD, EvSpTIMOUT, EvMdNOINTR, 0);
    long sw_new = OpenEvent(SwCARD, EvSpNEW, EvMdNOINTR, 0);
    long hw_ioe = OpenEvent(HwCARD, EvSpIOE, EvMdNOINTR, 0);
    long hw_tmo = OpenEvent(HwCARD, EvSpTIMOUT, EvMdNOINTR, 0);
    struct DIRENTRY entry;
    unsigned char frame[128];
    unsigned char back[0x4000];
    long cmd = 0;
    long result = -1;
    unsigned char time[8];
    long fd;
    int i;

    CHECK(_card_info(0) == 1 && TestEvent(sw_tmo) == 1 && TestEvent(hw_tmo) == 1);

    card_vmu_set_store(&s_fake);
    s_present[0] = 1;
    s_free[0] = 64;

    CHECK(_card_info(0x10) == 1);
    CHECK(TestEvent(sw_tmo) == 1 && TestEvent(hw_tmo) == 1 && TestEvent(sw_ioe) == 0);
    CHECK(lom_card_open("bu10:BASLUS-01013LOM00", F_READ) == -1);

    CHECK(_card_info(0) == 1);
    CHECK(TestEvent(sw_err) == 0 && TestEvent(sw_new) == 0);
    CHECK(TestEvent(sw_ioe) == 1 && TestEvent(sw_ioe) == 0);
    CHECK(TestEvent(hw_ioe) == 1 && TestEvent(hw_ioe) == 0);
    CHECK(_card_clear(0) == 1 && TestEvent(sw_ioe) == 1 && TestEvent(hw_ioe) == 1);
    CHECK(_card_load(0) == 1 && TestEvent(sw_ioe) == 1 && TestEvent(hw_ioe) == 1);
    CHECK(firstfile("bu00:BASLUS-01013*", &entry) == NULL);
    CHECK(firstfile("bu00:*", &entry) == &entry);
    CHECK(strcmp(entry.name, "BADCVMU-OTHER") == 0 && entry.size == 3 * 8192);
    CHECK(nextfile(&entry) == NULL);
    {
        unsigned char head[0x200];
        long filler_fd = lom_card_open("bu00:BADCVMU-OTHER", F_ASYNC | F_READ);

        memset(head, 0x5A, sizeof head);
        CHECK(filler_fd >= 0 && lom_card_read(filler_fd, head, sizeof head) == (long)sizeof head && head[0] == 0 && head[0x1FF] == 0);
        {
            long again = lom_card_open("bu00:BADCVMU-OTHER", F_READ);

            CHECK(again >= 0 && firstfile("bu00:*", &entry) == &entry && nextfile(&entry) == NULL);
            CHECK(lom_card_close(again) == 0);
        }
        CHECK(lom_card_close(filler_fd) == 0 && fake_count(0) == 0);
        CHECK(lom_card_open("bu00:BADCVMU-OTHER", F_WRITE) == -1);
        CHECK(lom_card_open("bu00:BADCVMU-OTHER", F_CREAT | F_BLOCKS(1)) == -1);
    }

    CHECK(save(0) == 0);
    CHECK(fake_count(0) == 1 && fake_find(0, "BASLUS-01013LOM00") >= 0);
    CHECK(s_files[fake_find(0, "BASLUS-01013LOM00")].bytes[0x123] == 0x23);

    CHECK(save(0xA5) == 0);
    CHECK(fake_count(0) == 1 && s_files[fake_find(0, "BASLUS-01013LOM00")].bytes[0x123] == 0xA5);

    CHECK(_card_load(0) == 1 && TestEvent(sw_ioe) == 1);
    CHECK(firstfile("bu00:BASLUS-01013*", &entry) == &entry);
    CHECK(strcmp(entry.name, "BASLUS-01013LOM00") == 0 && entry.size == 0x4000);
    CHECK(nextfile(&entry) == NULL);
    CHECK(firstfile("bu00:BASLUS-01013LOM0?", &entry) == &entry);
    CHECK(firstfile("bu00:BASLUS-01013LOM1?", &entry) == NULL);
    CHECK(firstfile("bu00:BASLUS-01013*", &entry) == &entry);
    fd = lom_card_open("bu00:BASLUS-01013LOM00", F_ASYNC | F_READ);
    CHECK(fd >= 0);
    memset(back, 0, sizeof back);
    CHECK(lom_card_read(fd, back, sizeof back) == (long)sizeof back && TestEvent(sw_ioe) == 1);
    CHECK(back[0] == 0xA5 && back[0x3FFF] == 0xA5);
    CHECK(lom_card_close(fd) == 0);

    CHECK(entry.head % 64 == 0 && entry.head >= 64);
    CHECK(_card_read(0, entry.head, frame) == 1 && TestEvent(hw_ioe) == 1);
    CHECK(frame[0] == 0xA5);
    frame[4] = 0x81;
    CHECK(_card_write(0, entry.head, frame) == 1 && TestEvent(hw_ioe) == 1);
    CHECK(s_files[fake_find(0, "BASLUS-01013LOM00")].bytes[4] == 0x81);
    CHECK(_card_read(0, entry.head + 1, frame) == 1 && frame[0] == 0xA5);

    CHECK(_card_read(0, 0, frame) == 1 && frame[0] == 'M' && frame[1] == 'C');

    s_free[0] = 9;
    CHECK(_card_load(0) == 1);
    CHECK(firstfile("bu00:*", &entry) == &entry);
    {
        int saw_ours = 0;
        int filler = -1;

        do
        {
            saw_ours |= strcmp(entry.name, "BASLUS-01013LOM00") == 0;
            if (strcmp(entry.name, "BADCVMU-OTHER") == 0)
            {
                filler = (int)entry.size;
            }
        } while (nextfile(&entry) != NULL);
        CHECK(saw_ours && filler == 12 * 8192);
    }
    s_free[0] = 200;

    i = s_writes;
    fd = lom_card_open("bu00:BASLUS-01013LOM00", F_WRITE);
    CHECK(fd >= 0 && lom_card_write(fd, s_blob, 16) == 16);
    s_present[0] = 0;
    CHECK(lom_card_close(fd) == 0 && s_writes == i);
    CHECK(_card_info(0) == 1 && TestEvent(sw_tmo) == 1);
    s_present[0] = 1;

    CHECK(lom_card_rename("bu00:BASLUS-01013LOM00", "bu00:BASLUS-01013LOM01") == 0);
    CHECK(fake_count(0) == 1 && fake_find(0, "BASLUS-01013LOM01") >= 0);
    CHECK(lom_card_rename("bu00:BASLUS-01013LOM00", "bu00:BASLUS-01013LOM02") == -1);

    CHECK(erase("bu00:BASLUS-01013LOM01") == 1 && fake_count(0) == 0);
    CHECK(erase("bu00:BASLUS-01013LOM01") == 0);

    CHECK(save(0) == 0);
    s_files[fake_find(0, "BASLUS-01013LOM00")].corrupt = 1;
    CHECK(_card_load(0) == 1);
    i = s_reads;
    CHECK(lom_card_open("bu00:BASLUS-01013LOM00", F_READ) == -1 && s_reads == i + 1);
    CHECK(lom_card_open("bu00:BASLUS-01013LOM00", F_READ) == -1 && s_reads == i + 1);
    CHECK(_card_load(0) == 1);
    CHECK(firstfile("bu00:BASLUS-01013*", &entry) == NULL);
    CHECK(lom_card_open("bu00:BASLUS-01013LOM00", F_READ) == -1 && s_reads == i + 1);
    CHECK(save(0x5A) == 0);
    CHECK(firstfile("bu00:BASLUS-01013*", &entry) == &entry && strcmp(entry.name, "BASLUS-01013LOM00") == 0);
    fd = lom_card_open("bu00:BASLUS-01013LOM00", F_READ);
    CHECK(fd >= 0 && lom_card_read(fd, back, 4) == 4 && back[0] == 0x5A && lom_card_close(fd) == 0);
    s_files[fake_find(0, "BASLUS-01013LOM00")].corrupt = 1;
    CHECK(lom_card_open("bu00:BASLUS-01013LOM00", F_READ) == -1);
    CHECK(erase("bu00:BASLUS-01013LOM00") == 1 && fake_count(0) == 0);

    {
        static unsigned char rrl[0xC000];
        unsigned char head[0x400];
        long ioe = OpenEvent(SwCARD, EvSpIOE, EvMdNOINTR, 0);

        for (i = 0; i < (int)sizeof rrl; i++)
        {
            rrl[i] = (unsigned char)(i * 7 + (i >> 8));
        }
        fd = lom_card_open("bu00:BASLUSP01013", F_CREAT | F_BLOCKS(6));
        CHECK(fd >= 0 && lom_card_close(fd) == 0);
        fd = lom_card_open("bu00:BASLUSP01013", F_ASYNC | F_WRITE);
        CHECK(fd >= 0 && lom_card_write(fd, rrl, sizeof rrl) == (long)sizeof rrl && TestEvent(ioe) == 1);
        CHECK(lom_card_close(fd) == 0);
        i = fake_find(0, "BASLUSP01013");
        CHECK(i >= 0 && s_files[i].size == sizeof rrl && memcmp(s_files[i].bytes, rrl, sizeof rrl) == 0);
        CHECK(firstfile("bu00:BASLUSP01013*", &entry) == &entry && strcmp(entry.name, "BASLUSP01013") == 0 && entry.size == (long)sizeof rrl);
        fd = lom_card_open("bu00:BASLUSP01013", F_READ);
        CHECK(fd >= 0 && lom_card_read(fd, head, sizeof head) == (long)sizeof head && lom_card_close(fd) == 0);
        CHECK(memcmp(head, rrl, sizeof head) == 0);
        CHECK(erase("bu00:BASLUSP01013") == 1 && fake_find(0, "BASLUSP01013") < 0);
    }
    {
        long ioe = OpenEvent(SwCARD, EvSpIOE, EvMdNOINTR, 0);
        int writes;
        int rewrites;

        for (i = 0; i < (int)sizeof s_blob; i++)
        {
            s_blob[i] = (unsigned char)(i ^ 0x3C);
        }
        fd = lom_card_open("bu00:BASLUS-01013DUMMY", F_CREAT | F_BLOCKS(2));
        CHECK(fd >= 0 && lom_card_close(fd) == 0);
        fd = lom_card_open("bu00:BASLUS-01013DUMMY", F_ASYNC | F_WRITE);
        writes = s_writes;
        s_busy_polls = 3;
        CHECK(fd >= 0 && lom_card_write(fd, s_blob, sizeof s_blob) == (long)sizeof s_blob);
        CHECK(TestEvent(ioe) == 0 && TestEvent(ioe) == 0 && TestEvent(ioe) == 0);
        CHECK(s_writes == writes);
        CHECK(TestEvent(ioe) == 1 && s_writes == writes + 1);
        CHECK(lom_card_close(fd) == 0 && s_writes == writes + 1);
        rewrites = s_rewrites;
        CHECK(lom_card_rename("bu00:BASLUS-01013DUMMY", "bu00:BASLUS-01013LOM03") == 0);
        CHECK(s_writes == writes + 1 && s_rewrites == rewrites + 1);
        CHECK(fake_find(0, "BASLUS-01013DUMMY") < 0 && fake_find(0, "BASLUS-01013LOM03") >= 0);
        CHECK(firstfile("bu00:BASLUS-01013LOM03", &entry) == &entry);
        CHECK(_card_read(0, entry.head, frame) == 1 && TestEvent(hw_ioe) == 1);
        frame[6] = 0x42;
        CHECK(_card_write(0, entry.head, frame) == 1 && TestEvent(hw_ioe) == 1);
        CHECK(s_writes == writes + 1 && s_rewrites == rewrites + 2);
        CHECK(s_files[fake_find(0, "BASLUS-01013LOM03")].bytes[6] == 0x42);

        fd = lom_card_open("bu00:BASLUS-01013LOM03", F_ASYNC | F_WRITE);
        s_busy_polls = 5;
        CHECK(fd >= 0 && lom_card_write(fd, s_blob, 16) == 16);
        CHECK(lom_card_close(fd) == 0 && s_job.active == 0 && s_writes == writes + 2);
        CHECK(TestEvent(ioe) == 1);
        CHECK(erase("bu00:BASLUS-01013LOM03") == 1 && fake_find(0, "BASLUS-01013LOM03") < 0);
    }
    memset(time, 0x77, sizeof(time));
    CHECK(McxCardType(0) == MCX_COMMAND_ISSUED);
    CHECK(McxSync(MCX_SYNC_WAIT, &cmd, &result) == McxSyncFin && result == McxErrSuccess);
    CHECK(McxGetTime(0, time) == MCX_COMMAND_ISSUED);
    CHECK(McxSync(MCX_SYNC_WAIT, &cmd, &result) == McxSyncFin && result == McxErrInvalid && time[0] == 0x77);
    CHECK(func_80034648(0, 1, 1) != 0);
    CHECK(McxCardType(0x10) == MCX_COMMAND_ISSUED);
    CHECK(McxSync(MCX_SYNC_WAIT, &cmd, &result) == McxSyncFin && result == McxErrNoCard);
    CHECK(McxSync(MCX_SYNC_WAIT, &cmd, &result) == McxSyncNone);

    CHECK(func_8001680C("bu00:BASLUS-01013NONE", F_READ) == -1);
    CHECK(func_8001683C(3) == 0);
    {
        char path[32];

        CHECK(func_800170BC(path, "bu00:") == path && func_80016F9C(path, "BASLUS") == path);
        CHECK(strcmp(path, "bu00:BASLUS") == 0);
    }

    CHECK(lom_card_game_state(0) == VMU_GAME_UNSUPPORTED);
    lom_card_game_allow_replace(0);
    card_vmu_set_store(&s_game_fake);
    s_game_state = VMU_GAME_OTHER;
    CHECK(lom_card_game_state(0) == VMU_GAME_OTHER && lom_card_game_state(1) == VMU_GAME_NO_ROOM);
    lom_card_game_allow_replace(1);
    CHECK(s_game_allowed[0] == 0 && s_game_allowed[1] == 1);
    CHECK(lom_card_game_last(0) == VMU_GAME_OTHER && lom_card_game_last(1) == VMU_GAME_NO_ROOM);
    s_game_state = VMU_GAME_MISSING;
    CHECK(lom_card_game_last(0) == VMU_GAME_OTHER && lom_card_game_state(0) == VMU_GAME_MISSING && lom_card_game_last(0) == VMU_GAME_MISSING);
    card_vmu_set_store(&s_fake);
    puts("OK");
    return 0;
}
