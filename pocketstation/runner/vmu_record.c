/*----------------------
 | vmu_record.c
 | Description: Keeps sampled calls of the game's functions: per function, the first call of each path key and up to 8 more per key over the scripts, two at most per script by reservoir sampling, appended to DIR/NAME.rec in the format call_compare.py reads; also every table offset vmu_rom8 read, in DIR/rom-SCRIPT.txt, and the reads of each frame, in DIR/romf-SCRIPT.txt.
 | Author: suinevere
 | Dependencies: vmu_record.h, vmu_hw.h
 | Globals: rec_top
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "vmu_record.h"

/*----------------------
 | REC_MAGIC / REC_VERSION
 | Description: A record's first word ("REC1" little-endian) and the format version after it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define REC_MAGIC 0x31434552u
#define REC_VERSION 1u

/*----------------------
 | REC_EXTRAS / REC_PER_SCRIPT
 | Description: Calls kept per path key beyond its first, over all scripts, and at most per script.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define REC_EXTRAS 8u
#define REC_PER_SCRIPT 2u

/*----------------------
 | HW_*
 | Description: Ids of the logged vmu_hw.h calls, in the order call_compare.HW_NAMES lists them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum {
    HW_PRESENT, HW_IDLE, HW_IRQ_ENABLE, HW_IRQ_DISABLE, HW_IRQ_ACK, HW_IRQ_PENDING, HW_IRQ_LEVELS, HW_IRQ_MASK,
    HW_SET_HANDLER, HW_TICK_MODE, HW_SOUND_PLAY, HW_SOUND_STOP, HW_SYS_FLAGS, HW_POWER, HW_SAVE_READ,
    HW_SAVE_WRITE, HW_BUZZ, HW_LCD, HW_BUTTONS
};

/*----------------------
 | HwCall
 | Description: One logged vmu_* call: id, the depth of the game call that made it, arguments and return.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint8_t id, depth, nargs;
    uint32_t a[2];
    uint32_t ret;
} HwCall;

/*----------------------
 | Rec
 | Description: One kept record, encoded, with the order it was taken in.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint32_t seq, len;
    uint8_t *bytes;
} Rec;

/*----------------------
 | KeyEntry
 | Description: One path key of a function: its bytes and hash, records of it already in the file, calls seen this run after its first, and this run's first and sampled records.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint8_t *key;
    uint32_t hash, in_file, seen, quota, filled;
    Rec first;
    Rec slot[REC_PER_SCRIPT];
} KeyEntry;

/*----------------------
 | FnState
 | Description: One function's key table (open addressing) and whether its existing file was read.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    KeyEntry **tab;
    uint32_t cap, n;
    int loaded;
} FnState;

/*----------------------
 | g_*
 | Description: The recorder's run: output directory, script, synthetic, started, finished and paused flags, calls entered and returned per function, frame, depth, the sampler, the take counter, the vmu_* log, the per-function state and the registry.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
RecFrame *rec_top;
static char g_dir[512];
static char g_script[64];
static int g_synthetic, g_on, g_done, g_paused;
static uint32_t *g_entered, *g_returned;
static uint32_t g_frame, g_depth, g_rng = 0x9E3779B9u, g_seq;
static HwCall *g_log;
static uint32_t g_log_n, g_log_cap;
static FnState *g_fn;
static RecFn *g_reg;

/*----------------------
 | g_rom_seen
 | Description: One bit per table offset vmu_rom8 has read this run, for DIR/rom-SCRIPT.txt.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint8_t g_rom_seen[0x10000 / 8];

/*----------------------
 | g_rom_frame / g_rom_dirty / g_rom_frames_out
 | Description: Reads of each table offset in the frame under way, whether any happened, and DIR/romf-SCRIPT.txt, which takes them a frame at a time.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint16_t g_rom_frame[0x10000];
static int g_rom_dirty;
static FILE *g_rom_frames_out;

/*----------------------
 | die
 | Description: Prints a message and exits with status 9.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: what -- message; arg -- detail
 | Returns: does not return
 ----------------------*/
static void die(const char *what, const char *arg) {
    fprintf(stderr, "vmu_record: %s %s\n", what, arg);
    exit(9);
}

/*----------------------
 | grab
 | Description: malloc that dies when out of memory.
 | Author: suinevere
 | Dependencies: die
 | Globals: N/A
 | Params: n -- bytes
 | Returns: the block
 ----------------------*/
static void *grab(size_t n) {
    void *p = malloc(n ? n : 1u);
    if (!p) die("out of memory", "");
    return p;
}

/*----------------------
 | hash_key
 | Description: FNV-1a over a path key.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: k -- key bytes; n -- count
 | Returns: the hash
 ----------------------*/
static uint32_t hash_key(const uint8_t *k, uint32_t n) {
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < n; i++) h = (h ^ k[i]) * 16777619u;
    return h;
}

/*----------------------
 | next_rand
 | Description: The sampler's xorshift32 step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_rng
 | Params: N/A
 | Returns: the next value
 ----------------------*/
static uint32_t next_rand(void) {
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 17;
    g_rng ^= g_rng << 5;
    return g_rng;
}

/*----------------------
 | key_bytes
 | Description: Bytes a path key of nbits bits takes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: nbits -- bit count
 | Returns: the byte count
 ----------------------*/
static uint32_t key_bytes(uint32_t nbits) { return (nbits + 7u) / 8u; }

/*----------------------
 | grow
 | Description: Doubles a function's key table.
 | Author: suinevere
 | Dependencies: grab
 | Globals: N/A
 | Params: s -- the function's state
 | Returns: N/A
 ----------------------*/
static void grow(FnState *s) {
    uint32_t cap = s->cap ? s->cap * 2u : 16u;
    KeyEntry **tab = grab(cap * sizeof *tab);
    memset(tab, 0, cap * sizeof *tab);
    for (uint32_t i = 0; i < s->cap; i++) {
        KeyEntry *e = s->tab[i];
        if (!e) continue;
        uint32_t j = e->hash & (cap - 1u);
        while (tab[j]) j = (j + 1u) & (cap - 1u);
        tab[j] = e;
    }
    free(s->tab);
    s->tab = tab;
    s->cap = cap;
}

/*----------------------
 | find_key
 | Description: A function's entry for a path key, made when new with this run's quota of extras from what the file already holds.
 | Author: suinevere
 | Dependencies: grow, hash_key, grab
 | Globals: N/A
 | Params: s -- the function's state; k -- key bytes; n -- count; in_file -- records of this key the file adds
 | Returns: the entry
 ----------------------*/
static KeyEntry *find_key(FnState *s, const uint8_t *k, uint32_t n, uint32_t in_file) {
    if ((s->n + 1u) * 2u > s->cap) grow(s);
    uint32_t h = hash_key(k, n), j = h & (s->cap - 1u);
    KeyEntry *e;
    while ((e = s->tab[j])) {
        if (e->hash == h && !memcmp(e->key, k, n)) break;
        j = (j + 1u) & (s->cap - 1u);
    }
    if (!e) {
        e = grab(sizeof *e);
        memset(e, 0, sizeof *e);
        e->key = grab(n);
        memcpy(e->key, k, n);
        e->hash = h;
        s->tab[j] = e;
        s->n++;
    }
    e->in_file += in_file;
    uint32_t extras = e->in_file ? e->in_file - 1u : 0u;
    e->quota = extras >= REC_EXTRAS ? 0u : (REC_EXTRAS - extras < REC_PER_SCRIPT ? REC_EXTRAS - extras : REC_PER_SCRIPT);
    return e;
}

/*----------------------
 | rec_path
 | Description: The path of a function's .rec file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_dir, rec_fn_names
 | Params: out -- buffer; size -- its size; fid -- function id
 | Returns: N/A
 ----------------------*/
static void rec_path(char *out, size_t size, uint16_t fid) {
    snprintf(out, size, "%s/%s.rec", g_dir, rec_fn_names[fid]);
}

/*----------------------
 | rd32 / rd16
 | Description: Little-endian reads from a record.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- bytes
 | Returns: the value
 ----------------------*/
static uint32_t rd32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

/*----------------------
 | load_fn
 | Description: Reads a function's existing .rec file once and counts its records per path key, so this run keeps only what the earlier scripts left room for.
 | Author: suinevere
 | Dependencies: rec_path, find_key, rd32, rd16, grab
 | Globals: g_fn
 | Params: fid -- function id
 | Returns: N/A
 ----------------------*/
static void load_fn(uint16_t fid) {
    FnState *s = &g_fn[fid];
    char path[700];
    s->loaded = 1;
    rec_path(path, sizeof path, fid);
    FILE *f = fopen(path, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = grab((size_t)n);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) die("cannot read", path);
    fclose(f);
    for (long at = 0; at + 14 <= n;) {
        uint32_t len = rd32(buf + at + 4);
        if (rd32(buf + at) != REC_MAGIC || len < 14u || at + (long)len > n || rd16(buf + at + 10) != fid) die("bad record in", path);
        find_key(s, buf + at + 14, key_bytes(rd16(buf + at + 12)), 1u);
        at += len;
    }
    free(buf);
}

/*----------------------
 | Out
 | Description: A growing byte buffer a record is encoded into.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint8_t *p;
    uint32_t n, cap;
} Out;

/*----------------------
 | put
 | Description: Appends bytes to an Out.
 | Author: suinevere
 | Dependencies: grab
 | Globals: N/A
 | Params: o -- the buffer; p -- bytes; n -- count
 | Returns: N/A
 ----------------------*/
static void put(Out *o, const void *p, uint32_t n) {
    if (o->n + n > o->cap) {
        uint32_t cap = o->cap ? o->cap : 4096u;
        while (cap < o->n + n) cap *= 2u;
        uint8_t *q = grab(cap);
        if (o->n) memcpy(q, o->p, o->n);
        free(o->p);
        o->p = q;
        o->cap = cap;
    }
    memcpy(o->p + o->n, p, n);
    o->n += n;
}

/*----------------------
 | put8 / put16 / put32
 | Description: Appends a little-endian value to an Out.
 | Author: suinevere
 | Dependencies: put
 | Globals: N/A
 | Params: o -- the buffer; v -- the value
 | Returns: N/A
 ----------------------*/
static void put8(Out *o, uint32_t v) {
    uint8_t b = (uint8_t)v;
    put(o, &b, 1u);
}
static void put16(Out *o, uint32_t v) {
    put8(o, v);
    put8(o, v >> 8);
}
static void put32(Out *o, uint32_t v) {
    put16(o, v);
    put16(o, v >> 16);
}

/*----------------------
 | put_str
 | Description: Appends a string as a length byte and its bytes.
 | Author: suinevere
 | Dependencies: put, put8
 | Globals: N/A
 | Params: o -- the buffer; s -- the string
 | Returns: N/A
 ----------------------*/
static void put_str(Out *o, const char *s) {
    uint32_t n = (uint32_t)strlen(s);
    if (n > 255u) n = 255u;
    put8(o, n);
    put(o, s, n);
}

/*----------------------
 | encode
 | Description: Encodes a finished call as a record (layout in call_compare.py's header).
 | Author: suinevere
 | Dependencies: put*, rec_fn_names
 | Globals: g_script, g_synthetic, g_log, g_seq
 | Params: f -- the frame; result -- return bytes or null; size -- their count
 | Returns: the record
 ----------------------*/
static Rec encode(const RecFrame *f, const void *result, uint8_t size) {
    Out o = {0, 0, 0};
    put32(&o, REC_MAGIC);
    put32(&o, 0u);
    put16(&o, REC_VERSION);
    put16(&o, f->fid);
    put16(&o, f->nbits);
    put(&o, f->bits, key_bytes(f->nbits));
    put8(&o, f->depth);
    put8(&o, g_synthetic ? 1u : 0u);
    put32(&o, f->frame);
    put_str(&o, g_script);
    put_str(&o, rec_fn_names[f->fid]);
    put8(&o, f->nargs);
    for (uint32_t i = 0; i < f->nargs; i++) {
        const RecArg *a = &f->args[i];
        put8(&o, a->kind);
        put8(&o, a->size);
        put(&o, a->before, a->size);
        if (a->kind == 1u) put(&o, a->after, a->size);
    }
    put8(&o, size);
    if (size) put(&o, result, size);
    put(&o, f->ram, sizeof f->ram);
    put(&o, f->fb, sizeof f->fb);
    put(&o, vmu_ram, sizeof vmu_ram);
    put(&o, vmu_fb(), VMU_FB_BYTES);
    put32(&o, g_log_n - f->hw_start);
    for (uint32_t i = f->hw_start; i < g_log_n; i++) {
        const HwCall *c = &g_log[i];
        put8(&o, c->id);
        put8(&o, c->depth);
        put8(&o, c->nargs);
        for (uint32_t j = 0; j < c->nargs; j++) put32(&o, c->a[j]);
        put32(&o, c->ret);
    }
    o.p[4] = (uint8_t)o.n;
    o.p[5] = (uint8_t)(o.n >> 8);
    o.p[6] = (uint8_t)(o.n >> 16);
    o.p[7] = (uint8_t)(o.n >> 24);
    Rec r = {g_seq++, o.n, o.p};
    return r;
}

/*----------------------
 | rec_enter
 | Description: Opens a call frame: pushes it, clears its bits, snapshots vmu_ram and the frame buffer and counts the entry.
 | Author: suinevere
 | Dependencies: die, key_bytes
 | Globals: rec_top, g_depth, g_log_n, g_frame, g_entered
 | Params: f -- the frame; fid -- function id; nbits -- path-key bits the function's probes use
 | Returns: N/A
 ----------------------*/
void rec_enter(RecFrame *f, uint16_t fid, uint16_t nbits) {
    if (nbits > REC_MAX_BITS) die("too many path bits for", rec_fn_names[fid]);
    f->fid = fid;
    f->nbits = nbits;
    memset(f->bits, 0, key_bytes(nbits));
    f->nargs = 0;
    f->depth = (uint8_t)(g_depth > 255u ? 255u : g_depth);
    memcpy(f->ram, vmu_ram, sizeof f->ram);
    memcpy(f->fb, vmu_fb(), sizeof f->fb);
    f->hw_start = g_log_n;
    f->frame = g_frame;
    f->parent = rec_top;
    rec_top = f;
    g_depth++;
    if (g_on && !g_paused) g_entered[fid]++;
}

/*----------------------
 | rec_arg
 | Description: Records an argument's value.
 | Author: suinevere
 | Dependencies: die
 | Globals: N/A
 | Params: f -- the frame; p -- the argument; size -- its size
 | Returns: N/A
 ----------------------*/
void rec_arg(RecFrame *f, const void *p, uint8_t size) {
    if (f->nargs == REC_MAX_ARGS || size > REC_ARG_BYTES) die("argument too large for", rec_fn_names[f->fid]);
    RecArg *a = &f->args[f->nargs++];
    a->kind = 0;
    a->size = size;
    a->ptr = 0;
    memcpy(a->before, p, size);
}

/*----------------------
 | rec_arg_ptr
 | Description: Records a pointer argument by its pointee, read now and again when the call returns; a null pointer is recorded as kind 2 with no bytes.
 | Author: suinevere
 | Dependencies: die
 | Globals: N/A
 | Params: f -- the frame; p -- the pointer; size -- the pointee's size
 | Returns: N/A
 ----------------------*/
void rec_arg_ptr(RecFrame *f, const void *p, uint8_t size) {
    if (f->nargs == REC_MAX_ARGS || size > REC_ARG_BYTES) die("bad pointer argument for", rec_fn_names[f->fid]);
    RecArg *a = &f->args[f->nargs++];
    a->kind = p ? 1u : 2u;
    a->size = p ? size : 0u;
    a->ptr = p;
    if (p) memcpy(a->before, p, size);
}

/*----------------------
 | rec_leave
 | Description: Closes the call frame: pops it, counts the return and keeps the call when its path key is new to the file, or when the per-script reservoir of its key picks it.
 | Author: suinevere
 | Dependencies: load_fn, find_key, encode, next_rand
 | Globals: rec_top, g_depth, g_log_n, g_fn, g_returned
 | Params: f -- the frame; result -- the return value or null; size -- its size or 0
 | Returns: N/A
 ----------------------*/
void rec_leave(RecFrame *f, const void *result, uint8_t size) {
    rec_top = f->parent;
    g_depth--;
    for (uint32_t i = 0; i < f->nargs; i++) {
        if (f->args[i].kind == 1u) memcpy(f->args[i].after, f->args[i].ptr, f->args[i].size);
    }
    if (g_on && !g_paused) {
        FnState *s = &g_fn[f->fid];
        g_returned[f->fid]++;
        if (!s->loaded) load_fn(f->fid);
        KeyEntry *e = find_key(s, f->bits, key_bytes(f->nbits), 0u);
        if (!e->in_file && !e->first.bytes) {
            e->first = encode(f, result, size);
        } else if (e->quota) {
            e->seen++;
            if (e->filled < e->quota) {
                e->slot[e->filled++] = encode(f, result, size);
            } else {
                uint32_t j = next_rand() % e->seen;
                if (j < e->quota) {
                    free(e->slot[j].bytes);
                    e->slot[j] = encode(f, result, size);
                }
            }
        }
    }
    if (!rec_top) g_log_n = 0;
}

/*----------------------
 | rec_register
 | Description: Enters a wrapper's address in the registry, which names vmu_set_handler's argument and lets record_synthetic reach static functions.
 | Author: suinevere
 | Dependencies: grab
 | Globals: g_reg
 | Params: fid -- function id; fn -- the wrapper
 | Returns: N/A
 ----------------------*/
void rec_register(uint16_t fid, RecFn fn) {
    if (!g_reg) {
        g_reg = grab(rec_fn_count * sizeof *g_reg);
        memset(g_reg, 0, rec_fn_count * sizeof *g_reg);
    }
    if (fid < rec_fn_count) g_reg[fid] = fn;
}

/*----------------------
 | rec_lookup
 | Description: A wrapper by function name.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_reg
 | Params: name -- the C function name
 | Returns: the wrapper, or null
 ----------------------*/
RecFn rec_lookup(const char *name) {
    for (uint16_t i = 0; i < rec_fn_count; i++) {
        if (!strcmp(rec_fn_names[i], name)) return g_reg ? g_reg[i] : 0;
    }
    return 0;
}

/*----------------------
 | fid_of
 | Description: The function id of a registered wrapper: 0xFFFF for null, 0xFFFE for an address the registry does not hold.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_reg
 | Params: fn -- the wrapper
 | Returns: the id
 ----------------------*/
static uint32_t fid_of(RecFn fn) {
    if (!fn) return 0xFFFFu;
    for (uint16_t i = 0; g_reg && i < rec_fn_count; i++) {
        if (g_reg[i] == fn) return i;
    }
    return 0xFFFEu;
}

/*----------------------
 | at_exit
 | Description: The atexit hook that writes the records when the host stops the process (a power exit).
 | Author: suinevere
 | Dependencies: rec_finish
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void at_exit(void) { rec_finish(); }

/*----------------------
 | rec_begin
 | Description: Starts recording into dir for one script, making dir if needed and seeding the sampler from the script name.
 | Author: suinevere
 | Dependencies: grab, hash_key, at_exit
 | Globals: g_dir, g_script, g_synthetic, g_on, g_rng, g_fn, g_entered, g_returned
 | Params: dir -- output directory; script -- script name; synthetic -- nonzero for record_synthetic
 | Returns: N/A
 ----------------------*/
void rec_begin(const char *dir, const char *script, int synthetic) {
    snprintf(g_dir, sizeof g_dir, "%s", dir);
    snprintf(g_script, sizeof g_script, "%s", script);
    g_synthetic = synthetic;
    g_rng = hash_key((const uint8_t *)g_script, (uint32_t)strlen(g_script)) | 1u;
    if (mkdir(g_dir, 0777) && errno != EEXIST) die("cannot make", g_dir);
    g_fn = grab(rec_fn_count * sizeof *g_fn);
    memset(g_fn, 0, rec_fn_count * sizeof *g_fn);
    g_entered = grab(rec_fn_count * sizeof *g_entered);
    memset(g_entered, 0, rec_fn_count * sizeof *g_entered);
    g_returned = grab(rec_fn_count * sizeof *g_returned);
    memset(g_returned, 0, rec_fn_count * sizeof *g_returned);
    g_on = 1;
    atexit(at_exit);
}

/*----------------------
 | rec_pause
 | Description: Stops or resumes keeping and counting calls, for record_synthetic's setup.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_paused
 | Params: on -- nonzero to pause
 | Returns: N/A
 ----------------------*/
void rec_pause(int on) { g_paused = on; }

/*----------------------
 | flush_rom_frame
 | Description: Appends the frame's table reads to DIR/romf-SCRIPT.txt as FRAME START END READS lines, hex but READS, END exclusive, and clears them.
 | Author: suinevere
 | Dependencies: die
 | Globals: g_rom_frame, g_rom_dirty, g_rom_frames_out, g_frame, g_dir, g_script, g_on
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void flush_rom_frame(void) {
    if (!g_rom_dirty || !g_on) return;
    if (!g_rom_frames_out) {
        char path[700];
        snprintf(path, sizeof path, "%s/romf-%s.txt", g_dir, g_script);
        g_rom_frames_out = fopen(path, "w");
        if (!g_rom_frames_out) die("cannot write", path);
    }
    uint32_t start = 0, reads = 0;
    for (uint32_t off = 0; off <= 0x10000u; off++) {
        uint16_t n = off < 0x10000u ? g_rom_frame[off] : 0;
        if (n && !reads) start = off;
        if (!n && reads) {
            fprintf(g_rom_frames_out, "%u %04X %04X %u\n", g_frame, start, off, reads);
            reads = 0;
        }
        if (n) {
            reads += n;
            g_rom_frame[off] = 0;
        }
    }
    g_rom_dirty = 0;
}

/*----------------------
 | rec_set_frame
 | Description: The frame number the next calls carry; a new number first writes out the last frame's table reads.
 | Author: suinevere
 | Dependencies: flush_rom_frame
 | Globals: g_frame
 | Params: frame -- frame number
 | Returns: N/A
 ----------------------*/
void rec_set_frame(uint32_t frame) {
    if (frame != g_frame) flush_rom_frame();
    g_frame = frame;
}

/*----------------------
 | rec_reset
 | Description: Drops every call in progress after a longjmp past them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: rec_top, g_depth, g_log_n
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void rec_reset(void) {
    rec_top = 0;
    g_depth = 0;
    g_log_n = 0;
}

/*----------------------
 | by_seq
 | Description: qsort order of kept records: the order they were taken.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: a, b -- Rec pointers
 | Returns: comparison
 ----------------------*/
static int by_seq(const void *a, const void *b) {
    const Rec *x = a, *y = b;
    return x->seq < y->seq ? -1 : x->seq > y->seq;
}

/*----------------------
 | write_counts
 | Description: Writes DIR/calls-SCRIPT.txt: NAME ENTERED RETURNED for every function entered this run.
 | Author: suinevere
 | Dependencies: die
 | Globals: g_dir, g_script, g_entered, g_returned
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void write_counts(void) {
    char path[700];
    snprintf(path, sizeof path, "%s/calls-%s.txt", g_dir, g_script);
    FILE *f = fopen(path, "w");
    if (!f) die("cannot write", path);
    for (uint16_t fid = 0; fid < rec_fn_count; fid++) {
        if (g_entered[fid]) fprintf(f, "%s %u %u\n", rec_fn_names[fid], g_entered[fid], g_returned[fid]);
    }
    fclose(f);
}

/*----------------------
 | write_rom_reads
 | Description: Writes DIR/rom-SCRIPT.txt: each run of table offsets vmu_rom8 read this run as START END, hex, END exclusive.
 | Author: suinevere
 | Dependencies: die
 | Globals: g_dir, g_script, g_rom_seen
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void write_rom_reads(void) {
    char path[700];
    snprintf(path, sizeof path, "%s/rom-%s.txt", g_dir, g_script);
    FILE *f = fopen(path, "w");
    if (!f) die("cannot write", path);
    uint32_t start = 0;
    int in = 0;
    for (uint32_t off = 0; off <= 0x10000u; off++) {
        int seen = off < 0x10000u && (g_rom_seen[off >> 3] >> (off & 7u) & 1u);
        if (seen && !in) start = off;
        if (!seen && in) fprintf(f, "%04X %04X\n", start, off);
        in = seen;
    }
    fclose(f);
}

/*----------------------
 | rec_finish
 | Description: Appends each function's kept records, in the order taken, to DIR/NAME.rec and writes the call counts and table reads; once.
 | Author: suinevere
 | Dependencies: rec_path, by_seq, write_counts, write_rom_reads, grab, die
 | Globals: g_on, g_done, g_fn
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void rec_finish(void) {
    if (!g_on || g_done) return;
    g_done = 1;
    for (uint16_t fid = 0; fid < rec_fn_count; fid++) {
        FnState *s = &g_fn[fid];
        uint32_t n = 0;
        for (uint32_t i = 0; i < s->cap; i++) {
            KeyEntry *e = s->tab[i];
            if (e) n += (e->first.bytes ? 1u : 0u) + e->filled;
        }
        if (!n) continue;
        Rec *all = grab(n * sizeof *all);
        uint32_t k = 0;
        for (uint32_t i = 0; i < s->cap; i++) {
            KeyEntry *e = s->tab[i];
            if (!e) continue;
            if (e->first.bytes) all[k++] = e->first;
            for (uint32_t j = 0; j < e->filled; j++) all[k++] = e->slot[j];
        }
        qsort(all, n, sizeof *all, by_seq);
        char path[700];
        rec_path(path, sizeof path, fid);
        FILE *f = fopen(path, "ab");
        if (!f) die("cannot write", path);
        for (uint32_t i = 0; i < n; i++) {
            if (fwrite(all[i].bytes, 1, all[i].len, f) != all[i].len) die("cannot write", path);
        }
        fclose(f);
        free(all);
    }
    write_counts();
    write_rom_reads();
    flush_rom_frame();
    if (g_rom_frames_out) fclose(g_rom_frames_out);
    g_rom_frames_out = 0;
}

/*----------------------
 | log_call
 | Description: Appends a vmu_* call to the log, before it runs so calls it makes (the handler's) follow it.
 | Author: suinevere
 | Dependencies: grab
 | Globals: g_log, g_log_n, g_depth
 | Params: id -- HW_* id; nargs -- argument count; a0, a1 -- arguments
 | Returns: the call's index, for its return
 ----------------------*/
static uint32_t log_call(uint8_t id, uint8_t nargs, uint32_t a0, uint32_t a1) {
    if (!rec_top) return UINT32_MAX;
    if (g_log_n == g_log_cap) {
        uint32_t cap = g_log_cap ? g_log_cap * 2u : 4096u;
        HwCall *q = grab(cap * sizeof *q);
        if (g_log_n) memcpy(q, g_log, g_log_n * sizeof *q);
        free(g_log);
        g_log = q;
        g_log_cap = cap;
    }
    HwCall *c = &g_log[g_log_n];
    c->id = id;
    c->depth = (uint8_t)(g_depth - 1u > 255u ? 255u : g_depth - 1u);
    c->nargs = nargs;
    c->a[0] = a0;
    c->a[1] = a1;
    c->ret = 0;
    return g_log_n++;
}

/*----------------------
 | set_ret
 | Description: Stores a logged call's return, if the call is still in the log.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_log, g_log_n
 | Params: i -- the call's index; v -- its return
 | Returns: v
 ----------------------*/
static uint32_t set_ret(uint32_t i, uint32_t v) {
    if (i < g_log_n) g_log[i].ret = v;
    return v;
}

/*----------------------
 | rec_vmu_*
 | Description: Each logs its call (id, arguments, then its return) and calls the real vmu_hw.h function.
 | Author: suinevere
 | Dependencies: log_call, set_ret, vmu_hw.h
 | Globals: g_log
 | Params: as vmu_hw.h
 | Returns: as vmu_hw.h
 ----------------------*/
uint8_t rec_vmu_buttons(void) { return (uint8_t)set_ret(log_call(HW_BUTTONS, 0, 0, 0), vmu_buttons()); }
void rec_vmu_present(void) {
    log_call(HW_PRESENT, 0, 0, 0);
    vmu_present();
}
void rec_vmu_idle(void) {
    log_call(HW_IDLE, 0, 0, 0);
    vmu_idle();
}
void rec_vmu_irq_enable(uint16_t lines) {
    log_call(HW_IRQ_ENABLE, 1, lines, 0);
    vmu_irq_enable(lines);
}
void rec_vmu_irq_disable(uint16_t lines) {
    log_call(HW_IRQ_DISABLE, 1, lines, 0);
    vmu_irq_disable(lines);
}
void rec_vmu_irq_ack(uint16_t lines) {
    log_call(HW_IRQ_ACK, 1, lines, 0);
    vmu_irq_ack(lines);
}
uint16_t rec_vmu_irq_pending(void) { return (uint16_t)set_ret(log_call(HW_IRQ_PENDING, 0, 0, 0), vmu_irq_pending()); }
uint16_t rec_vmu_irq_levels(void) { return (uint16_t)set_ret(log_call(HW_IRQ_LEVELS, 0, 0, 0), vmu_irq_levels()); }
uint16_t rec_vmu_irq_mask(void) { return (uint16_t)set_ret(log_call(HW_IRQ_MASK, 0, 0, 0), vmu_irq_mask()); }
void rec_vmu_set_handler(void (*irq)(void)) {
    log_call(HW_SET_HANDLER, 1, fid_of(irq), 0);
    vmu_set_handler(irq);
}
void rec_vmu_tick_mode(uint8_t mode) {
    log_call(HW_TICK_MODE, 1, mode, 0);
    vmu_tick_mode(mode);
}
void rec_vmu_sound_play(uint32_t samples, uint16_t counts_per_sample) {
    log_call(HW_SOUND_PLAY, 2, samples, counts_per_sample);
    vmu_sound_play(samples, counts_per_sample);
}
void rec_vmu_sound_stop(void) {
    log_call(HW_SOUND_STOP, 0, 0, 0);
    vmu_sound_stop();
}
uint8_t rec_vmu_sys_flags(void) { return (uint8_t)set_ret(log_call(HW_SYS_FLAGS, 0, 0, 0), vmu_sys_flags()); }
void rec_vmu_power(uint8_t mode) {
    log_call(HW_POWER, 1, mode, 0);
    vmu_power(mode);
}
uint8_t rec_vmu_save_read(uint16_t sector, uint16_t dst) {
    uint32_t i = log_call(HW_SAVE_READ, 2, sector, dst);
    return (uint8_t)set_ret(i, vmu_save_read(sector, dst));
}
uint8_t rec_vmu_save_write(uint16_t sector, uint16_t src) {
    uint32_t i = log_call(HW_SAVE_WRITE, 2, sector, src);
    return (uint8_t)set_ret(i, vmu_save_write(sector, src));
}
void rec_vmu_buzz(uint8_t tone) {
    log_call(HW_BUZZ, 1, tone, 0);
    vmu_buzz(tone);
}
void rec_vmu_lcd(uint8_t on) {
    log_call(HW_LCD, 1, on, 0);
    vmu_lcd(on);
}

/*----------------------
 | __wrap_vmu_rom8
 | Description: The game's vmu_rom8 in recording builds, linked with --wrap=vmu_rom8: marks the offset in g_rom_seen, counts it for the frame, then reads through the real one.
 | Author: suinevere
 | Dependencies: vmu_hw.h (__real_vmu_rom8)
 | Globals: g_rom_seen, g_rom_frame, g_rom_dirty
 | Params: off -- table offset
 | Returns: the byte
 ----------------------*/
uint8_t __real_vmu_rom8(uint16_t off);
uint8_t __wrap_vmu_rom8(uint16_t off) {
    g_rom_seen[off >> 3] |= (uint8_t)(1u << (off & 7u));
    if (g_rom_frame[off] != 0xFFFFu) g_rom_frame[off]++;
    g_rom_dirty = 1;
    return __real_vmu_rom8(off);
}
