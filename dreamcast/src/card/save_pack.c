/*----------------------
 | save_pack.c
 | Description: LZSS codec for card files: groups of eight items behind a flag byte (bit set for a literal, LSB first), each match two bytes of 12-bit distance and 4-bit length with 255-run length extension.
 | Author: suinevere
 | Dependencies: save_pack.h
 | Globals: s_head, s_prev
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "save_pack.h"

/*----------------------
 | PACK_WINDOW / PACK_MIN_MATCH / PACK_MAX_MATCH / PACK_CHAIN / PACK_HASH_BITS / PACK_LEN_EXTEND
 | Description: Match window and length limits, how many earlier positions a match search tries, the hash table size, and the length nibble that says extension bytes follow.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PACK_WINDOW 4096
#define PACK_MIN_MATCH 3
#define PACK_MAX_MATCH 4096
#define PACK_CHAIN 64
#define PACK_HASH_BITS 12
#define PACK_LEN_EXTEND 15

/*----------------------
 | s_head / s_prev
 | Description: Most recent position for each 3-byte hash, and for each window slot the position before it with the same hash.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_head[1 << PACK_HASH_BITS];
static int s_prev[PACK_WINDOW];

/*----------------------
 | pack_hash
 | Description: Hashes the three bytes at p into a head table index.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- three readable bytes
 | Returns: 0 .. (1 << PACK_HASH_BITS) - 1
 ----------------------*/
static unsigned int pack_hash(const unsigned char *p)
{
    unsigned int key = ((unsigned int)p[0] << 16) | ((unsigned int)p[1] << 8) | p[2];

    return (key * 2654435761u) >> (32 - PACK_HASH_BITS);
}

/*----------------------
 | pack_insert
 | Description: Records position pos under its hash so later searches can find it.
 | Author: suinevere
 | Dependencies: pack_hash
 | Globals: s_head, s_prev
 | Params: in / size -- input, pos -- position to record
 | Returns: N/A
 ----------------------*/
static void pack_insert(const unsigned char *in, unsigned int size, unsigned int pos)
{
    unsigned int h;

    if (pos + PACK_MIN_MATCH > size)
    {
        return;
    }
    h = pack_hash(in + pos);
    s_prev[pos % PACK_WINDOW] = s_head[h];
    s_head[h] = (int)pos;
}

/*----------------------
 | pack_find
 | Description: Longest earlier match for the bytes at pos within the window, searching at most PACK_CHAIN candidates.
 | Author: suinevere
 | Dependencies: pack_hash
 | Globals: s_head, s_prev
 | Params: in / size -- input, pos -- position to match, dist -- distance of the match out
 | Returns: match length, 0 when shorter than PACK_MIN_MATCH
 ----------------------*/
static unsigned int pack_find(const unsigned char *in, unsigned int size, unsigned int pos, unsigned int *dist)
{
    unsigned int max = size - pos < PACK_MAX_MATCH ? size - pos : PACK_MAX_MATCH;
    unsigned int best = 0;
    int cand;
    int depth;

    if (max < PACK_MIN_MATCH)
    {
        return 0;
    }
    cand = s_head[pack_hash(in + pos)];
    for (depth = 0; depth < PACK_CHAIN && cand >= 0 && pos - (unsigned int)cand <= PACK_WINDOW; depth++)
    {
        unsigned int len = 0;
        int next;

        while (len < max && in[(unsigned int)cand + len] == in[pos + len])
        {
            len++;
        }
        if (len > best)
        {
            best = len;
            *dist = pos - (unsigned int)cand;
            if (len == max)
            {
                break;
            }
        }
        next = s_prev[(unsigned int)cand % PACK_WINDOW];
        if (next >= cand)
        {
            break;
        }
        cand = next;
    }
    return best >= PACK_MIN_MATCH ? best : 0;
}

/*----------------------
 | save_pack_bound
 | Description: Largest output save_pack can produce for size input bytes: every byte a literal plus a flag byte per eight.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: size -- input bytes
 | Returns: bytes
 ----------------------*/
unsigned int save_pack_bound(unsigned int size)
{
    return size + (size + 7) / 8;
}

/*----------------------
 | save_pack
 | Description: Compresses size bytes of in into out.
 | Author: suinevere
 | Dependencies: pack_find, pack_insert
 | Globals: s_head
 | Params: in / size -- input, out / cap -- output buffer and its capacity
 | Returns: compressed bytes, or -1 when out is too small
 ----------------------*/
int save_pack(const unsigned char *in, unsigned int size, unsigned char *out, unsigned int cap)
{
    unsigned int pos = 0;
    unsigned int o = 0;
    unsigned int flag_at = 0;
    int bit = 8;
    unsigned int i;

    for (i = 0; i < (1u << PACK_HASH_BITS); i++)
    {
        s_head[i] = -1;
    }
    while (pos < size)
    {
        unsigned int dist = 0;
        unsigned int len;

        if (bit == 8)
        {
            if (o >= cap)
            {
                return -1;
            }
            flag_at = o++;
            out[flag_at] = 0;
            bit = 0;
        }
        len = pack_find(in, size, pos, &dist);
        if (len == 0)
        {
            if (o >= cap)
            {
                return -1;
            }
            out[flag_at] |= (unsigned char)(1 << bit);
            out[o++] = in[pos];
            pack_insert(in, size, pos);
            pos++;
        }
        else
        {
            unsigned int nib = len - PACK_MIN_MATCH < PACK_LEN_EXTEND ? len - PACK_MIN_MATCH : PACK_LEN_EXTEND;
            unsigned int rest = len - PACK_MIN_MATCH - nib;

            if (o + 2 > cap)
            {
                return -1;
            }
            out[o++] = (unsigned char)((dist - 1) & 0xFF);
            out[o++] = (unsigned char)((((dist - 1) >> 4) & 0xF0) | nib);
            if (nib == PACK_LEN_EXTEND)
            {
                for (;;)
                {
                    unsigned int step = rest < 255 ? rest : 255;

                    if (o >= cap)
                    {
                        return -1;
                    }
                    out[o++] = (unsigned char)step;
                    rest -= step;
                    if (step < 255)
                    {
                        break;
                    }
                }
            }
            for (i = 0; i < len; i++)
            {
                pack_insert(in, size, pos + i);
            }
            pos += len;
        }
        bit++;
    }
    return (int)o;
}

/*----------------------
 | save_unpack
 | Description: Decompresses save_pack output into exactly out_size bytes, refusing streams that run short, point before the start or overrun.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: in / size -- compressed bytes, out / out_size -- destination and the uncompressed size
 | Returns: 0 on success, -1 on a corrupt stream
 ----------------------*/
int save_unpack(const unsigned char *in, unsigned int size, unsigned char *out, unsigned int out_size)
{
    unsigned int i = 0;
    unsigned int o = 0;
    unsigned int flags = 0;
    int bit = 8;

    while (o < out_size)
    {
        if (bit == 8)
        {
            if (i >= size)
            {
                return -1;
            }
            flags = in[i++];
            bit = 0;
        }
        if (flags & (1u << bit))
        {
            if (i >= size)
            {
                return -1;
            }
            out[o++] = in[i++];
        }
        else
        {
            unsigned int dist;
            unsigned int len;

            if (i + 2 > size)
            {
                return -1;
            }
            dist = ((unsigned int)in[i] | (((unsigned int)in[i + 1] & 0xF0) << 4)) + 1;
            len = (in[i + 1] & 0x0F) + PACK_MIN_MATCH;
            i += 2;
            if ((in[i - 1] & 0x0F) == PACK_LEN_EXTEND)
            {
                unsigned int step;

                do
                {
                    if (i >= size)
                    {
                        return -1;
                    }
                    step = in[i++];
                    len += step;
                } while (step == 255);
            }
            if (dist > o || len > out_size - o)
            {
                return -1;
            }
            while (len-- > 0)
            {
                out[o] = out[o - dist];
                o++;
            }
        }
        bit++;
    }
    return 0;
}
