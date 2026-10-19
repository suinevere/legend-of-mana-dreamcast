/*----------------------
 | test_cd_stream.c
 | Description: Host test for cd_stream.c: sectors arrive at 150 per second of vblanks, bursts are capped, the callback stops the stream, CdGetSector reads header then payload, unread sectors wait, and delivery never re-enters.
 | Author: suinevere
 | Dependencies: cd_stream.h
 | Globals: s_fail, s_sectors, s_calls, s_stop_after, s_read_payload, s_seen_header, s_payload_ok, s_ready_limit, s_order_ok, s_reenter, s_depth, s_max_depth
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "cd_stream.h"

static int s_fail;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); s_fail = 1; } } while (0)

static uint8_t s_sectors[40][CD_STREAM_SECTOR_BYTES];
static int s_calls, s_stop_after, s_read_payload, s_payload_ok, s_ready_limit, s_order_ok, s_reenter, s_depth, s_max_depth;
static uint32_t s_seen_header[8];

/*----------------------
 | fake_fetch
 | Description: Returns a test sector once it is within s_ready_limit, else NULL as an unread sector.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_sectors, s_ready_limit
 | Params: ctx -- ignored, sector -- sector number
 | Returns: sector bytes or NULL
 ----------------------*/
static const uint8_t *fake_fetch(void *ctx, unsigned sector)
{
    (void)ctx;
    return (int)sector < s_ready_limit ? s_sectors[sector] : NULL;
}

/*----------------------
 | on_sector
 | Description: The test's sector callback: reads the header (and payload when asked), checks sector order, and can stop or re-enter.
 | Author: suinevere
 | Dependencies: CdGetSector, cd_stream_service
 | Globals: all of the above
 | Params: bytes, remaining -- ignored
 | Returns: NULL to stop after s_stop_after calls, else (u8*)1
 ----------------------*/
static unsigned char *on_sector(int bytes, unsigned remaining)
{
    uint32_t payload[(CD_STREAM_SECTOR_BYTES - 32) / 4];

    (void)bytes;
    (void)remaining;
    s_depth++;
    s_max_depth = s_depth > s_max_depth ? s_depth : s_max_depth;
    CHECK(CdGetSector(s_seen_header, 8) == 1);
    if ((s_seen_header[0] & 0xFF) != (uint32_t)s_calls)
    {
        s_order_ok = 0;
    }
    if (s_read_payload)
    {
        CHECK(CdGetSector(payload, (CD_STREAM_SECTOR_BYTES - 32) / 4) == 1);
        s_payload_ok = ((const uint8_t *)payload)[0] == 0xA5;
        CHECK(CdGetSector(payload, 1) == 0);
    }
    s_calls++;
    if (s_reenter)
    {
        cd_stream_service(1000);
    }
    s_depth--;
    return (s_stop_after != 0 && s_calls == s_stop_after) ? (unsigned char *)0 : (unsigned char *)1;
}

/*----------------------
 | start
 | Description: Resets the test state and starts a stream of the given length at vblank 0.
 | Author: suinevere
 | Dependencies: cd_stream_start
 | Globals: all of the above
 | Params: total -- sectors in the stream
 | Returns: N/A
 ----------------------*/
static void start(unsigned total)
{
    int i;

    for (i = 0; i < 40; i++)
    {
        memset(s_sectors[i], 0xA5, sizeof(s_sectors[i]));
        memset(s_sectors[i], 0, 32);
        s_sectors[i][0] = (uint8_t)i;
        s_sectors[i][4] = 1;
    }
    s_calls = 0;
    s_stop_after = 0;
    s_read_payload = 0;
    s_payload_ok = 0;
    s_ready_limit = 40;
    s_order_ok = 1;
    s_reenter = 0;
    s_depth = 0;
    s_max_depth = 0;
    cd_stream_start(total, on_sector, 0, fake_fetch, NULL);
}

/*----------------------
 | test_paced_delivery
 | Description: Nothing is due at the start, 5 sectors after 2 vblanks, and a repeat call at the same vblank delivers nothing more.
 | Author: suinevere
 | Dependencies: start, cd_stream_service
 | Globals: s_calls
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_paced_delivery(void)
{
    start(40);
    cd_stream_service(0);
    CHECK(s_calls == 0);
    cd_stream_service(2);
    CHECK(s_calls == 5);
    cd_stream_service(2);
    CHECK(s_calls == 5);
}

/*----------------------
 | test_burst_is_capped
 | Description: A long stall delivers at most CD_STREAM_MAX_BURST sectors in one call.
 | Author: suinevere
 | Dependencies: start, cd_stream_service
 | Globals: s_calls
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_burst_is_capped(void)
{
    start(40);
    cd_stream_service(60);
    CHECK(s_calls == CD_STREAM_MAX_BURST);
}

/*----------------------
 | test_callback_null_stops
 | Description: A callback returning NULL ends the stream.
 | Author: suinevere
 | Dependencies: start, cd_stream_service, cd_stream_active
 | Globals: s_calls, s_stop_after
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_callback_null_stops(void)
{
    start(40);
    s_stop_after = 3;
    cd_stream_service(60);
    CHECK(s_calls == 3);
    CHECK(!cd_stream_active());
}

/*----------------------
 | test_runs_out_and_stops
 | Description: Delivering the last sector ends the stream.
 | Author: suinevere
 | Dependencies: start, cd_stream_service, cd_stream_active, cd_stream_delivered
 | Globals: s_calls
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_runs_out_and_stops(void)
{
    start(4);
    cd_stream_service(60);
    CHECK(s_calls == 4);
    CHECK(!cd_stream_active());
    CHECK(cd_stream_delivered() == 4);
}

/*----------------------
 | test_get_sector_reads_header_then_payload
 | Description: CdGetSector returns the 8 header words, then the payload, then nothing.
 | Author: suinevere
 | Dependencies: start, cd_stream_service
 | Globals: s_seen_header, s_payload_ok, s_read_payload
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_get_sector_reads_header_then_payload(void)
{
    start(40);
    s_read_payload = 1;
    cd_stream_service(1);
    CHECK(s_calls == 2);
    CHECK((s_seen_header[0] & 0xFF) == 1 && s_seen_header[1] == 1);
    CHECK(s_payload_ok);
}

/*----------------------
 | test_no_current_sector_returns_zero
 | Description: Outside a callback there is no sector to read.
 | Author: suinevere
 | Dependencies: cd_stream_stop, CdGetSector
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_no_current_sector_returns_zero(void)
{
    uint32_t w[8];

    cd_stream_stop();
    CHECK(CdGetSector(w, 8) == 0);
}

/*----------------------
 | test_fetch_not_ready_waits_without_skipping
 | Description: An unread sector holds delivery back, and later calls resume from it in order.
 | Author: suinevere
 | Dependencies: start, cd_stream_service
 | Globals: s_calls, s_ready_limit, s_order_ok
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_fetch_not_ready_waits_without_skipping(void)
{
    start(40);
    s_ready_limit = 2;
    cd_stream_service(10);
    CHECK(s_calls == 2);
    s_ready_limit = 40;
    cd_stream_service(10);
    CHECK(s_calls == 25);
    CHECK(s_order_ok);
}

/*----------------------
 | test_no_reentry
 | Description: A service call made from inside the callback returns at once.
 | Author: suinevere
 | Dependencies: start, cd_stream_service
 | Globals: s_reenter, s_max_depth
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_no_reentry(void)
{
    start(40);
    s_reenter = 1;
    cd_stream_service(4);
    CHECK(s_max_depth == 1);
    CHECK(s_calls == 10);
}

/*----------------------
 | test_refresh_changes_pacing
 | Description: At 50 Hz, 2 vblanks deliver 6 sectors (150/50 per vblank); switching back to 60 Hz delivers the usual 5, and the refresh is reset to 60 for later tests.
 | Author: suinevere
 | Dependencies: start, cd_stream_service, cd_stream_set_refresh
 | Globals: s_calls
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_refresh_changes_pacing(void)
{
    cd_stream_set_refresh(50);
    start(40);
    cd_stream_service(2);
    CHECK(s_calls == 6);
    cd_stream_set_refresh(60);
    start(40);
    cd_stream_service(2);
    CHECK(s_calls == 5);
}

/*----------------------
 | main
 | Description: Runs every cd_stream test and prints ok when all pass.
 | Author: suinevere
 | Dependencies: the tests above
 | Globals: s_fail
 | Params: N/A
 | Returns: 0 on success, 1 on any failure
 ----------------------*/
int main(void)
{
    test_paced_delivery();
    test_burst_is_capped();
    test_callback_null_stops();
    test_runs_out_and_stops();
    test_get_sector_reads_header_then_payload();
    test_no_current_sector_returns_zero();
    test_fetch_not_ready_waits_without_skipping();
    test_no_reentry();
    test_refresh_changes_pacing();
    if (!s_fail)
    {
        printf("ok\n");
    }
    return s_fail;
}
