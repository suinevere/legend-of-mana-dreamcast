/*----------------------
 | log_ring.c
 | Description: The queued serial log ring; a flush stops waiting on a FIFO that never drains, since with no cable and RTS/CTS on the console's port never takes a byte.
 | Author: suinevere
 | Dependencies: log_ring.h, stddef.h
 | Globals: s_port, s_ring, s_head, s_tail, s_dead
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stddef.h>
#include "log_ring.h"

/*----------------------
 | LOG_RING_BYTES / s_port / s_ring / s_head / s_tail / s_dead
 | Description: The bound port, the bytes waiting for it, and whether a flush has given it up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOG_RING_BYTES 16384u
static const LogPort *s_port;
static char s_ring[LOG_RING_BYTES];
static unsigned s_head;
static unsigned s_tail;
static int s_dead;

/*----------------------
 | log_ring_bind
 | Description: Sets the port the ring feeds; until then every call does nothing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_port
 | Params: port -- the transmit FIFO and helpers
 | Returns: N/A
 ----------------------*/
void log_ring_bind(const LogPort *port)
{
    s_port = port;
}

/*----------------------
 | log_ring_drain
 | Description: Moves queued bytes into the FIFO while it has room, never waiting.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_port, s_ring, s_head, s_tail, s_dead
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void log_ring_drain(void)
{
    int old;

    if (s_port == NULL || s_dead)
    {
        return;
    }
    old = s_port->lock();
    while (s_tail != s_head && s_port->tx_room() > 0u)
    {
        s_port->tx_put((uint8_t)s_ring[s_tail]);
        s_tail = (s_tail + 1u) % LOG_RING_BYTES;
    }
    s_port->unlock(old);
}

/*----------------------
 | log_ring_flush
 | Description: Drains every queued byte, but discards the rest and marks the port dead after LOG_RING_STALL_US without progress.
 | Author: suinevere
 | Dependencies: log_ring_drain
 | Globals: s_port, s_head, s_tail, s_dead
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void log_ring_flush(void)
{
    unsigned long long since;
    unsigned last;

    if (s_port == NULL || s_dead)
    {
        return;
    }
    since = s_port->now_us();
    last = s_tail;
    while (s_tail != s_head)
    {
        log_ring_drain();
        if (s_tail != last)
        {
            last = s_tail;
            since = s_port->now_us();
        }
        else if (s_port->now_us() - since > LOG_RING_STALL_US)
        {
            s_dead = 1;
            s_tail = s_head;
        }
    }
}

/*----------------------
 | log_ring_push
 | Description: Queues text, flushing first when it would not fit, and drops it once the port is dead.
 | Author: suinevere
 | Dependencies: log_ring_flush, log_ring_drain
 | Globals: s_port, s_ring, s_head, s_tail, s_dead
 | Params: text -- bytes to queue, len -- byte count
 | Returns: N/A
 ----------------------*/
void log_ring_push(const char *text, unsigned len)
{
    unsigned i;
    int old;

    if (s_port == NULL || s_dead || len >= LOG_RING_BYTES)
    {
        return;
    }
    if ((s_head - s_tail + LOG_RING_BYTES) % LOG_RING_BYTES + len >= LOG_RING_BYTES)
    {
        log_ring_flush();
        if (s_dead)
        {
            return;
        }
    }
    old = s_port->lock();
    for (i = 0; i < len; i++)
    {
        s_ring[s_head] = text[i];
        s_head = (s_head + 1u) % LOG_RING_BYTES;
    }
    s_port->unlock(old);
    log_ring_drain();
}

/*----------------------
 | log_ring_dead
 | Description: Whether a flush has given the port up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_dead
 | Params: N/A
 | Returns: 1 once dead, else 0
 ----------------------*/
int log_ring_dead(void)
{
    return s_dead;
}

/*----------------------
 | log_ring_reset
 | Description: Empties the ring, unbinds the port and clears the dead flag, for host tests.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_port, s_head, s_tail, s_dead
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void log_ring_reset(void)
{
    s_port = NULL;
    s_head = 0;
    s_tail = 0;
    s_dead = 0;
}
