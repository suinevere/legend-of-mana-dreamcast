/*----------------------
 | test_cdrom_dc.c
 | Description: Host tests for cdrom_dc.c's stdio resource reads, queued-read callback ordering, the fanned-out RES/<XX> subfolder path, raw WMAP layout cells, stream decompression of raw PS1 resources, and resource-naming halts, run against a real temp-directory disc fixture written by test_cdrom_dc_host.py.
 | Author: suinevere
 | Dependencies: cdrom_dc.c, resource_index.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

/* Pre-empts common.h's splat labels.inc/macro.inc inline asm, which this host test never needs. */
#define INCLUDE_ASM_H
#include "main/cdrom.h"
#include "resource_index.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static jmp_buf s_halt_jump;
static char s_halt_message[256];
static int s_halted;

void dc_halt(const char *message)
{
    strncpy(s_halt_message, message, sizeof(s_halt_message) - 1);
    s_halt_message[sizeof(s_halt_message) - 1] = 0;
    s_halted = 1;
    longjmp(s_halt_jump, 1);
}

void dc_log(const char *format, ...)
{
    (void)format;
}

void load_overlay(int resource_index)
{
    (void)resource_index;
}

static int s_decompress_calls;

void cdrom_decompress_buffer(u8 *source, u8 *destination)
{
    s_decompress_calls++;
    memcpy(destination, source + 1, 4);
}

static unsigned char s_cb_dst[8];
static int s_ask_calls;
static int s_complete_calls;
static s32 s_complete_size;

static u8 *test_callback(s32 bytes_transferred, u32 bytes_remaining)
{
    if (bytes_remaining != 0)
    {
        s_ask_calls++;
        return s_cb_dst;
    }
    s_complete_calls++;
    s_complete_size = bytes_transferred;
    return 0;
}

static void test_cdrom_init_reads_the_fixture_index(void)
{
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        cdrom_init();
    }
    CHECK(!s_halted);
}

static void test_stream_copies_a_converted_stream_exactly(void)
{
    unsigned char dst[8];

    memset(dst, 0xAA, sizeof(dst));
    s_decompress_calls = 0;
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        CHECK(cdrom_stream(10, dst) == 0);
    }
    CHECK(!s_halted);
    CHECK(s_decompress_calls == 0);
    CHECK(memcmp(dst, "ABCD", 4) == 0);
}

static void test_queue_read_with_callback_defers_to_process_state(void)
{
    s_ask_calls = 0;
    s_complete_calls = 0;
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        CHECK(cdrom_queue_read_with_callback(11, test_callback) == 0);
    }
    CHECK(!s_halted);
    CHECK(s_ask_calls == 0 && s_complete_calls == 0);
    CHECK(cdrom_process_state() == 0);
    CHECK(s_ask_calls == 1 && s_complete_calls == 1);
    CHECK(s_complete_size == 6);
    CHECK(memcmp(s_cb_dst, "FEDCBA", 6) == 0);
}

static void test_stream_returns_zero_for_skipped_resource(void)
{
    unsigned char dst[4];

    memset(dst, 0xAA, sizeof(dst));
    s_halt_message[0] = 0;
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        CHECK(cdrom_stream(12, dst) == 0);
    }
    CHECK(!s_halted);
    CHECK(dst[0] == 0xAA);
}

static void test_queue_read_of_skipped_resource_halts_naming_not_on_disc(void)
{
    unsigned char dst[4];

    s_halt_message[0] = 0;
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        cdrom_queue_read(12, dst);
        CHECK(0);
    }
    CHECK(strstr(s_halt_message, "resource 12 not on disc") != 0);
}

static void test_queue_read_of_entry_9_skip_halts_naming_not_on_disc(void)
{
    unsigned char dst[4];

    s_halt_message[0] = 0;
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        cdrom_queue_read(9, dst);
        CHECK(0);
    }
    CHECK(strstr(s_halt_message, "resource 9 not on disc") != 0);
}

static void test_queue_seek_returns_size_without_halting(void)
{
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        CHECK(cdrom_queue_seek(0x123) == 4);
        CHECK(cdrom_queue_seek(12) == 0);
    }
    CHECK(!s_halted);
}

static void test_queue_read_reads_a_fanned_out_res_subfolder(void)
{
    unsigned char dst[4];

    memset(dst, 0xAA, sizeof(dst));
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        CHECK(cdrom_queue_read(0x123, dst) == 4);
    }
    CHECK(!s_halted);
    CHECK(memcmp(dst, "HIJK", 4) == 0);
}

static void test_truncated_file_halts_naming_both_sizes(void)
{
    unsigned char dst[8];

    s_halt_message[0] = 0;
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        cdrom_queue_read(13, dst);
        CHECK(0);
    }
    CHECK(strstr(s_halt_message, "13") != 0);
    CHECK(strstr(s_halt_message, "8") != 0);
    CHECK(strstr(s_halt_message, "4") != 0);
}

static void test_queue_read_reads_a_wmap_layout_cell_raw(void)
{
    unsigned char dst[4];

    memset(dst, 0xAA, sizeof(dst));
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        CHECK(cdrom_queue_read(0x12EB, dst) == 4);
    }
    CHECK(!s_halted);
    CHECK(memcmp(dst, "LAYO", 4) == 0);
}

static void test_stream_decompresses_a_raw_ps1_resource(void)
{
    unsigned char dst[4];

    memset(dst, 0xAA, sizeof(dst));
    s_decompress_calls = 0;
    s_halted = 0;
    if (setjmp(s_halt_jump) == 0)
    {
        CHECK(cdrom_stream(0x14DF, dst) == 0);
    }
    CHECK(!s_halted);
    CHECK(s_decompress_calls == 1);
    CHECK(memcmp(dst, "IMAG", 4) == 0);
}

int main(void)
{
    test_cdrom_init_reads_the_fixture_index();
    test_stream_copies_a_converted_stream_exactly();
    test_queue_read_with_callback_defers_to_process_state();
    test_stream_returns_zero_for_skipped_resource();
    test_queue_read_of_skipped_resource_halts_naming_not_on_disc();
    test_queue_read_of_entry_9_skip_halts_naming_not_on_disc();
    test_queue_seek_returns_size_without_halting();
    test_queue_read_reads_a_fanned_out_res_subfolder();
    test_truncated_file_halts_naming_both_sizes();
    test_queue_read_reads_a_wmap_layout_cell_raw();
    test_stream_decompresses_a_raw_ps1_resource();
    if (failures == 0)
    {
        printf("OK\n");
    }
    return failures != 0;
}
