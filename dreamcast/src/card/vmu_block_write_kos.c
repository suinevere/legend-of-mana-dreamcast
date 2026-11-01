/*----------------------
 | vmu_block_write_kos.c
 | Description: KallistiOS's vmu_block_write with a bounded wait for the device's Maple frame, so a VMU pulled mid-write fails the write instead of spinning the writer forever; adapted from KOS's hardware/maple/vmu.c.
 | Author: suinevere
 | Dependencies: kos/thread.h, kos/genwait.h, dc/maple.h, dc/maple/vmu.h, arch/timer.h, string.h, makefile (KOS_CDROM_PATCH_MEMBERS renames KOS's own copy)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <string.h>
#include <kos/thread.h>
#include <kos/genwait.h>
#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <arch/timer.h>

/*----------------------
 | VMU_WRITE_LOCK_MS / VMU_WRITE_REPLY_MS / VMU_WRITE_TRIES / VMU_WRITE_RETRY_MS
 | Description: How long a write waits for the device's frame before giving up, for each phase's reply, and KOS's four tries a block with 100 ms between.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_WRITE_LOCK_MS 500
#define VMU_WRITE_REPLY_MS 100
#define VMU_WRITE_TRIES 4
#define VMU_WRITE_RETRY_MS 100

/*----------------------
 | write_callback
 | Description: KOS's vmu_block_write_callback: hands the answered frame back to the writer and wakes it.
 | Author: suinevere
 | Dependencies: genwait_wake_all
 | Globals: N/A
 | Params: st -- maple state, frm -- answered frame
 | Returns: N/A
 ----------------------*/
static void write_callback(maple_state_t *st, maple_frame_t *frm)
{
    (void)st;
    frm->state = MAPLE_FRAME_UNSENT;
    genwait_wake_all(frm);
}

/*----------------------
 | lock_frame
 | Description: Takes the device's frame, yielding while another request holds it, until VMU_WRITE_LOCK_MS have passed.
 | Author: suinevere
 | Dependencies: maple_frame_lock, thd_pass, timer_ms_gettime64
 | Globals: N/A
 | Params: dev -- VMU
 | Returns: 0 once locked, -1 when it stayed busy
 ----------------------*/
static int lock_frame(maple_device_t *dev)
{
    uint64_t until = timer_ms_gettime64() + VMU_WRITE_LOCK_MS;

    while (maple_frame_lock(&dev->frame) < 0)
    {
        if (timer_ms_gettime64() >= until)
        {
            return -1;
        }
        thd_pass();
    }
    return 0;
}

/*----------------------
 | send_and_wait
 | Description: Queues the locked frame with a command and its words, then waits for the reply; a frame left unanswered is released.
 | Author: suinevere
 | Dependencies: maple_frame_init, maple_queue_frame, genwait_wait, write_callback
 | Globals: N/A
 | Params: dev -- VMU, cmd -- Maple command, words -- parameter words, count -- word count
 | Returns: MAPLE_EOK once answered, MAPLE_ETIMEOUT or MAPLE_EFAIL with the frame released
 ----------------------*/
static int send_and_wait(maple_device_t *dev, int cmd, const uint32_t *words, int count)
{
    uint32_t *send_buf;

    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    memcpy(send_buf, words, (size_t)count * 4);
    dev->frame.cmd = cmd;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = count;
    dev->frame.callback = write_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);
    if (genwait_wait(&dev->frame, "vmu_block_write", VMU_WRITE_REPLY_MS, NULL) < 0 && dev->frame.state != MAPLE_FRAME_UNSENT)
    {
        dev->frame.state = MAPLE_FRAME_VACANT;
        return MAPLE_ETIMEOUT;
    }
    if (dev->frame.state != MAPLE_FRAME_UNSENT)
    {
        dev->frame.state = MAPLE_FRAME_VACANT;
        return MAPLE_EFAIL;
    }
    return MAPLE_EOK;
}

/*----------------------
 | write_once
 | Description: One try at a block: its four 128-byte phases, then the sync command, as KOS's vmu_block_write_internal sends them.
 | Author: suinevere
 | Dependencies: lock_frame, send_and_wait
 | Globals: N/A
 | Params: dev -- VMU, blocknum -- block, buffer -- 512 bytes
 | Returns: MAPLE_EOK, MAPLE_EAGAIN when the frame stayed busy, or the failing phase's error
 ----------------------*/
static int write_once(maple_device_t *dev, uint16_t blocknum, const uint8_t *buffer)
{
    uint32_t words[2 + 128 / 4];
    uint32_t block = ((uint32_t)(blocknum & 0xff) << 24) | ((uint32_t)((blocknum >> 8) & 0xff) << 16);
    int rv = MAPLE_EOK;
    int phase;
    int r;

    if (lock_frame(dev) != 0)
    {
        return MAPLE_EAGAIN;
    }
    for (phase = 0; phase < 4; phase++)
    {
        words[0] = MAPLE_FUNC_MEMCARD;
        words[1] = block | ((uint32_t)phase << 8);
        memcpy(words + 2, buffer + 128 * phase, 128);
        if ((r = send_and_wait(dev, MAPLE_COMMAND_BWRITE, words, 2 + 128 / 4)) != MAPLE_EOK)
        {
            return r;
        }
        if (((maple_response_t *)dev->frame.recv_buf)->response != MAPLE_RESPONSE_OK)
        {
            rv = MAPLE_EFAIL;
        }
    }
    words[0] = MAPLE_FUNC_MEMCARD;
    words[1] = block | (4u << 8);
    if ((r = send_and_wait(dev, MAPLE_COMMAND_BSYNC, words, 2)) != MAPLE_EOK)
    {
        return r;
    }
    dev->frame.state = MAPLE_FRAME_VACANT;
    return rv;
}

/*----------------------
 | vmu_block_write
 | Description: Writes one 512-byte VMU block, trying up to four times as KOS does, each try failing rather than waiting forever on a frame a pulled VMU never frees.
 | Author: suinevere
 | Dependencies: write_once, thd_sleep
 | Globals: N/A
 | Params: dev -- VMU, blocknum -- block, buffer -- 512 bytes
 | Returns: MAPLE_EOK, or the last try's error
 ----------------------*/
int vmu_block_write(maple_device_t *dev, uint16_t blocknum, const uint8_t *buffer)
{
    int rv = MAPLE_EFAIL;
    int i;

    for (i = 0; i < VMU_WRITE_TRIES; i++)
    {
        if ((rv = write_once(dev, blocknum, buffer)) == MAPLE_EOK)
        {
            return rv;
        }
        thd_sleep(VMU_WRITE_RETRY_MS);
    }
    return rv;
}
