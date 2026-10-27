/*----------------------
 | gte_soft.h
 | Description: C register-file model of the PS1 GTE that replaces the cop2 inline-asm macros of inline_c.h and gte_dmpsx_compat.h on the Dreamcast.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: lom_gte, lom_libgte_tables
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef GTE_SOFT_H
#define GTE_SOFT_H

#include <stdint.h>

#define SDK_GTE_DMPSX_BRIDGE_H

/*----------------------
 | GteRegisters
 | Description: The GTE data and control registers, kept at their hardware widths with packed pairs split into native fields.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct GteRegisters
{
    int16_t v[3][3];
    uint8_t rgbc[4];
    uint16_t otz;
    int16_t ir[4];
    int16_t sxy[3][2];
    uint16_t sz[4];
    uint8_t rgb[3][4];
    int32_t mac[4];
    int32_t lzcs;
    int32_t lzcr;
    int16_t rt[3][3];
    int32_t tr[3];
    int16_t llm[3][3];
    int32_t bk[3];
    int16_t lcm[3][3];
    int32_t fc[3];
    int32_t ofx;
    int32_t ofy;
    uint16_t h;
    int16_t dqa;
    int32_t dqb;
    int16_t zsf3;
    int16_t zsf4;
    uint32_t flag;
} GteRegisters;

/*----------------------
 | lom_gte
 | Description: The single emulated GTE register file every gte_* macro reads and writes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern GteRegisters lom_gte;

/*----------------------
 | lom_libgte_tables
 | Description: The libgte table blob from gen/libgte_tables.bin, little-endian as in SLUS_010.13, laid out as gen/libgte_tables.h describes.
 | Author: suinevere
 | Dependencies: libgte_tables.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern const uint8_t lom_libgte_tables[];

/*----------------------
 | GTE_MX_RT
 | Description: MVMVA matrix selector for the rotation matrix.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_MX_RT 0

/*----------------------
 | GTE_MX_LLM
 | Description: MVMVA matrix selector for the light matrix.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_MX_LLM 1

/*----------------------
 | GTE_MX_LCM
 | Description: MVMVA matrix selector for the light colour matrix.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_MX_LCM 2

/*----------------------
 | GTE_CV_TR
 | Description: MVMVA translation selector for TR.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_CV_TR 0

/*----------------------
 | GTE_CV_BK
 | Description: MVMVA translation selector for the background colour.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_CV_BK 1

/*----------------------
 | GTE_CV_NONE
 | Description: MVMVA translation selector for no translation.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_CV_NONE 3

/*----------------------
 | GTE_V_IR
 | Description: MVMVA vector selector for IR1-IR3.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_V_IR 3

void gte_soft_reset(void);
int gte_soft_unr_entry(int index);
void gte_soft_ldv(int slot, const void *vector);
void gte_soft_ldlvl(const void *vector);
void gte_soft_ldsxy3(uint32_t sxy0, uint32_t sxy1, uint32_t sxy2);
void gte_soft_set_matrix(int mx, const void *matrix);
void gte_soft_set_trans(const void *matrix);
void gte_soft_set_back_color(int32_t r, int32_t g, int32_t b);
void gte_soft_rtps(void);
void gte_soft_rtpt(void);
void gte_soft_mvmva(int sf, int mx, int v, int cv, int lm);
void gte_soft_nclip(void);
void gte_soft_ncs(void);
void gte_soft_sqr(int sf);
void gte_soft_gpf(int sf, int lm);
void gte_soft_stsxy(int index, void *out);
void gte_soft_stflg(void *out);
void gte_soft_stszotz(void *out);
void gte_soft_stopz(void *out);
void gte_soft_stlvnl(void *out);
void gte_soft_stsv(void *out);
void gte_soft_strgb(void *out);

/*----------------------
 | gte_ldv0 / gte_ldv1 / gte_ldv2 / gte_ldv3
 | Description: Load V0, V1, V2 (or all three) from SVECTOR-layout memory.
 | Author: suinevere
 | Dependencies: gte_soft_ldv
 | Globals: lom_gte
 | Params: r0, r1, r2 -- SVECTOR addresses
 | Returns: N/A
 ----------------------*/
#define gte_ldv0(r0) gte_soft_ldv(0, (const void *)(r0))
#define gte_ldv1(r0) gte_soft_ldv(1, (const void *)(r0))
#define gte_ldv2(r0) gte_soft_ldv(2, (const void *)(r0))
#define gte_ldv3(r0, r1, r2) \
    (gte_soft_ldv(0, (const void *)(r0)), gte_soft_ldv(1, (const void *)(r1)), gte_soft_ldv(2, (const void *)(r2)))

/*----------------------
 | gte_ldlvl / gte_ldsxy3
 | Description: Load IR1-IR3 from three 32-bit words, or SXY0-SXY2 from three packed screen words in memory order.
 | Author: suinevere
 | Dependencies: gte_soft_ldlvl, gte_soft_ldsxy3
 | Globals: lom_gte
 | Params: r0 -- VECTOR address, or r0..r2 -- packed SXY words
 | Returns: N/A
 ----------------------*/
#define gte_ldlvl(r0) gte_soft_ldlvl((const void *)(r0))
#define gte_ldsxy3(r0, r1, r2) gte_soft_ldsxy3((uint32_t)(r0), (uint32_t)(r1), (uint32_t)(r2))

/*----------------------
 | gte_SetRotMatrix / gte_SetLightMatrix / gte_SetColorMatrix / gte_SetTransMatrix / gte_SetBackColor
 | Description: Load the rotation, light or colour matrix, the translation vector, or the background colour (scaled by 16).
 | Author: suinevere
 | Dependencies: gte_soft_set_matrix, gte_soft_set_trans, gte_soft_set_back_color
 | Globals: lom_gte
 | Params: r0 -- MATRIX address, or r0..r2 -- colour components
 | Returns: N/A
 ----------------------*/
#define gte_SetRotMatrix(r0) gte_soft_set_matrix(GTE_MX_RT, (const void *)(r0))
#define gte_SetLightMatrix(r0) gte_soft_set_matrix(GTE_MX_LLM, (const void *)(r0))
#define gte_SetColorMatrix(r0) gte_soft_set_matrix(GTE_MX_LCM, (const void *)(r0))
#define gte_SetTransMatrix(r0) gte_soft_set_trans((const void *)(r0))
#define gte_SetBackColor(r0, r1, r2) gte_soft_set_back_color((int32_t)(r0), (int32_t)(r1), (int32_t)(r2))

/*----------------------
 | gte_rtps / gte_rtpt
 | Description: Perspective-transform V0, or V0-V2, through RT/TR and the projection into the SXY and SZ FIFOs (sf=1, lm=0).
 | Author: suinevere
 | Dependencies: gte_soft_rtps, gte_soft_rtpt
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define gte_rtps() gte_soft_rtps()
#define gte_rtpt() gte_soft_rtpt()

/*----------------------
 | gte_rt* / gte_ll* / gte_lc*
 | Description: MVMVA forms that multiply V0-V2 or IR by RT, LLM or LCM and add TR, BK or nothing.
 | Author: suinevere
 | Dependencies: gte_soft_mvmva
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define gte_rtv0() gte_soft_mvmva(1, GTE_MX_RT, 0, GTE_CV_NONE, 0)
#define gte_rtv1() gte_soft_mvmva(1, GTE_MX_RT, 1, GTE_CV_NONE, 0)
#define gte_rtv2() gte_soft_mvmva(1, GTE_MX_RT, 2, GTE_CV_NONE, 0)
#define gte_rtir() gte_soft_mvmva(1, GTE_MX_RT, GTE_V_IR, GTE_CV_NONE, 0)
#define gte_rtir_sf0() gte_soft_mvmva(0, GTE_MX_RT, GTE_V_IR, GTE_CV_NONE, 0)
#define gte_rtv0tr() gte_soft_mvmva(1, GTE_MX_RT, 0, GTE_CV_TR, 0)
#define gte_rtv1tr() gte_soft_mvmva(1, GTE_MX_RT, 1, GTE_CV_TR, 0)
#define gte_rtv2tr() gte_soft_mvmva(1, GTE_MX_RT, 2, GTE_CV_TR, 0)
#define gte_rtirtr() gte_soft_mvmva(1, GTE_MX_RT, GTE_V_IR, GTE_CV_TR, 0)
#define gte_rtv0bk() gte_soft_mvmva(1, GTE_MX_RT, 0, GTE_CV_BK, 0)
#define gte_rtv1bk() gte_soft_mvmva(1, GTE_MX_RT, 1, GTE_CV_BK, 0)
#define gte_rtv2bk() gte_soft_mvmva(1, GTE_MX_RT, 2, GTE_CV_BK, 0)
#define gte_rtirbk() gte_soft_mvmva(1, GTE_MX_RT, GTE_V_IR, GTE_CV_BK, 0)
#define gte_llv0() gte_soft_mvmva(1, GTE_MX_LLM, 0, GTE_CV_NONE, 0)
#define gte_llv1() gte_soft_mvmva(1, GTE_MX_LLM, 1, GTE_CV_NONE, 0)
#define gte_llv2() gte_soft_mvmva(1, GTE_MX_LLM, 2, GTE_CV_NONE, 0)
#define gte_llir() gte_soft_mvmva(1, GTE_MX_LLM, GTE_V_IR, GTE_CV_NONE, 0)
#define gte_lcv0() gte_soft_mvmva(1, GTE_MX_LCM, 0, GTE_CV_NONE, 0)
#define gte_lcv1() gte_soft_mvmva(1, GTE_MX_LCM, 1, GTE_CV_NONE, 0)
#define gte_lcv2() gte_soft_mvmva(1, GTE_MX_LCM, 2, GTE_CV_NONE, 0)
#define gte_lcir() gte_soft_mvmva(1, GTE_MX_LCM, GTE_V_IR, GTE_CV_NONE, 0)

/*----------------------
 | gte_nclip / gte_ncs / gte_sqr0 / gte_sqr12
 | Description: Normal clip of SXY0-SXY2 into MAC0, single-normal colour, and IR squaring without or with the 12-bit shift.
 | Author: suinevere
 | Dependencies: gte_soft_nclip, gte_soft_ncs, gte_soft_sqr
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define gte_nclip() gte_soft_nclip()
#define gte_ncs() gte_soft_ncs()
#define gte_sqr0() gte_soft_sqr(0)
#define gte_sqr12() gte_soft_sqr(1)

/*----------------------
 | gte_stsxy / gte_stsxy0 / gte_stsxy1 / gte_stsxy2 / gte_stsxy01 / gte_stsxy3
 | Description: Store screen XY FIFO entries as two native shorts (x then y) per entry.
 | Author: suinevere
 | Dependencies: gte_soft_stsxy
 | Globals: lom_gte
 | Params: r0..r2 -- destination addresses
 | Returns: N/A
 ----------------------*/
#define gte_stsxy(r0) gte_soft_stsxy(2, (void *)(r0))
#define gte_stsxy0(r0) gte_soft_stsxy(0, (void *)(r0))
#define gte_stsxy1(r0) gte_soft_stsxy(1, (void *)(r0))
#define gte_stsxy2(r0) gte_soft_stsxy(2, (void *)(r0))
#define gte_stsxy01(r0, r1) (gte_soft_stsxy(0, (void *)(r0)), gte_soft_stsxy(1, (void *)(r1)))
#define gte_stsxy3(r0, r1, r2) \
    (gte_soft_stsxy(0, (void *)(r0)), gte_soft_stsxy(1, (void *)(r1)), gte_soft_stsxy(2, (void *)(r2)))

/*----------------------
 | gte_stflg / gte_stszotz / gte_stopz / gte_stlvnl / gte_stsv / gte_strgb
 | Description: Store FLAG, SZ3/4, MAC0, MAC1-MAC3, IR1-IR3 as shorts, or the newest colour FIFO entry as r,g,b,code bytes.
 | Author: suinevere
 | Dependencies: gte_soft_stflg, gte_soft_stszotz, gte_soft_stopz, gte_soft_stlvnl, gte_soft_stsv, gte_soft_strgb
 | Globals: lom_gte
 | Params: r0 -- destination address
 | Returns: N/A
 ----------------------*/
#define gte_stflg(r0) gte_soft_stflg((void *)(r0))
#define gte_stszotz(r0) gte_soft_stszotz((void *)(r0))
#define gte_stopz(r0) gte_soft_stopz((void *)(r0))
#define gte_stlvnl(r0) gte_soft_stlvnl((void *)(r0))
#define gte_stsv(r0) gte_soft_stsv((void *)(r0))
#define gte_strgb(r0) gte_soft_strgb((void *)(r0))

#endif
