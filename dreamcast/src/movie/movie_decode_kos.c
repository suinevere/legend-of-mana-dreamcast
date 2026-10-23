/*----------------------
 | movie_decode_kos.c
 | Description: A worker thread one priority above the game thread that runs libpress_dc.c's MDEC decodes, so a frame decodes while the game waits for vblank the way the PS1's MDEC does.
 | Author: suinevere
 | Dependencies: movie_decode_kos.h, libpress_dc.h, kos/thread.h, kos/sem.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stddef.h>

#include <kos/sem.h>
#include <kos/thread.h>

#include "libpress_dc.h"
#include "movie_decode_kos.h"

/*----------------------
 | WORKER_STACK_BYTES
 | Description: Stack for the decode worker; a macroblock decode keeps about 2 KB of locals.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define WORKER_STACK_BYTES (16 * 1024)

/*----------------------
 | s_ready / s_job
 | Description: Signalled once per job handed over, and the job the worker runs next.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static semaphore_t s_ready;
static void (*volatile s_job)(void);

/*----------------------
 | worker
 | Description: Waits for each job and runs it.
 | Author: suinevere
 | Dependencies: sem_wait
 | Globals: s_ready, s_job
 | Params: arg -- unused
 | Returns: never
 ----------------------*/
static void *worker(void *arg)
{
    (void)arg;
    for (;;)
    {
        sem_wait(&s_ready);
        if (s_job != NULL)
        {
            void (*job)(void) = s_job;

            s_job = NULL;
            job();
        }
    }
    return NULL;
}

/*----------------------
 | start
 | Description: libpress_dc.c's runner start hook: hands the job to the worker.
 | Author: suinevere
 | Dependencies: sem_signal
 | Globals: s_ready, s_job
 | Params: job -- the decode to run
 | Returns: N/A
 ----------------------*/
static void start(void (*job)(void))
{
    s_job = job;
    sem_signal(&s_ready);
}

/*----------------------
 | wait
 | Description: libpress_dc.c's runner wait hook: gives up the CPU so the worker can finish.
 | Author: suinevere
 | Dependencies: thd_pass
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void wait(void)
{
    thd_pass();
}

void movie_decode_kos_init(void)
{
    kthread_attr_t attr = {0};

    sem_init(&s_ready, 0);
    attr.create_detached = true;
    attr.stack_size = WORKER_STACK_BYTES;
    attr.prio = PRIO_DEFAULT - 1;
    attr.label = "mdec";
    if (thd_create_ex(&attr, worker, NULL) != NULL)
    {
        libpress_dc_set_runner(start, wait);
    }
}
