/*----------------------
 | ot_walk.c
 | Description: Walks a PS1 ordering-table packet chain, counting non-empty packets, and halts on a chain that does not terminate within a bounded number of packets.
 | Author: suinevere
 | Dependencies: ot_walk.h, gpu_tag.h, dc_system.c (dc_halt)
 | Globals: s_host_base (LOM_HOST_TEST only)
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "ot_walk.h"
#include "gpu_tag.h"

/*----------------------
 | dc_halt
 | Description: Declared locally without dc_system.h's noreturn attribute, so the code ot_walk_count runs after calling it stays live when a host test mocks it to return.
 | Author: suinevere
 | Dependencies: dc_system.c (dc_halt)
 | Globals: N/A
 | Params: message -- halt reason
 | Returns: N/A
 ----------------------*/
extern void dc_halt(const char *message);

/*----------------------
 | OT_WALK_MAX_PACKETS
 | Description: Most packets ot_walk_count visits before treating the chain as a non-terminating loop.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define OT_WALK_MAX_PACKETS 65536

#ifdef LOM_HOST_TEST

/*----------------------
 | s_host_base
 | Description: Host address the 24-bit tag field 0 maps to, set by ot_walk_set_base for host tests.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uintptr_t s_host_base;

/*----------------------
 | ot_walk_set_base
 | Description: Sets s_host_base, the host address the 24-bit tag field 0 maps to, for host tests.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_host_base
 | Params: base -- host address field 0 maps to
 | Returns: N/A
 ----------------------*/
void ot_walk_set_base(uintptr_t base)
{
    s_host_base = base;
}

/*----------------------
 | ot_walk_pointer
 | Description: Resolves a tag's 24-bit address field to a host pointer relative to s_host_base, standing in for gpu_tag_pointer under a host test.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_host_base
 | Params: addr -- 24-bit tag address field, already checked against GPU_TAG_END
 | Returns: pointer to the next packet
 ----------------------*/
static const u_long *ot_walk_pointer(uint32_t addr)
{
    return (const u_long *)(s_host_base + addr);
}

#else

/*----------------------
 | ot_walk_pointer
 | Description: Resolves a tag's 24-bit address field to a Dreamcast RAM pointer.
 | Author: suinevere
 | Dependencies: gpu_tag_pointer
 | Globals: N/A
 | Params: addr -- 24-bit tag address field, already checked against GPU_TAG_END
 | Returns: pointer to the next packet
 ----------------------*/
static const u_long *ot_walk_pointer(uint32_t addr)
{
    return (const u_long *)gpu_tag_pointer(addr);
}

#endif

/*----------------------
 | ot_walk_count
 | Description: Follows a packet chain from ot_head until the raw 24-bit tag field is GPU_TAG_END, counting packets whose length is non-zero.
 | Author: suinevere
 | Dependencies: gpu_tag_addr, gpu_tag_len, ot_walk_pointer, dc_halt
 | Globals: N/A
 | Params: ot_head -- first packet of the chain
 | Returns: packets visited with a non-zero length
 ----------------------*/
unsigned ot_walk_count(const u_long *ot_head)
{
    const u_long *p = ot_head;
    unsigned count = 0;
    unsigned visited = 0;

    for (;;)
    {
        uint32_t raw = gpu_tag_addr((uint32_t)p[0]);

        if (gpu_tag_len((uint32_t)p[0]) != 0)
        {
            count++;
        }
        if (raw == GPU_TAG_END)
        {
            break;
        }
        if (++visited >= OT_WALK_MAX_PACKETS)
        {
            dc_halt("ot_walk: loop");
            return count;
        }
        p = ot_walk_pointer(raw);
    }
    return count;
}
