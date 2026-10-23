/*----------------------
 | test_iso_dir.c
 | Description: Host tests for iso_dir.c: the root entry comes out of a primary volume descriptor, and directory lookups match names the way the disc filesystem needs.
 | Author: suinevere
 | Dependencies: iso_dir.c, iso_dir.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "iso_dir.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static void put32(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static unsigned add_record(unsigned char *sector, unsigned at, const char *id, unsigned id_length, unsigned extent, unsigned size, int dir)
{
    unsigned length = 33u + id_length + ((id_length & 1u) == 0 ? 1u : 0u);

    sector[at] = (unsigned char)length;
    put32(sector + at + 2, extent);
    put32(sector + at + 10, size);
    sector[at + 25] = dir ? 2 : 0;
    sector[at + 32] = (unsigned char)id_length;
    memcpy(sector + at + 33, id, id_length);
    return at + length;
}

static void test_pvd_root(void)
{
    unsigned char pvd[ISO_SECTOR_BYTES];
    IsoEntry root;

    memset(pvd, 0, sizeof(pvd));
    CHECK(iso_pvd_root(pvd, &root) == -1);
    pvd[0] = 1;
    memcpy(pvd + 1, "CD001", 5);
    add_record(pvd, 156, "\0", 1, 45018, 2048, 1);
    CHECK(iso_pvd_root(pvd, &root) == 0);
    CHECK(root.extent == 45018 && root.size == 2048 && root.is_dir);
}

static void test_dir_find(void)
{
    unsigned char sector[ISO_SECTOR_BYTES];
    IsoEntry entry;
    unsigned at = 0;

    memset(sector, 0, sizeof(sector));
    at = add_record(sector, at, "\0", 1, 45018, 2048, 1);
    at = add_record(sector, at, "\1", 1, 45018, 2048, 1);
    at = add_record(sector, at, "OVL", 3, 45020, 4096, 1);
    at = add_record(sector, at, "RESINDEX.BIN;1", 14, 548825, 40452, 0);
    add_record(sector, at, "README.;1", 9, 600, 12, 0);
    CHECK(iso_dir_find(sector, "RESINDEX.BIN", 12, &entry) == 1);
    CHECK(entry.extent == 548825 && entry.size == 40452 && !entry.is_dir);
    CHECK(iso_dir_find(sector, "resindex.bin", 12, &entry) == 1);
    CHECK(iso_dir_find(sector, "OVL", 3, &entry) == 1 && entry.is_dir && entry.extent == 45020);
    CHECK(iso_dir_find(sector, "README", 6, &entry) == 1 && entry.extent == 600);
    CHECK(iso_dir_find(sector, "RESINDEX", 8, &entry) == 0);
    CHECK(iso_dir_find(sector, "MISSING.BIN", 11, &entry) == 0);
}

static void test_bad_record_stops(void)
{
    unsigned char sector[ISO_SECTOR_BYTES];
    IsoEntry entry;

    memset(sector, 0, sizeof(sector));
    sector[0] = 20;
    sector[32] = 30;
    CHECK(iso_dir_find(sector, "A", 1, &entry) == 0);
}

int main(void)
{
    test_pvd_root();
    test_dir_find();
    test_bad_record_stops();
    if (failures == 0)
        printf("OK\n");
    return failures != 0;
}
