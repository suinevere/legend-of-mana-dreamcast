/*----------------------
 | log_ring.h
 | Description: The queued serial log: a byte ring fed to a transmit FIFO without waiting, whose flush gives the port up for good when it stops draining.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOG_RING_H
#define LOG_RING_H

#include <stdint.h>

/*----------------------
 | LOG_RING_STALL_US
 | Description: How long a flush waits on a FIFO that takes no byte before declaring the port dead, as KallistiOS's scif_write does for a missing cable.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOG_RING_STALL_US 20000u

/*----------------------
 | LogPort
 | Description: The transmit FIFO's free slots and byte write, a microsecond clock, and the interrupt lock guarding the ring.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    unsigned (*tx_room)(void);
    void (*tx_put)(uint8_t c);
    unsigned long long (*now_us)(void);
    int (*lock)(void);
    void (*unlock)(int old);
} LogPort;

/*----------------------
 | log_ring_bind
 | Description: Sets the port the ring feeds; until then every call does nothing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_port
 | Params: port -- the transmit FIFO and helpers
 | Returns: N/A
 ----------------------*/
void log_ring_bind(const LogPort *port);

/*----------------------
 | log_ring_push
 | Description: Queues text, flushing first when it would not fit, and drops it once the port is dead.
 | Author: suinevere
 | Dependencies: log_ring_flush, log_ring_drain
 | Globals: s_port, s_ring, s_head, s_tail, s_dead
 | Params: text -- bytes to queue, len -- byte count
 | Returns: N/A
 ----------------------*/
void log_ring_push(const char *text, unsigned len);

/*----------------------
 | log_ring_drain
 | Description: Moves queued bytes into the FIFO while it has room, never waiting.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_port, s_ring, s_head, s_tail, s_dead
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void log_ring_drain(void);

/*----------------------
 | log_ring_flush
 | Description: Drains every queued byte, but discards the rest and marks the port dead after LOG_RING_STALL_US without progress.
 | Author: suinevere
 | Dependencies: log_ring_drain
 | Globals: s_port, s_head, s_tail, s_dead
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void log_ring_flush(void);

/*----------------------
 | log_ring_dead
 | Description: Whether a flush has given the port up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_dead
 | Params: N/A
 | Returns: 1 once dead, else 0
 ----------------------*/
int log_ring_dead(void);

/*----------------------
 | log_ring_reset
 | Description: Empties the ring, unbinds the port and clears the dead flag, for host tests.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_port, s_head, s_tail, s_dead
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void log_ring_reset(void);

#endif
