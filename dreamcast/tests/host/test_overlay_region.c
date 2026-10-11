/*----------------------
 | test_overlay_region.c
 | Description: Host checks for the Dreamcast overlay region lookup, the fit check against a decoded header and the OVLnnC.BIN and OVLnn.BIN file names.
 | Author: suinevere
 | Dependencies: overlay_region.h, overlay_header.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "overlay_region.h"

static int s_failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); s_failures++; } } while (0)

static const OverlayRegion s_table[] = {
    { 15, "CHECKPS", 0x8C800000u, 0x00040000u, 0x8C840000u, 0x00040000u },
    { 4, "TITLE", 0x8C880000u, 0x00040000u, 0x8C8C0000u, 0x000C0000u },
};

static OverlayHeader fitting(void)
{
    OverlayHeader h;

    h.magic = OVERLAY_HEADER_MAGIC;
    h.version = 1;
    h.code_base = 0x8C800000u;
    h.code_size = 0x1000u;
    h.data_size = 0xAC88u;
    h.bss_start = 0x8C84AC88u;
    h.bss_end = 0x8C84B000u;
    return h;
}

static void test_find(void)
{
    CHECK(overlay_region_find(s_table, 2, 15) == &s_table[0]);
    CHECK(overlay_region_find(s_table, 2, 4) == &s_table[1]);
    CHECK(overlay_region_find(s_table, 2, 2) == 0);
}

static void test_fitting_overlay_passes(void)
{
    OverlayHeader h = fitting();

    CHECK(overlay_region_check(&s_table[0], &h, 0xAC88u) == 0);
    h.code_size = 0x40000u;
    h.bss_end = 0x8C880000u;
    CHECK(overlay_region_check(&s_table[0], &h, 0xAC88u) == 0);
}

static void test_code_that_does_not_fit_names_the_overlay(void)
{
    OverlayHeader h = fitting();

    h.code_size = 0x40004u;
    CHECK(strcmp(overlay_region_check(&s_table[0], &h, 0xAC88u), "CHECKPS") == 0);
    h = fitting();
    h.code_base = 0x8C880000u;
    CHECK(strcmp(overlay_region_check(&s_table[0], &h, 0xAC88u), "CHECKPS") == 0);
    h.code_size = 0;
    h.code_base = 0;
    CHECK(overlay_region_check(&s_table[0], &h, 0xAC88u) != 0);
}

static void test_data_that_does_not_fit_names_the_overlay(void)
{
    OverlayHeader h = fitting();

    CHECK(strcmp(overlay_region_check(&s_table[0], &h, 0xAC8Cu), "CHECKPS") == 0);
    h.data_size = 0x40004u;
    h.bss_start = 0x8C880004u;
    h.bss_end = 0x8C880004u;
    CHECK(strcmp(overlay_region_check(&s_table[0], &h, 0x40004u), "CHECKPS") == 0);
    h = fitting();
    h.bss_end = 0x8C880004u;
    CHECK(strcmp(overlay_region_check(&s_table[0], &h, 0xAC88u), "CHECKPS") == 0);
    h = fitting();
    h.bss_start = 0x8C84AC84u;
    CHECK(strcmp(overlay_region_check(&s_table[0], &h, 0xAC88u), "CHECKPS") == 0);
    h = fitting();
    h.bss_end = h.bss_start - 4;
    CHECK(strcmp(overlay_region_check(&s_table[0], &h, 0xAC88u), "CHECKPS") == 0);
}

static void test_title_data_region_takes_its_blob(void)
{
    OverlayHeader h = fitting();

    h.code_base = 0x8C880000u;
    h.data_size = 0xB03E4u;
    h.bss_start = 0x8C8C0000u + 0xB03E4u;
    h.bss_end = 0x8C980000u;
    CHECK(overlay_region_check(&s_table[1], &h, 0xB03E4u) == 0);
}

static void test_file_names(void)
{
    char name[16];

    overlay_file_name(15, 1, name);
    CHECK(strcmp(name, "OVL15C.BIN") == 0);
    overlay_file_name(4, 1, name);
    CHECK(strcmp(name, "OVL04C.BIN") == 0);
    overlay_file_name(15, 0, name);
    CHECK(strcmp(name, "OVL15.BIN") == 0);
}

int main(void)
{
    test_find();
    test_fitting_overlay_passes();
    test_code_that_does_not_fit_names_the_overlay();
    test_data_that_does_not_fit_names_the_overlay();
    test_title_data_region_takes_its_blob();
    test_file_names();
    if (s_failures == 0)
    {
        printf("OK\n");
    }
    return s_failures != 0;
}
