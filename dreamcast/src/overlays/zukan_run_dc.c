/*----------------------
 | zukan_run_dc.c
 | Description: Hand port of ZUKAN's entry zukan_run (PS1 0x80140E00, 0x294 bytes), which the decomp keeps only as raw MIPS in the zukan_run_code rodatabin; same calls in the same order.
 | Author: suinevere
 | Dependencies: overlays/zukan/internal/zukan.h, common/render_context.h, main/controller.h, overlays/field/field_text.h, libgpu.h, libetc.h
 | Globals: D_80157524, g_zukan_resource_buffer, g_zukan_exit_requested
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "common.h"
#include "common/render_context.h"
#include "main/controller.h"
#include "overlays/field/field_text.h"
#include "overlays/zukan/internal/zukan.h"
#include <libetc.h>
#include <libgpu.h>

/*----------------------
 | D_80157524 / g_zukan_resource_buffer / g_zukan_exit_requested
 | Description: ZUKAN's render-context pair, entry-resource buffer and exit flag, all in its packed data.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern RenderContext *D_80157524;
extern u8 *g_zukan_resource_buffer;
extern s32 g_zukan_exit_requested;

/*----------------------
 | field_update_input_repeat
 | Description: FIELD's per-frame input auto-repeat step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void field_update_input_repeat(void);

/*----------------------
 | field_reset_input_repeat
 | Description: FIELD's input auto-repeat reset, declared here because menu.h's <strings.h> clashes with the Psy-Q bcopy.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void field_reset_input_repeat(void);

/*----------------------
 | cdrom_process_state
 | Description: Main's per-frame CD state step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void cdrom_process_state(void);

/*----------------------
 | ZUKAN_STATE_OFFSET / ZUKAN_OT_LENGTH / ZUKAN_UI_RESOURCE_BLOCKS
 | Description: Where zukan_initialize_state's work starts after the two render contexts, OT entries cleared per frame, and the load-buffer size (in 64-sector blocks) from which the UI resource must be reloaded on exit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ZUKAN_STATE_OFFSET 0x8180
#define ZUKAN_OT_LENGTH 16
#define ZUKAN_UI_RESOURCE_BLOCKS 9

/*----------------------
 | zukan_run
 | Description: Sets up ZUKAN's two render contexts, runs its frame loop at two vblanks until the exit flag is set, then restores FIELD's input repeat and text windows.
 | Author: suinevere
 | Dependencies: zukan_initialize_state, zukan_update_frame, zukan_load_ui_resource, libgpu, controller, field_text
 | Globals: D_80157524, g_zukan_resource_buffer, g_zukan_exit_requested
 | Params: work -- buffer holding both render contexts then ZUKAN's state, category -- starting category
 | Returns: N/A
 ----------------------*/
void zukan_run(u8 *work, s32 category)
{
    RenderContext *ctx = (RenderContext *)work;
    RenderContext *current;
    RenderContext *next;
    s32 sectors;

    D_80157524 = ctx;
    setRECT(&ctx[0].clear_rect, 0, 8, 320, 224);
    setRECT(&ctx[1].clear_rect, 0, 240, 320, 224);
    SetDefDispEnv(&ctx[0].disp_env, 0, 0, 320, 240);
    SetDefDispEnv(&D_80157524[1].disp_env, 0, 232, 320, 240);
    SetDefDrawEnv(&D_80157524[0].draw_env, 0, 240, 320, 224);
    SetDefDrawEnv(&D_80157524[1].draw_env, 0, 8, 320, 224);
    D_80157524[1].draw_env.dtd = 0;
    D_80157524[0].draw_env.dtd = 0;
    g_zukan_exit_requested = 0;
    D_80157524[0].frame_parity = 0;
    D_80157524[1].frame_parity = 1;
    g_zukan_resource_buffer = (u8 *)(((uintptr_t)zukan_initialize_state(work + ZUKAN_STATE_OFFSET, category) + 3) & ~(uintptr_t)3);
    current = D_80157524;
    ClearOTagR(current->ot, ZUKAN_OT_LENGTH);
    ClearOTagR(D_80157524[1].ot, ZUKAN_OT_LENGTH);
    VSync(0);
    PutDispEnv(&current->disp_env);
    update_controllers();
    for (;;)
    {
        ClearOTagR(current->ot, ZUKAN_OT_LENGTH);
        current->prim_cursor = &current->ot[ZUKAN_OT_LENGTH];
        field_text_reset_scratch();
        field_update_input_repeat();
        zukan_update_frame(current);
        field_text_upload_immediate_cache();
        DrawSync(0);
        set_controller_vsync_interval(2);
        VSync(2);
        if (g_zukan_exit_requested != 0)
        {
            break;
        }
        ClearImage(&current->clear_rect, 0, 0, 0);
        next = (current == D_80157524) ? &current[1] : D_80157524;
        PutDispEnv(&next->disp_env);
        PutDrawEnv(&next->draw_env);
        DrawOTag(&current->ot[ZUKAN_OT_LENGTH - 1]);
        current = next;
        update_controllers();
        cdrom_process_state();
    }
    sectors = (s32)(*(u32 *)LOAD_BUFFER_AT(0xC) >> 9);
    if ((sectors + 63) / 64 >= ZUKAN_UI_RESOURCE_BLOCKS)
    {
        zukan_load_ui_resource();
    }
    field_reset_input_repeat();
    field_text_reset_windows();
}
