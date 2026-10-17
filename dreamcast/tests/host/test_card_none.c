/*----------------------
 | test_card_none.c
 | Description: Host test for the no-card backend: card requests are issued and time out, only timeout events fire, directory searches find nothing and libmcx reports no card.
 | Author: suinevere
 | Dependencies: card_none.c, libapi.h, libmcx.h
 ----------------------*/
#include <libapi.h>
#include <libmcx.h>

int printf(const char *format, ...);
int func_80033E7C(long chan);
int func_80034648(long chan, long block, long count);
int puts(const char *text);
long func_8001680C(char *path, long mode);
long func_8001681C(long fd, void *buf, long n);
long func_8001682C(long fd, void *buf, long n);
long func_8001683C(long fd);
long func_8001685C(char *from, char *to);
char *func_80016F9C(char *dst, const char *src);
char *func_800170BC(char *dst, const char *src);

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); return 1; } } while (0)

int main(void)
{
    long ioe = OpenEvent(SwCARD, EvSpIOE, EvMdNOINTR, 0);
    long err = OpenEvent(SwCARD, EvSpERROR, EvMdNOINTR, 0);
    long tmo = OpenEvent(SwCARD, EvSpTIMOUT, EvMdNOINTR, 0);
    long new_card = OpenEvent(SwCARD, EvSpNEW, EvMdNOINTR, 0);
    long hw_tmo = OpenEvent(HwCARD, EvSpTIMOUT, EvMdNOINTR, 0);
    long cmd = 0;
    long result = -1;
    unsigned char clock[8];
    struct DIRENTRY entry;

    CHECK(ioe != 0 && ioe != err && err != tmo && tmo != new_card && new_card != hw_tmo);
    CHECK(EnableEvent(tmo) == 1);
    CHECK(_card_info(0) == 1);
    CHECK(_card_load(0) == 1);
    CHECK(TestEvent(tmo) == 1);
    CHECK(TestEvent(hw_tmo) == 1);
    CHECK(TestEvent(ioe) == 0 && TestEvent(err) == 0 && TestEvent(new_card) == 0);
    CHECK(firstfile("bu00:*", &entry) == 0);
    CHECK(nextfile(&entry) == 0);
    CHECK(McxCardType(0) == MCX_COMMAND_ISSUED);
    CHECK(McxSync(MCX_SYNC_WAIT, &cmd, &result) == McxSyncFin && result == McxErrNoCard);
    result = -1;
    CHECK(McxGetTime(0, clock) == MCX_COMMAND_ISSUED);
    CHECK(McxSync(MCX_SYNC_WAIT, &cmd, &result) == McxSyncFin && result == McxErrNoCard);
    CHECK(CloseEvent(ioe) == 1);
    CHECK(_card_wait(0) == 1);
    CHECK(erase("bu00:BASLUS-01013") == 0);
    CHECK(func_80033E7C(0) == 0);
    CHECK(func_80034648(0, 1, 1) == 0);
    {
        char path[32];
        char data[4];

        CHECK(func_8001680C("bu00:BASLUS", 0x8001) == -1);
        CHECK(func_8001681C(3, data, 4) == -1 && func_8001682C(3, data, 4) == -1);
        CHECK(func_8001683C(3) == 0);
        CHECK(func_8001685C("bu00:A", "bu00:B") == 0);
        CHECK(func_800170BC(path, "bu00:") == path);
        CHECK(func_80016F9C(path, "BASLUS") == path);
        CHECK(path[0] == 'b' && path[5] == 'B' && path[10] == 'S' && path[11] == 0);
    }
    puts("OK");
    return 0;
}
