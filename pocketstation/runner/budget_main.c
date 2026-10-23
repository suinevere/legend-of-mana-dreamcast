/*----------------------
 | budget_main.c
 | Description: Bare-metal driver for the instruction budget: runs the VMU-shaped game on vmu_host.c from inputs budget.py places in memory, marking each frame and the end where budget.py's hooks see them.
 | Author: suinevere
 | Dependencies: game.h, vmu_host.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <setjmp.h>
#include <string.h>

#include "game.h"
#include "vmu_host.h"

/*----------------------
 | BUDGET_IN
 | Description: Where budget.py places the inputs: a BudgetIn header, the file, the steps and the boots, at the header's offsets.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BUDGET_IN 0x30000000u

/*----------------------
 | STEP_REBOOT
 | Description: A step byte that reboots; any other step is a sample with that button mask.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STEP_REBOOT 0x80u

/*----------------------
 | BudgetIn
 | Description: The input header: the file's length and offset, the step count and offset, the boot count and offset.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint32_t file_len, file_off;
    uint32_t steps, steps_off;
    uint32_t boots, boots_off;
} BudgetIn;

/*----------------------
 | BudgetBoot
 | Description: One app entry: the device word count, the system flags and the device words in psk_host_load_entry order.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint32_t n, sys;
    uint32_t words[64];
} BudgetBoot;

/*----------------------
 | g_*
 | Description: The inputs, the next step, the game's file across reboots and the escape back to main.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const BudgetIn *const g_in = (const BudgetIn *)BUDGET_IN;
static uint32_t g_next;
static uint8_t g_file[0x10000];
static jmp_buf g_top;

/*----------------------
 | budget_frame_mark / budget_done
 | Description: Empty marks budget.py hooks: the end of a frame, and the end of the run.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A (budget_done does not return)
 ----------------------*/
__attribute__((noinline)) void budget_frame_mark(void) { __asm__ volatile(""); }

__attribute__((noinline, noreturn)) void budget_done(void) {
    for (;;) {
        __asm__ volatile("");
    }
}

/*----------------------
 | step_at
 | Description: Step i of the script.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_in
 | Params: i -- step index
 | Returns: the step byte
 ----------------------*/
static uint8_t step_at(uint32_t i) { return ((const uint8_t *)BUDGET_IN)[g_in->steps_off + i]; }

/*----------------------
 | advance
 | Description: Leaves through g_top on a reboot or the end, else sets the next sample's buttons.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_next, g_top
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void advance(void) {
    if (g_next < g_in->steps && step_at(g_next) == STEP_REBOOT) {
        g_next++;
        longjmp(g_top, 2);
    }
    if (g_next >= g_in->steps) longjmp(g_top, 1);
    vmu_host_set_buttons(step_at(g_next));
}

/*----------------------
 | frame
 | Description: The frame point: marks the frame, then moves to the next step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_next
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void frame(void) {
    budget_frame_mark();
    g_next++;
    advance();
}

/*----------------------
 | boot
 | Description: Starts boot k from its entry state, keeping the game's file, and runs the game until a step leaves.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_in, g_file
 | Params: k -- boot index
 | Returns: does not return
 ----------------------*/
static void boot(uint32_t k) {
    const BudgetBoot *b = (const BudgetBoot *)(BUDGET_IN + g_in->boots_off) + (k < g_in->boots ? k : g_in->boots - 1u);
    vmu_host_init(g_file, g_in->file_len);
    vmu_host_set_entry(b->words, b->n);
    vmu_host_set_sys((uint8_t)b->sys);
    vmu_host_set_frame(frame);
    advance();
    game_entry();
    budget_done();
}

/*----------------------
 | main
 | Description: Copies the file in and runs every boot of the script, then stops at budget_done.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_in, g_file, g_top
 | Params: N/A
 | Returns: does not return
 ----------------------*/
int main(void) {
    volatile uint32_t k = 0;
    memcpy(g_file, (const uint8_t *)BUDGET_IN + g_in->file_off, g_in->file_len);
    int how = setjmp(g_top);
    if (how == 2) {
        k++;
        memcpy(g_file, vmu_host_file(), g_in->file_len);
    }
    if (how != 1) boot(k);
    budget_done();
}

/*----------------------
 | _init / _fini
 | Description: The empty constructor and destructor hooks newlib's start and exit paths call; the budget build has neither.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void _init(void) {}

void _fini(void) {}

/*----------------------
 | _start
 | Description: Entry: budget.py sets the stack and has loaded every segment, BSS zeroed, so it only calls main.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: does not return
 ----------------------*/
void _start(void) {
    main();
    budget_done();
}
