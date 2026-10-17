/*----------------------
 | card_none.c
 | Description: Memory-card BIOS, libcard and libmcx calls answered as a PS1 with both slots empty, so card screens take their no-card paths until D5's VMU backend replaces this file.
 | Author: suinevere
 | Dependencies: libapi.h, libmcx.h
 | Globals: s_event_specs, s_event_count, s_mcx_pending
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <libapi.h>
#include <libmcx.h>

/*----------------------
 | CARD_EVENT_MAX / CARD_EVENT_HANDLE_BASE
 | Description: Most events OpenEvent records, and the bit pattern of the handles it returns, like the BIOS's 0xF1000000-based descriptors.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CARD_EVENT_MAX 32
#define CARD_EVENT_HANDLE_BASE 0xF1000000L

/*----------------------
 | s_event_specs / s_event_count
 | Description: Spec of each event OpenEvent handed out, indexed by handle minus CARD_EVENT_HANDLE_BASE.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static long s_event_specs[CARD_EVENT_MAX];
static int s_event_count;

/*----------------------
 | s_mcx_pending
 | Description: Whether an Mcx command was issued and its McxSync result not yet collected.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_mcx_pending;

/*----------------------
 | OpenEvent
 | Description: Records the event's spec and returns a new handle; a full table returns the last handle again.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_event_specs, s_event_count
 | Params: desc -- event class, spec -- event kind, mode -- delivery mode, func -- handler
 | Returns: event handle
 ----------------------*/
long OpenEvent(unsigned long desc, long spec, long mode, long (*func)())
{
    (void)desc;
    (void)mode;
    (void)func;
    if (s_event_count < CARD_EVENT_MAX)
    {
        s_event_specs[s_event_count++] = spec;
    }
    return CARD_EVENT_HANDLE_BASE | (s_event_count - 1);
}

/*----------------------
 | CloseEvent / EnableEvent
 | Description: Accept any handle; events need no state beyond their spec.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: event -- event handle
 | Returns: 1
 ----------------------*/
long CloseEvent(long event)
{
    (void)event;
    return 1;
}

long EnableEvent(long event)
{
    (void)event;
    return 1;
}

/*----------------------
 | TestEvent
 | Description: Reports a timeout event as fired and every other event as not, which is how an empty slot answers every card request.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_event_specs, s_event_count
 | Params: event -- event handle
 | Returns: 1 for a timeout event, else 0
 ----------------------*/
long TestEvent(long event)
{
    long index = event & ~CARD_EVENT_HANDLE_BASE;

    if (index < 0 || index >= s_event_count)
    {
        return 0;
    }
    return s_event_specs[index] == EvSpTIMOUT;
}

/*----------------------
 | EnterCriticalSection / ExitCriticalSection
 | Description: Nothing to mask, since no card interrupt ever arrives.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: EnterCriticalSection returns 1
 ----------------------*/
int EnterCriticalSection(void)
{
    return 1;
}

void ExitCriticalSection(void)
{
}

/*----------------------
 | _card_info / _card_load / _card_clear / _card_format
 | Description: Accept the request; it completes with a timeout event.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: chan -- slot channel
 | Returns: 1 (issued)
 ----------------------*/
long _card_info(long chan)
{
    (void)chan;
    return 1;
}

long _card_load(long chan)
{
    (void)chan;
    return 1;
}

long _card_clear(long chan)
{
    (void)chan;
    return 1;
}

long _card_format(long chan)
{
    (void)chan;
    return 1;
}

/*----------------------
 | _card_read / _card_write
 | Description: Accept the block request; it completes with a timeout event and moves no bytes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: chan -- slot channel, block -- block number, buf -- data buffer
 | Returns: 1 (issued)
 ----------------------*/
long _card_read(long chan, long block, unsigned char *buf)
{
    (void)chan;
    (void)block;
    (void)buf;
    return 1;
}

long _card_write(long chan, long block, unsigned char *buf)
{
    (void)chan;
    (void)block;
    (void)buf;
    return 1;
}

/*----------------------
 | _new_card
 | Description: Clears the new-card flag the BIOS would set; there is never a card to flag.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void _new_card(void)
{
}

/*----------------------
 | firstfile / nextfile
 | Description: Directory search on an empty slot finds no files.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- search pattern, entry -- result record
 | Returns: NULL
 ----------------------*/
struct DIRENTRY *firstfile(char *path, struct DIRENTRY *entry)
{
    (void)path;
    (void)entry;
    return NULL;
}

struct DIRENTRY *nextfile(struct DIRENTRY *entry)
{
    (void)entry;
    return NULL;
}

/*----------------------
 | McxCardType / McxGetTime
 | Description: Accept the command; its McxSync result is McxErrNoCard.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_mcx_pending
 | Params: port -- controller port, time -- clock bytes (left untouched)
 | Returns: MCX_COMMAND_ISSUED
 ----------------------*/
int McxCardType(int port)
{
    (void)port;
    s_mcx_pending = 1;
    return MCX_COMMAND_ISSUED;
}

int McxGetTime(int port, unsigned char *time)
{
    (void)port;
    (void)time;
    s_mcx_pending = 1;
    return MCX_COMMAND_ISSUED;
}

/*----------------------
 | McxSync
 | Description: Finishes the pending command with McxErrNoCard, or reports none pending.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_mcx_pending
 | Params: mode -- wait mode (ignored), cmd -- command out, result -- result out
 | Returns: McxSyncFin when a command finished, else McxSyncNone
 ----------------------*/
int McxSync(int mode, long *cmd, long *result)
{
    (void)mode;
    if (!s_mcx_pending)
    {
        return McxSyncNone;
    }
    s_mcx_pending = 0;
    *cmd = 0;
    *result = McxErrNoCard;
    return McxSyncFin;
}

/*----------------------
 | _card_wait
 | Description: Waits for a card request that has already timed out.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: drive -- slot
 | Returns: 1
 ----------------------*/
long _card_wait(long drive)
{
    (void)drive;
    return 1;
}

/*----------------------
 | erase
 | Description: Deleting a file on an empty slot fails.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- card file path
 | Returns: 0
 ----------------------*/
long erase(char *path)
{
    (void)path;
    return 0;
}

/*----------------------
 | func_80033E7C / func_80034648
 | Description: libmcx internals CARDA calls on its PocketStation save path; with no card both fail.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: chan -- slot channel, block -- first block, count -- block count
 | Returns: 0
 ----------------------*/
int func_80033E7C(long chan)
{
    (void)chan;
    return 0;
}

int func_80034648(long chan, long block, long count)
{
    (void)chan;
    (void)block;
    (void)count;
    return 0;
}

/*----------------------
 | func_8001680C / func_8001681C / func_8001682C / func_8001683C / func_8001685C
 | Description: BIOS file open, read, write, close and rename as main's trampolines expose them to NIKI; on an empty slot opens, reads, writes and renames fail.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path/from/to -- card file paths, mode -- open flags, fd -- file handle, buf -- data, n -- byte count
 | Returns: open, read and write -1; close and rename 0
 ----------------------*/
long func_8001680C(char *path, long mode)
{
    (void)path;
    (void)mode;
    return -1;
}

long func_8001681C(long fd, void *buf, long n)
{
    (void)fd;
    (void)buf;
    (void)n;
    return -1;
}

long func_8001682C(long fd, void *buf, long n)
{
    (void)fd;
    (void)buf;
    (void)n;
    return -1;
}

long func_8001683C(long fd)
{
    (void)fd;
    return 0;
}

long func_8001685C(char *from, char *to)
{
    (void)from;
    (void)to;
    return 0;
}

/*----------------------
 | func_80016F9C / func_800170BC
 | Description: BIOS strcat and strcpy, which NIKI uses to build card file paths.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: dst -- destination string, src -- source string
 | Returns: dst
 ----------------------*/
char *func_80016F9C(char *dst, const char *src)
{
    char *end = dst;

    while (*end != 0)
    {
        end++;
    }
    while ((*end++ = *src++) != 0)
    {
    }
    return dst;
}

char *func_800170BC(char *dst, const char *src)
{
    char *out = dst;

    while ((*out++ = *src++) != 0)
    {
    }
    return dst;
}
