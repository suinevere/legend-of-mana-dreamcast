/*----------------------
 | vmu_record.h
 | Description: The recording layer behind gen_record.py's wrappers: call frames with path-key bits, the vmu_* calls made inside a call, and sampled records written to DIR/NAME.rec.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: rec_top
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef VMU_RECORD_H
#define VMU_RECORD_H

#include <stdint.h>

#include "vmu_hw.h"

/*----------------------
 | REC_MAX_BITS / REC_MAX_ARGS / REC_ARG_BYTES
 | Description: Limits of one call frame: path-key bits (two per probe), arguments, and bytes one argument or its pointee may take.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define REC_MAX_BITS 512
#define REC_MAX_ARGS 8
#define REC_ARG_BYTES 16

/*----------------------
 | RecFn
 | Description: The generic function pointer the wrapper registry holds; GCC lets any function type cast to it without a warning.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef void (*RecFn)(void);

/*----------------------
 | RecArg
 | Description: One recorded argument: kind 0 holds its bytes, kind 1 (a pointer) the pointee's bytes before the call, after it and the pointer, kind 2 a null pointer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    uint8_t kind, size;
    uint8_t before[REC_ARG_BYTES], after[REC_ARG_BYTES];
    const void *ptr;
} RecArg;

/*----------------------
 | RecFrame
 | Description: One wrapped call in progress, on the caller's stack: function id, path-key bits, arguments, the RAM and frame buffer at entry, where its vmu_* calls start in the log, its depth, frame and parent.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct RecFrame {
    uint16_t fid, nbits;
    uint8_t bits[REC_MAX_BITS / 8];
    uint8_t nargs, depth;
    RecArg args[REC_MAX_ARGS];
    uint8_t ram[0x400];
    uint8_t fb[VMU_FB_BYTES];
    uint32_t hw_start, frame;
    struct RecFrame *parent;
} RecFrame;

/*----------------------
 | rec_top
 | Description: The innermost wrapped call in progress, whose bits the probes in the running __impl set.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern RecFrame *rec_top;

/*----------------------
 | rec_bit
 | Description: Sets one path-key bit of the running call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: rec_top
 | Params: k -- bit index
 | Returns: N/A
 ----------------------*/
static inline void rec_bit(unsigned k) { rec_top->bits[k >> 3] |= (uint8_t)(1u << (k & 7u)); }

/*----------------------
 | rec_cond
 | Description: The probe around a condition: sets bit 2k when it holds and 2k+1 when not, and passes its truth through.
 | Author: suinevere
 | Dependencies: rec_bit
 | Globals: rec_top
 | Params: k -- probe index; t -- the condition's truth
 | Returns: t
 ----------------------*/
static inline int rec_cond(unsigned k, int t) {
    rec_bit(2u * k + (t ? 0u : 1u));
    return t;
}

/*----------------------
 | rec_enter
 | Description: Opens a call frame: pushes it, clears its bits and snapshots vmu_ram and the frame buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: rec_top
 | Params: f -- the frame; fid -- function id; nbits -- path-key bits the function's probes use
 | Returns: N/A
 ----------------------*/
void rec_enter(RecFrame *f, uint16_t fid, uint16_t nbits);

/*----------------------
 | rec_arg
 | Description: Records an argument's value.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: f -- the frame; p -- the argument; size -- its size
 | Returns: N/A
 ----------------------*/
void rec_arg(RecFrame *f, const void *p, uint8_t size);

/*----------------------
 | rec_arg_ptr
 | Description: Records a pointer argument by its pointee, read now and again when the call returns.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: f -- the frame; p -- the pointer; size -- the pointee's size
 | Returns: N/A
 ----------------------*/
void rec_arg_ptr(RecFrame *f, const void *p, uint8_t size);

/*----------------------
 | rec_leave
 | Description: Closes the call frame: pops it and, when its path key is new or the sample picks it, keeps the call as a record.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: rec_top
 | Params: f -- the frame; result -- the return value or null; size -- its size or 0
 | Returns: N/A
 ----------------------*/
void rec_leave(RecFrame *f, const void *result, uint8_t size);

/*----------------------
 | rec_register
 | Description: Enters a wrapper's address in the registry the handler argument of vmu_set_handler is named from.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: fid -- function id; fn -- the wrapper
 | Returns: N/A
 ----------------------*/
void rec_register(uint16_t fid, RecFn fn);

/*----------------------
 | rec_lookup
 | Description: A wrapper by function name, for record_synthetic to call functions no script reaches.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: name -- the C function name
 | Returns: the wrapper, or null
 ----------------------*/
RecFn rec_lookup(const char *name);

/*----------------------
 | rec_begin
 | Description: Starts recording into dir for one script: records carry the script name and, if synthetic, the synthetic flag; the records are written at rec_finish or at exit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: dir -- output directory; script -- script name; synthetic -- nonzero for record_synthetic
 | Returns: N/A
 ----------------------*/
void rec_begin(const char *dir, const char *script, int synthetic);

/*----------------------
 | rec_pause
 | Description: Stops or resumes keeping and counting calls, for record_synthetic's setup.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: on -- nonzero to pause
 | Returns: N/A
 ----------------------*/
void rec_pause(int on);

/*----------------------
 | rec_set_frame
 | Description: The frame number the next calls carry.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: frame -- frame number (vmu_runner's sample count)
 | Returns: N/A
 ----------------------*/
void rec_set_frame(uint32_t frame);

/*----------------------
 | rec_reset
 | Description: Drops every call in progress, for a boot the runner longjmps back to; those calls are never recorded.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: rec_top
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void rec_reset(void);

/*----------------------
 | rec_finish
 | Description: Appends the kept records to DIR/NAME.rec, once.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void rec_finish(void);

/*----------------------
 | rec_fn_names / rec_fn_bits / REC_FN_COUNT
 | Description: The generated table of game functions by id (sorted names) and the path-key bits each one's probes use.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern const char *const rec_fn_names[];
extern const uint16_t rec_fn_bits[];
extern const uint16_t rec_fn_count;

/*----------------------
 | rec_vmu_*
 | Description: The vmu_hw.h calls the generated game files make instead of the real ones: each logs its id, arguments and return, then calls through. vmu_r8, vmu_w8, vmu_fb and vmu_rom8 are memory and stay unlogged.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: N/A
 | Params: as vmu_hw.h
 | Returns: as vmu_hw.h
 ----------------------*/
uint8_t rec_vmu_buttons(void);
void rec_vmu_present(void);
void rec_vmu_idle(void);
void rec_vmu_irq_enable(uint16_t lines);
void rec_vmu_irq_disable(uint16_t lines);
void rec_vmu_irq_ack(uint16_t lines);
uint16_t rec_vmu_irq_pending(void);
uint16_t rec_vmu_irq_levels(void);
uint16_t rec_vmu_irq_mask(void);
void rec_vmu_set_handler(void (*irq)(void));
void rec_vmu_tick_mode(uint8_t mode);
void rec_vmu_sound_play(uint32_t samples, uint16_t counts_per_sample);
void rec_vmu_sound_stop(void);
uint8_t rec_vmu_sys_flags(void);
void rec_vmu_power(uint8_t mode);
uint8_t rec_vmu_save_read(uint16_t sector, uint16_t dst);
uint8_t rec_vmu_save_write(uint16_t sector, uint16_t src);
void rec_vmu_buzz(uint8_t tone);
void rec_vmu_lcd(uint8_t on);

#endif
