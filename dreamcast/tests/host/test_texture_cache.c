/*----------------------
 | test_texture_cache.c
 | Description: Host tests for VRAM generations, LRU palette banks with pinning, colour conversion, twiddling, wrapped page reads and texture cache reuse, invalidation, expansion, deferred flush and mid-frame change paths.
 | Author: suinevere
 | Dependencies: vram_mirror.h, palette_bank.h, texture_cache.h, gpu_decode.h, vram_mirror.c, palette_bank.c, texture_cache.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "vram_mirror.h"
#include "gpu_decode.h"
#include "palette_bank.h"
#include "texture_cache.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

#define ARENA_BYTES (4u * 1024u * 1024u)
#define ARENA_HEADER 32u

static uint8_t s_arena[ARENA_BYTES];
static unsigned s_arena_used;
static unsigned s_arena_limit = ARENA_BYTES;
static unsigned s_free_all_calls;
static unsigned s_uploads;
static unsigned s_upload_bytes;
static uint32_t s_palette[1024];
static unsigned s_palette_writes;
static unsigned s_palette_max_index;

static void *fake_alloc(unsigned bytes)
{
    void *p;

    if (s_arena_used + ARENA_HEADER + bytes > s_arena_limit)
    {
        return NULL;
    }
    s_arena_used += ARENA_HEADER;
    p = &s_arena[s_arena_used];
    s_arena_used += bytes;
    return p;
}

static void fake_free_all(void)
{
    s_arena_used = 0;
    s_free_all_calls++;
}

static void fake_upload(void *dst, const void *src, unsigned bytes)
{
    memcpy(dst, src, bytes);
    s_uploads++;
    s_upload_bytes = bytes;
}

static void fake_set_entry(unsigned index, uint32_t argb8888)
{
    if (index < 1024)
    {
        s_palette[index] = argb8888;
    }
    s_palette_writes++;
    if (index > s_palette_max_index)
    {
        s_palette_max_index = index;
    }
}

static const PaletteOps s_palette_ops = { fake_set_entry };
static const TextureOps s_texture_ops = { fake_alloc, fake_free_all, fake_upload };

static void reset_all(void)
{
    s_arena_used = 0;
    s_arena_limit = ARENA_BYTES;
    s_free_all_calls = 0;
    s_uploads = 0;
    s_upload_bytes = 0;
    s_palette_writes = 0;
    s_palette_max_index = 0;
    memset(s_palette, 0, sizeof(s_palette));
    palette_bank_init(&s_palette_ops);
    texture_cache_init(&s_texture_ops);
}

static void load_word(int x, int y, unsigned short value)
{
    RECT rect = { (short)x, (short)y, 1, 1 };
    u_long v = value;

    vram_mirror_load(&rect, &v);
}

static void test_palette_convert(void)
{
    CHECK(palette_convert(0x0000, PAL_OPAQUE) == 0);
    CHECK(palette_convert(0x801F, PAL_HALF) == 0x80FF0000u);
    CHECK(palette_convert(0x001F, PAL_STP_ONLY) == 0);
    CHECK(palette_convert(0x7FFF, PAL_OPAQUE) == 0xFFFFFFFFu);
    CHECK(palette_convert(0x001F, PAL_HALF) == 0xFFFF0000u);
    CHECK(palette_convert(0x801F, PAL_STP_ONLY) == 0xFFFF0000u);
    CHECK(palette_convert(0x801F, PAL_NONSTP_ONLY) == 0);
    CHECK(palette_convert(0x03E0, PAL_NONSTP_ONLY) == 0xFF00FF00u);
    CHECK(palette_convert(0x7C00, PAL_OPAQUE) == 0xFF0000FFu);
    CHECK(palette_convert(0x8010, PAL_COVER) == 0xFFFFFFFFu);
    CHECK(palette_convert(0x7C00, PAL_COVER) == 0);
    CHECK(palette_convert(0x0000, PAL_COVER) == 0);
}

static void test_twiddle_index(void)
{
    CHECK(texture_twiddle_index(0, 0) == 0);
    CHECK(texture_twiddle_index(1, 0) == 2);
    CHECK(texture_twiddle_index(0, 1) == 1);
    CHECK(texture_twiddle_index(3, 3) == 15);
    CHECK(texture_twiddle_index(255, 255) == 65535);
}

static void test_generation_blocks(void)
{
    uint32_t page_gen;
    uint32_t wrapped_gen;

    page_gen = vram_mirror_generation(256, 0, 64, 256);
    load_word(900, 300, 1);
    CHECK(vram_mirror_generation(256, 0, 64, 256) == page_gen);
    load_word(300, 200, 1);
    CHECK(vram_mirror_generation(256, 0, 64, 256) > page_gen);
    wrapped_gen = vram_mirror_generation(1000, 500, 64, 32);
    load_word(5, 3, 1);
    CHECK(vram_mirror_generation(1000, 500, 64, 32) > wrapped_gen);
}

static void test_palette_same_clut_reused_then_refilled(void)
{
    RECT clut = { 0, 480, 16, 1 };
    unsigned short words[16];
    int slot;
    int i;

    reset_all();
    for (i = 0; i < 16; i++)
    {
        words[i] = (unsigned short)(0x0400 + i);
    }
    vram_mirror_load(&clut, (u_long *)words);
    slot = palette_bank_acquire(0, 480, DC_DEPTH_4, PAL_OPAQUE, 1);
    CHECK(slot >= 0);
    CHECK(s_palette_writes == 16);
    CHECK(s_palette[slot * 16 + 1] == palette_convert(0x0401, PAL_OPAQUE));
    CHECK(palette_bank_acquire(0, 480, DC_DEPTH_4, PAL_OPAQUE, 1) == slot);
    CHECK(s_palette_writes == 16);
    load_word(512, 0, 0x1234);
    CHECK(palette_bank_acquire(0, 480, DC_DEPTH_4, PAL_OPAQUE, 1) == slot);
    CHECK(s_palette_writes == 16);
    for (i = 0; i < 16; i++)
    {
        words[i] = (unsigned short)(0x7C00 | i);
    }
    vram_mirror_load(&clut, (u_long *)words);
    CHECK(palette_bank_acquire(0, 480, DC_DEPTH_4, PAL_OPAQUE, 2) == slot);
    CHECK(s_palette_writes == 32);
    CHECK(s_palette[slot * 16 + 1] == palette_convert(0x7C01, PAL_OPAQUE));
    CHECK(palette_bank_acquire(0, 480, DC_DEPTH_4, PAL_HALF, 2) != slot);
}

static void test_palette_pinning_and_fallback(void)
{
    DcSurfaceTexture out;
    int seen[64];
    int first_slot;
    int i;

    reset_all();
    memset(seen, 0, sizeof(seen));
    first_slot = -1;
    for (i = 0; i < 64; i++)
    {
        int slot = palette_bank_acquire(i * 16, 440, DC_DEPTH_4, PAL_OPAQUE, 1);

        CHECK(slot >= 0 && slot < 64);
        if (slot >= 0 && slot < 64)
        {
            seen[slot]++;
        }
        if (i == 0)
        {
            first_slot = slot;
        }
    }
    for (i = 0; i < 64; i++)
    {
        CHECK(seen[i] == 1);
    }
    CHECK(palette_bank_in_use() == 64);
    CHECK(palette_bank_acquire(0, 441, DC_DEPTH_4, PAL_OPAQUE, 1) == -1);
    memset(&out, 0, sizeof(out));
    CHECK(texture_cache_lookup(0, 0, 0, 441, DC_DEPTH_4, PAL_OPAQUE, 1, &out) == 0);
    CHECK(out.format == TEX_FMT_ARGB1555);
    CHECK(out.palette_slot == -1);
    CHECK(out.texture != NULL);
    CHECK(texture_cache_expansions() == 1);
    CHECK(s_upload_bytes == 256u * 256u * 2u);
    CHECK(palette_bank_acquire(0, 441, DC_DEPTH_4, PAL_OPAQUE, 2) == first_slot);
    CHECK(palette_bank_in_use() == 64);
}

static void test_palette_8bpp_takes_aligned_run(void)
{
    int small;
    int run;

    reset_all();
    small = palette_bank_acquire(0, 460, DC_DEPTH_4, PAL_OPAQUE, 1);
    CHECK(small >= 0);
    s_palette_writes = 0;
    s_palette_max_index = 0;
    run = palette_bank_acquire(0, 461, DC_DEPTH_8, PAL_OPAQUE, 1);
    CHECK(run >= 0);
    CHECK(run % 16 == 0);
    CHECK(small < run || small >= run + 16);
    CHECK(s_palette_writes == 256);
    CHECK(s_palette_max_index == (unsigned)(run * 16 + 255));
    CHECK(palette_bank_in_use() == 17);
}

static void test_page_reuse_and_invalidation(void)
{
    DcSurfaceTexture out;
    DcSurfaceTexture again;
    const uint8_t *tex;

    reset_all();
    load_word(64, 0, 0x4321);
    CHECK(texture_cache_lookup(64, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 1, &out) == 0);
    CHECK(out.format == TEX_FMT_PAL4);
    CHECK(out.palette_slot >= 0);
    CHECK(s_uploads == 1);
    CHECK(s_upload_bytes == 32768u);
    tex = (const uint8_t *)out.texture;
    CHECK((tex[0] & 0x0F) == 1);
    CHECK((tex[1] & 0x0F) == 2);
    CHECK((tex[4] & 0x0F) == 3);
    CHECK((tex[5] & 0x0F) == 4);
    CHECK(texture_cache_lookup(64, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 1, &again) == 0);
    CHECK(s_uploads == 1);
    CHECK(again.texture == out.texture);
    load_word(70, 10, 0x1111);
    CHECK(texture_cache_lookup(64, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 2, &again) == 0);
    CHECK(s_uploads == 2);
    CHECK(again.texture == out.texture);
}

static void test_direct_page_converts(void)
{
    DcSurfaceTexture out;
    const uint16_t *tex;

    reset_all();
    load_word(512, 256, 0x001F);
    load_word(513, 256, 0x0000);
    load_word(512, 257, 0xFC00);
    CHECK(texture_cache_lookup(512, 256, 0, 0, DC_DEPTH_15, PAL_OPAQUE, 1, &out) == 0);
    CHECK(out.format == TEX_FMT_ARGB1555);
    CHECK(out.palette_slot == -1);
    CHECK(s_upload_bytes == 131072u);
    tex = (const uint16_t *)out.texture;
    CHECK(tex[texture_twiddle_index(0, 0)] == 0xFC00);
    CHECK(tex[texture_twiddle_index(1, 0)] == 0x0000);
    CHECK(tex[texture_twiddle_index(0, 1)] == 0x801F);
    CHECK(texture_cache_expansions() == 0);
}

static void test_read_page_wraps(void)
{
    static uint16_t direct[256 * 256];
    static uint8_t indices[256 * 256];
    unsigned short words[192];
    RECT right = { 960, 0, 64, 1 };
    RECT left = { 0, 0, 192, 1 };
    int i;

    for (i = 0; i < 64; i++)
    {
        words[i] = (unsigned short)(0x100 + i);
    }
    vram_mirror_load(&right, (u_long *)words);
    for (i = 0; i < 192; i++)
    {
        words[i] = (unsigned short)(0x200 + i);
    }
    vram_mirror_load(&left, (u_long *)words);
    texture_read_page(960, 0, DC_DEPTH_15, NULL, direct);
    CHECK(direct[0] == 0x100);
    CHECK(direct[63] == 0x13F);
    CHECK(direct[64] == 0x200);
    CHECK(direct[255] == 0x200 + 191);
    load_word(0, 256, 0x0AAA);
    load_word(0, 511, 0x0BBB);
    texture_read_page(0, 256, DC_DEPTH_15, NULL, direct);
    CHECK(direct[0] == 0x0AAA);
    CHECK(direct[255 * 256] == 0x0BBB);
    load_word(128, 300, 0xBA98);
    texture_read_page(128, 256, DC_DEPTH_8, indices, NULL);
    CHECK(indices[44 * 256 + 0] == 0x98);
    CHECK(indices[44 * 256 + 1] == 0xBA);
    texture_read_page(128, 256, DC_DEPTH_4, indices, NULL);
    CHECK(indices[44 * 256 + 0] == 0x8);
    CHECK(indices[44 * 256 + 3] == 0xB);
}

static void test_flush_on_alloc_failure(void)
{
    DcSurfaceTexture out;

    reset_all();
    s_arena_limit = 96u * 1024u;
    CHECK(texture_cache_lookup(0, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 1, &out) == 0);
    CHECK(texture_cache_lookup(64, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 2, &out) == 0);
    CHECK(texture_cache_flushes() == 0);
    CHECK(texture_cache_lookup(128, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 3, &out) == 0);
    CHECK(texture_cache_flushes() == 1);
    CHECK(s_free_all_calls == 1);
    CHECK(out.texture != NULL);
    CHECK(out.format == TEX_FMT_PAL4);
    CHECK(texture_cache_skips() == 0);
}

static void test_mid_frame_flush_deferred(void)
{
    DcSurfaceTexture out;
    DcSurfaceTexture first;

    reset_all();
    s_arena_limit = 96u * 1024u;
    CHECK(texture_cache_lookup(0, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 1, &first) == 0);
    CHECK(texture_cache_lookup(64, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 1, &out) == 0);
    CHECK(texture_cache_lookup(128, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 1, &out) == -1);
    CHECK(s_free_all_calls == 0);
    CHECK(texture_cache_skips() == 1);
    CHECK(texture_cache_flushes() == 1);
    CHECK(texture_cache_lookup(0, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 1, &out) == 0);
    CHECK(out.texture == first.texture);
    CHECK(s_free_all_calls == 0);
    texture_cache_begin_frame();
    CHECK(s_free_all_calls == 1);
    CHECK(texture_cache_lookup(128, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 2, &out) == 0);
    CHECK(out.texture != NULL);
    CHECK(texture_cache_flushes() == 1);
    texture_cache_begin_frame();
    CHECK(s_free_all_calls == 1);
}

static void test_page_changed_mid_frame_gets_new_memory(void)
{
    static uint8_t snapshot[32768];
    DcSurfaceTexture out;
    DcSurfaceTexture again;

    reset_all();
    load_word(192, 0, 0x1111);
    CHECK(texture_cache_lookup(192, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 7, &out) == 0);
    memcpy(snapshot, out.texture, sizeof(snapshot));
    load_word(192, 0, 0x2222);
    CHECK(texture_cache_lookup(192, 0, 0, 480, DC_DEPTH_4, PAL_OPAQUE, 7, &again) == 0);
    CHECK(again.texture != out.texture);
    CHECK(memcmp(snapshot, out.texture, sizeof(snapshot)) == 0);
    CHECK((((const uint8_t *)again.texture)[0] & 0x0F) == 2);
    CHECK((((const uint8_t *)out.texture)[0] & 0x0F) == 1);
}

static void test_clut_changed_mid_frame_gets_new_slot(void)
{
    RECT clut = { 48, 490, 16, 1 };
    unsigned short words[16];
    uint32_t old_entries[16];
    int slot;
    int moved;
    int i;

    reset_all();
    for (i = 0; i < 16; i++)
    {
        words[i] = (unsigned short)(0x0010 + i);
    }
    vram_mirror_load(&clut, (u_long *)words);
    slot = palette_bank_acquire(48, 490, DC_DEPTH_4, PAL_OPAQUE, 9);
    CHECK(slot >= 0);
    memcpy(old_entries, &s_palette[slot * 16], sizeof(old_entries));
    for (i = 0; i < 16; i++)
    {
        words[i] = (unsigned short)(0x7C00 | i);
    }
    vram_mirror_load(&clut, (u_long *)words);
    moved = palette_bank_acquire(48, 490, DC_DEPTH_4, PAL_OPAQUE, 9);
    CHECK(moved >= 0);
    CHECK(moved != slot);
    CHECK(memcmp(old_entries, &s_palette[slot * 16], sizeof(old_entries)) == 0);
    CHECK(s_palette[moved * 16 + 1] == palette_convert(0x7C01, PAL_OPAQUE));
    CHECK(palette_bank_acquire(48, 490, DC_DEPTH_4, PAL_OPAQUE, 9) == moved);
}

static void test_clut_changed_mid_frame_with_no_slot_falls_back(void)
{
    DcSurfaceTexture out;
    int i;

    reset_all();
    for (i = 0; i < 64; i++)
    {
        CHECK(palette_bank_acquire(i * 16, 442, DC_DEPTH_4, PAL_OPAQUE, 4) >= 0);
    }
    load_word(0, 442, 0x1234);
    CHECK(palette_bank_acquire(0, 442, DC_DEPTH_4, PAL_OPAQUE, 4) == -1);
    CHECK(texture_cache_lookup(0, 0, 0, 442, DC_DEPTH_4, PAL_OPAQUE, 4, &out) == 0);
    CHECK(out.format == TEX_FMT_ARGB1555);
}

static void test_clut_stp(void)
{
    RECT clut = { 32, 470, 16, 1 };
    unsigned short words[16];
    int i;

    memset(words, 0, sizeof(words));
    vram_mirror_load(&clut, (u_long *)words);
    CHECK(palette_clut_stp(32, 470, DC_DEPTH_4) == STP_NONE);
    words[3] = 0x8001;
    words[4] = 0x8000;
    vram_mirror_load(&clut, (u_long *)words);
    CHECK(palette_clut_stp(32, 470, DC_DEPTH_4) == STP_ALL);
    words[5] = 0x0001;
    vram_mirror_load(&clut, (u_long *)words);
    CHECK(palette_clut_stp(32, 470, DC_DEPTH_4) == STP_MIXED);
    for (i = 0; i < 16; i++)
    {
        words[i] = 0x1234;
    }
    vram_mirror_load(&clut, (u_long *)words);
    CHECK(palette_clut_stp(32, 470, DC_DEPTH_4) == STP_NONE);
}

int main(void)
{
    test_palette_convert();
    test_twiddle_index();
    test_generation_blocks();
    test_palette_same_clut_reused_then_refilled();
    test_palette_pinning_and_fallback();
    test_palette_8bpp_takes_aligned_run();
    test_page_reuse_and_invalidation();
    test_direct_page_converts();
    test_read_page_wraps();
    test_flush_on_alloc_failure();
    test_mid_frame_flush_deferred();
    test_page_changed_mid_frame_gets_new_memory();
    test_clut_changed_mid_frame_gets_new_slot();
    test_clut_changed_mid_frame_with_no_slot_falls_back();
    test_clut_stp();

    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
