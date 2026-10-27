/*----------------------
 | test_gte.c
 | Description: Host tests for the Saturn GTE model; every expected value below is worked by hand from the psx-spx GTE formulas.
 | Author: suinevere
 | Dependencies: gte_soft.h, gte_soft.c, libgte.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 |
 | Derivations (H=1000, OFX=160<<16, OFY=120<<16 unless stated; n is the UNR quotient H*0x10000/SZ3):
 | UNR_0 = max(0,(0x40000/0x100+1)/2-0x101) = 0x200-0x101 = 0xFF; UNR_80 = (0x40000/0x180+1)/2-0x101 = 341-257 = 0x54; UNR_100 = max(0,0x100-0x101) = 0.
 | IDENT_* = (0x1000*V)>>12 = V for V0=(100,-200,300).
 | ROT90_* = Z-rotation rows (0,-1,0),(1,0,0),(0,0,1) applied to (100,200,300) = (-200,100,300).
 | RTV0TR_* = TR+V = (1000+100, -2000+200, 50+300) = (1100,-1800,350).
 | SAT_LM0: TR=(-5,70000,7) gives IR=(-5,0x7FFF,7) and FLAG = bit23|bit31 = 0x80800000.
 | SAT_LM1: same TR with lm=1 gives IR=(0,0x7FFF,7), FLAG = bit24|bit23|bit31 = 0x81800000.
 | SAT_IR3_ONLY: TR=(0,0,70000) saturates only IR3, FLAG = bit22 alone = 0x00400000 (bit22 is outside the error mask).
 | SQR0_* = IR^2 = (3^2, (-4)^2, 100^2) = (9,16,10000); SQR12_* = (0x1000^2, 0x800^2, 0x2000^2)>>12 = (4096,1024,16384).
 | PROJ: V0=(100,-50,700): SZ3=700, z=clz16(700)=6, d=700<<6=0xAF00, u=UNR[(0xAF00-0x7FC0)>>7=0x5E]+0x101=0x75+0x101,
 |       two Newton steps give n=93623 (= round(1000*0x10000/700)); SX=(93623*100+OFX)>>16=302, SY=(93623*-50+OFY)>>16=48, OTZ=700>>2=175.
 | DIVOVF: V0=(10,0,400): H<2*SZ3 fails so n=0x1FFFF, FLAG=bit17|bit31=0x80020000; SX=(0x1FFFF*10+OFX)>>16=179, SY=120.
 | SXSAT: V0=(2000,0,1000): n=0x10000, SX=2160 clamps to 0x3FF with FLAG=bit14|bit31=0x80004000; SY=120.
 | FIFO: A=(100,-50,700)->(302,48), B=(-30,40,600)->n=109227->(109,186), C=(0,0,2000)->n=32768->(160,120); SZ1..3=700,600,2000.
 | NCLIP: (0,0),(10,0),(0,10) gives SX1*SY2 = +100 (clockwise with y down); swapping the last two gives -100.
 | NCS: LLM row0=(0,0,0x1000), V0=(0,0,2048) -> IR=(2048,0,0); LCM column0=(0x1000,0x800,0x400), BK=(10,20,30)<<4
 |      -> MAC=(160+2048, 320+1024, 480+512)=(2208,1344,992) -> RGB=MAC>>4=(138,84,62), code from RGBC.
 | NCS_SAT: BK red 255<<4: MAC1=4080+2048=6128, 6128>>4=383 clamps to 255 with FLAG=bit21 only = 0x00200000.
 | RTPT_IR3: RT33=0x2000, V0z=0x7000 gives MAC3=0xE000 > 0x7FFF so IR3 saturates on the first vertex only; FLAG keeps bit22 = 0x00400000 after V1, V2.
 | MAC44_POS: TR=0x7FFFFFFF, RT=I, V=(1,1,1): 0x7FFFFFFF000+0x1000 = 2^43 > 0x7FFFFFFFFFF sets bits 30,29,28 and wraps to -2^43, so MAC=-2^43>>12=-0x80000000,
 |            IR=-0x8000 (bits 24,23,22), FLAG = 0x70000000|0x01C00000|bit31 = 0xF1C00000.
 | MAC44_NEG: TR=-0x80000000, V=(-1,-1,-1): -2^43-0x1000 < -2^43 sets bits 27,26,25 and wraps to 2^43-0x1000, so MAC=0x7FFFFFFF, IR=0x7FFF, FLAG = 0x0FC00000|bit31 = 0x8FC00000.
 | MAC0_POS: SXY=(0x7FFF,0),(0,0x7FFF),(-0x8000,-0x8000): 0x7FFF^2 + 0x7FFF*0x8000*2 = 0xBFFE0001 > 0x7FFFFFFF sets bit16; MAC0 keeps the low word, FLAG = 0x80010000.
 | MAC0_NEG: swapping the last two vertices negates the sum to -0xBFFE0001 < -0x80000000, bit15, MAC0 low word 0x4001FFFF, FLAG = 0x80008000.
 | DQ: PROJ point (n=93623) with DQA=-100, DQB=0x1400000: MAC0 = -9362300+20971520 = 11609220, IR0 = 11609220>>12 = 2834, FLAG = 0.
 | DQ_SAT: DQA=+100: MAC0 = 9362300+20971520 = 30333820, >>12 = 7405 clamps to IR0=0x1000 with bit12 alone (outside the error mask) = 0x00001000.
 | NEGZ: V0=(10,0,-100): SZ3 clamps -100 to 0 (bit18), then H=1000 >= 2*0 overflows (bit17, n=0x1FFFF) giving SX=179, SY=120; FLAG = 0x80060000.
 | DIV_Q_*: RTPS quotients read from MAC0 with DQA=1, DQB=0 at the normalising-shift extremes (SZ3 = 1, 0x7FFF, 0x8000, 0xFFFF) and two mid values, recorded from the bit-loop divide.
 | INITGEOM_*: the ctc2 immediates in SLUS_010.13 InitGeom (0x8001CBF4): ZSF3=341, ZSF4=256, H=1000, DQA=-4194, DQB=0x1400000, OFX=OFY=0.
 | DIVERGE_*: inputs where the PS1 traps (add overflow at 0x8001CE2C/0x8001D0F8, break in ratan2); the Saturn wraps or uses a 0 quotient:
 |   VectorNormal(-0x8000 x3): 3*0x40000000 wraps to -0x40000000; CompMatrix: I*(1,0,0) + t0 0x7FFFFFFF wraps to -0x80000000;
 |   ratan2(0,-0x80000000): x stays -0x80000000 after negation, quotient 0, 1024 - ratan[0]=0, then 2048 - 1024 = 1024.
 | --cases mode reads "name args..." lines and prints each libgte result plus IR0-IR3, MAC1-MAC3, LZCS, LZCR, for test_gte_host.py to compare with the PS1 code.
 ----------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "gte_soft.h"
#include "libgte.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

#define UNR_0 0xFF
#define UNR_80 0x54
#define UNR_100 0x00
#define IDENT_X 100
#define IDENT_Y (-200)
#define IDENT_Z 300
#define ROT90_X (-200)
#define ROT90_Y 100
#define ROT90_Z 300
#define RTV0TR_X 1100
#define RTV0TR_Y (-1800)
#define RTV0TR_Z 350
#define SAT_LM0_FLAG 0x80800000u
#define SAT_LM1_FLAG 0x81800000u
#define SAT_IR3_ONLY_FLAG 0x00400000u
#define PROJ_SX 302
#define PROJ_SY 48
#define PROJ_SZ 700
#define PROJ_OTZ 175
#define DIV_Q_1_1 65536
#define DIV_Q_10_9 116508
#define DIV_Q_7000_7FFF 57345
#define DIV_Q_7FFF_8000 65534
#define DIV_Q_FFFE_FFFF 65534
#define DIV_Q_123_4567 1073
#define DIVOVF_SX 179
#define DIVOVF_SY 120
#define DIVOVF_FLAG 0x80020000u
#define SXSAT_SX 0x3FF
#define SXSAT_FLAG 0x80004000u
#define FIFO_B_SX 109
#define FIFO_B_SY 186
#define FIFO_C_SX 160
#define FIFO_C_SY 120
#define NCLIP_CW 100
#define NCS_R 138
#define NCS_G 84
#define NCS_B 62
#define NCS_SAT_FLAG 0x00200000u
#define RTPT_IR3_FLAG 0x00400000u
#define FLAG_NONE 0u
#define IR_MAX 0x7FFF
#define IR_MIN (-0x8000)
#define SAT_TR_X (-5)
#define SAT_TR_Y 70000
#define SAT_TR_Z 7
#define SAT_LM1_IR_X 0
#define SQR0_X 9
#define SQR0_Y 16
#define SQR0_Z 10000
#define SQR12_X 4096
#define SQR12_Y 1024
#define SQR12_Z 16384
#define SXSAT_SY 120
#define FIFO_A_SZ 700
#define FIFO_B_SZ 600
#define FIFO_C_SZ 2000
#define NCS_CODE 0x24
#define NCS_SAT_R 255
#define STACK_ONE 0x1000
#define STACK_TR 7
#define INT32_MAX_VALUE 0x7FFFFFFF
#define INT32_MIN_VALUE (-0x7FFFFFFF - 1)
#define MAC44_POS_FLAG 0xF1C00000u
#define MAC44_NEG_FLAG 0x8FC00000u
#define MAC0_POS_VALUE ((int32_t)0xBFFE0001u)
#define MAC0_POS_FLAG 0x80010000u
#define MAC0_NEG_VALUE 0x4001FFFF
#define MAC0_NEG_FLAG 0x80008000u
#define DQ_DQA (-100)
#define DQ_DQB 0x1400000
#define DQ_MAC0 11609220
#define DQ_IR0 2834
#define DQ_SAT_DQA 100
#define DQ_SAT_IR0 0x1000
#define DQ_SAT_FLAG 0x00001000u
#define NEGZ_SX 179
#define NEGZ_SY 120
#define NEGZ_FLAG 0x80060000u
#define INITGEOM_ZSF3 341
#define INITGEOM_ZSF4 256
#define INITGEOM_H 1000
#define INITGEOM_DQA (-4194)
#define INITGEOM_DQB 0x1400000
#define INITGEOM_OFFSET 0
#define CASE_LINE 1024
#define DIVERGE_VNORM_IN (-0x8000)
#define DIVERGE_VNORM_SUM (-0x40000000L)
#define DIVERGE_COMP_T INT32_MIN_VALUE
#define DIVERGE_RATAN 1024L
#define CASE_ARGS 48
#define MATRIX_FIELDS 13
#define MATRIX_PAD_OFFSET 18

typedef struct TestMatrix
{
    int16_t m[3][3];
    int32_t t[3];
} TestMatrix;

typedef struct TestSvector
{
    int16_t vx, vy, vz, pad;
} TestSvector;

typedef struct TestVector
{
    int32_t vx, vy, vz, pad;
} TestVector;

typedef struct TestSxy
{
    int16_t x, y;
} TestSxy;

static void set_identity(TestMatrix *m)
{
    memset(m, 0, sizeof(*m));
    m->m[0][0] = 0x1000;
    m->m[1][1] = 0x1000;
    m->m[2][2] = 0x1000;
}

static void setup_projection(void)
{
    TestMatrix m;

    gte_soft_reset();
    set_identity(&m);
    gte_SetRotMatrix(&m);
    gte_SetTransMatrix(&m);
    SetGeomScreen(1000);
    SetGeomOffset(160, 120);
}

static uint32_t pack_sxy(int16_t x, int16_t y)
{
    TestSxy s = { x, y };
    uint32_t word;

    memcpy(&word, &s, sizeof(word));
    return word;
}

static void test_unr_table(void)
{
    CHECK(gte_soft_unr_entry(0) == UNR_0);
    CHECK(gte_soft_unr_entry(0x80) == UNR_80);
    CHECK(gte_soft_unr_entry(0x100) == UNR_100);
}

static void test_rtv0_identity(void)
{
    TestMatrix m;
    TestSvector v = { 100, -200, 300, 0 };
    TestVector out;
    TestSvector sv;
    int32_t flag;

    gte_soft_reset();
    set_identity(&m);
    gte_SetRotMatrix(&m);
    gte_ldv0(&v);
    gte_rtv0();
    gte_stlvnl(&out);
    gte_stsv(&sv);
    gte_stflg(&flag);
    CHECK(out.vx == IDENT_X && out.vy == IDENT_Y && out.vz == IDENT_Z);
    CHECK(sv.vx == IDENT_X && sv.vy == IDENT_Y && sv.vz == IDENT_Z);
    CHECK(flag == FLAG_NONE);
}

static void test_rtv0_rot90(void)
{
    TestMatrix m;
    TestSvector v = { 100, 200, 300, 0 };
    TestVector out;

    gte_soft_reset();
    memset(&m, 0, sizeof(m));
    m.m[0][1] = -0x1000;
    m.m[1][0] = 0x1000;
    m.m[2][2] = 0x1000;
    gte_SetRotMatrix(&m);
    gte_ldv0(&v);
    gte_rtv0();
    gte_stlvnl(&out);
    CHECK(out.vx == ROT90_X && out.vy == ROT90_Y && out.vz == ROT90_Z);
}

static void test_rtv0tr(void)
{
    TestMatrix m;
    TestSvector v = { 100, 200, 300, 0 };
    TestVector out;

    gte_soft_reset();
    set_identity(&m);
    m.t[0] = 1000;
    m.t[1] = -2000;
    m.t[2] = 50;
    gte_SetRotMatrix(&m);
    gte_SetTransMatrix(&m);
    gte_ldv0(&v);
    gte_rtv0tr();
    gte_stlvnl(&out);
    CHECK(out.vx == RTV0TR_X && out.vy == RTV0TR_Y && out.vz == RTV0TR_Z);
}

static void test_ir_saturation(void)
{
    TestMatrix m;
    TestSvector zero = { 0, 0, 0, 0 };
    TestSvector sv;
    TestVector mac;
    uint32_t flag;

    gte_soft_reset();
    memset(&m, 0, sizeof(m));
    m.t[0] = SAT_TR_X;
    m.t[1] = SAT_TR_Y;
    m.t[2] = SAT_TR_Z;
    gte_SetTransMatrix(&m);
    gte_ldv0(&zero);
    gte_rtv0tr();
    gte_stsv(&sv);
    gte_stlvnl(&mac);
    gte_stflg(&flag);
    CHECK(sv.vx == SAT_TR_X && sv.vy == IR_MAX && sv.vz == SAT_TR_Z);
    CHECK(mac.vx == SAT_TR_X && mac.vy == SAT_TR_Y && mac.vz == SAT_TR_Z);
    CHECK(flag == SAT_LM0_FLAG);

    gte_soft_mvmva(1, GTE_MX_RT, 0, GTE_CV_TR, 1);
    gte_stsv(&sv);
    gte_stflg(&flag);
    CHECK(sv.vx == SAT_LM1_IR_X && sv.vy == IR_MAX && sv.vz == SAT_TR_Z);
    CHECK(flag == SAT_LM1_FLAG);

    m.t[0] = 0;
    m.t[1] = 0;
    m.t[2] = SAT_TR_Y;
    gte_SetTransMatrix(&m);
    gte_rtv0tr();
    gte_stflg(&flag);
    CHECK(flag == SAT_IR3_ONLY_FLAG);
}

static void test_sqr(void)
{
    TestVector in0 = { 3, -4, 100, 0 };
    TestVector in12 = { 0x1000, 0x800, -0x2000, 0 };
    TestVector out;

    gte_soft_reset();
    gte_ldlvl(&in0);
    gte_sqr0();
    gte_stlvnl(&out);
    CHECK(out.vx == SQR0_X && out.vy == SQR0_Y && out.vz == SQR0_Z);
    gte_ldlvl(&in12);
    gte_sqr12();
    gte_stlvnl(&out);
    CHECK(out.vx == SQR12_X && out.vy == SQR12_Y && out.vz == SQR12_Z);
}

static void test_rtps_projection(void)
{
    TestSvector v = { 100, -50, 700, 0 };
    TestSxy sxy;
    int32_t otz;
    uint32_t flag;

    setup_projection();
    gte_ldv0(&v);
    gte_rtps();
    gte_stsxy(&sxy);
    gte_stszotz(&otz);
    gte_stflg(&flag);
    CHECK(sxy.x == PROJ_SX && sxy.y == PROJ_SY);
    CHECK(lom_gte.sz[3] == PROJ_SZ);
    CHECK(otz == PROJ_OTZ);
    CHECK(flag == FLAG_NONE);
}

static void test_rtps_divide_overflow(void)
{
    TestSvector v = { 10, 0, 400, 0 };
    TestSxy sxy;
    uint32_t flag;

    setup_projection();
    gte_ldv0(&v);
    gte_rtps();
    gte_stsxy(&sxy);
    gte_stflg(&flag);
    CHECK(sxy.x == DIVOVF_SX && sxy.y == DIVOVF_SY);
    CHECK(flag == DIVOVF_FLAG);
}

static void test_rtps_sx_saturation(void)
{
    TestSvector v = { 2000, 0, 1000, 0 };
    TestSxy sxy;
    uint32_t flag;

    setup_projection();
    gte_ldv0(&v);
    gte_rtps();
    gte_stsxy(&sxy);
    gte_stflg(&flag);
    CHECK(sxy.x == SXSAT_SX && sxy.y == SXSAT_SY);
    CHECK(flag == SXSAT_FLAG);
}

static void test_fifo_rtps_vs_rtpt(void)
{
    TestSvector a = { 100, -50, 700, 0 };
    TestSvector b = { -30, 40, 600, 0 };
    TestSvector c = { 0, 0, 2000, 0 };
    TestSxy s0, s1, s2, t0, t1, t2;

    setup_projection();
    gte_ldv0(&a);
    gte_rtps();
    gte_ldv0(&b);
    gte_rtps();
    gte_ldv0(&c);
    gte_rtps();
    gte_stsxy3(&s0, &s1, &s2);
    CHECK(s0.x == PROJ_SX && s0.y == PROJ_SY);
    CHECK(s1.x == FIFO_B_SX && s1.y == FIFO_B_SY);
    CHECK(s2.x == FIFO_C_SX && s2.y == FIFO_C_SY);
    CHECK(lom_gte.sz[1] == FIFO_A_SZ && lom_gte.sz[2] == FIFO_B_SZ && lom_gte.sz[3] == FIFO_C_SZ);

    setup_projection();
    gte_ldv3(&a, &b, &c);
    gte_rtpt();
    gte_stsxy01(&t0, &t1);
    gte_stsxy2(&t2);
    CHECK(memcmp(&s0, &t0, sizeof(s0)) == 0);
    CHECK(memcmp(&s1, &t1, sizeof(s1)) == 0);
    CHECK(memcmp(&s2, &t2, sizeof(s2)) == 0);
    CHECK(lom_gte.sz[1] == FIFO_A_SZ && lom_gte.sz[2] == FIFO_B_SZ && lom_gte.sz[3] == FIFO_C_SZ);
}

static void test_rtpt_flag_accumulates(void)
{
    TestMatrix m;
    TestSvector far = { 0, 0, 0x7000, 0 };
    TestSvector near_a = { 0, 0, 1000, 0 };
    TestSvector near_b = { 0, 0, 1500, 0 };
    uint32_t flag;

    setup_projection();
    set_identity(&m);
    m.m[2][2] = 0x2000;
    gte_SetRotMatrix(&m);
    gte_ldv3(&far, &near_a, &near_b);
    gte_rtpt();
    gte_stflg(&flag);
    CHECK(flag == RTPT_IR3_FLAG);
}

static void test_rtps_divide_boundaries(void)
{
    static const int32_t cases[][3] = {
        { 1, 1, DIV_Q_1_1 }, { 0x10, 0x9, DIV_Q_10_9 }, { 0x7000, 0x7FFF, DIV_Q_7000_7FFF },
        { 0x7FFF, 0x8000, DIV_Q_7FFF_8000 }, { 0xFFFE, 0xFFFF, DIV_Q_FFFE_FFFF }, { 0x123, 0x4567, DIV_Q_123_4567 },
    };
    unsigned i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        TestSvector v = { 0, 0, (int16_t)0, 0 };

        setup_projection();
        SetGeomScreen(cases[i][0]);
        lom_gte.dqa = 1;
        lom_gte.dqb = 0;
        lom_gte.tr[2] = cases[i][1];
        gte_ldv0(&v);
        gte_rtps();
        CHECK(lom_gte.mac[0] == cases[i][2]);
    }
}

static void test_mac44_overflow(void)
{
    TestMatrix m;
    TestSvector plus = { 1, 1, 1, 0 };
    TestSvector minus = { -1, -1, -1, 0 };
    TestVector mac;
    TestSvector sv;
    uint32_t flag;

    gte_soft_reset();
    set_identity(&m);
    m.t[0] = INT32_MAX_VALUE;
    m.t[1] = INT32_MAX_VALUE;
    m.t[2] = INT32_MAX_VALUE;
    gte_SetRotMatrix(&m);
    gte_SetTransMatrix(&m);
    gte_ldv0(&plus);
    gte_rtv0tr();
    gte_stlvnl(&mac);
    gte_stsv(&sv);
    gte_stflg(&flag);
    CHECK(mac.vx == INT32_MIN_VALUE && mac.vy == INT32_MIN_VALUE && mac.vz == INT32_MIN_VALUE);
    CHECK(sv.vx == IR_MIN && sv.vy == IR_MIN && sv.vz == IR_MIN);
    CHECK(flag == MAC44_POS_FLAG);

    m.t[0] = INT32_MIN_VALUE;
    m.t[1] = INT32_MIN_VALUE;
    m.t[2] = INT32_MIN_VALUE;
    gte_SetTransMatrix(&m);
    gte_ldv0(&minus);
    gte_rtv0tr();
    gte_stlvnl(&mac);
    gte_stsv(&sv);
    gte_stflg(&flag);
    CHECK(mac.vx == INT32_MAX_VALUE && mac.vy == INT32_MAX_VALUE && mac.vz == INT32_MAX_VALUE);
    CHECK(sv.vx == IR_MAX && sv.vy == IR_MAX && sv.vz == IR_MAX);
    CHECK(flag == MAC44_NEG_FLAG);
}

static void test_mac0_overflow(void)
{
    int32_t value;
    uint32_t flag;

    gte_soft_reset();
    gte_ldsxy3(pack_sxy(0x7FFF, 0), pack_sxy(0, 0x7FFF), pack_sxy(-0x8000, -0x8000));
    gte_nclip();
    gte_stopz(&value);
    gte_stflg(&flag);
    CHECK(value == MAC0_POS_VALUE);
    CHECK(flag == MAC0_POS_FLAG);
    gte_ldsxy3(pack_sxy(0x7FFF, 0), pack_sxy(-0x8000, -0x8000), pack_sxy(0, 0x7FFF));
    gte_nclip();
    gte_stopz(&value);
    gte_stflg(&flag);
    CHECK(value == MAC0_NEG_VALUE);
    CHECK(flag == MAC0_NEG_FLAG);
}

static void test_depth_cue(void)
{
    TestSvector v = { 100, -50, 700, 0 };
    int32_t mac0;
    uint32_t flag;

    setup_projection();
    lom_gte.dqa = DQ_DQA;
    lom_gte.dqb = DQ_DQB;
    gte_ldv0(&v);
    gte_rtps();
    gte_stopz(&mac0);
    gte_stflg(&flag);
    CHECK(mac0 == DQ_MAC0);
    CHECK(lom_gte.ir[0] == DQ_IR0);
    CHECK(flag == FLAG_NONE);

    lom_gte.dqa = DQ_SAT_DQA;
    gte_rtps();
    gte_stflg(&flag);
    CHECK(lom_gte.ir[0] == DQ_SAT_IR0);
    CHECK(flag == DQ_SAT_FLAG);
}

static void test_negative_sz(void)
{
    TestSvector v = { 10, 0, -100, 0 };
    TestSxy sxy;
    uint32_t flag;

    setup_projection();
    gte_ldv0(&v);
    gte_rtps();
    gte_stsxy(&sxy);
    gte_stflg(&flag);
    CHECK(lom_gte.sz[3] == 0);
    CHECK(sxy.x == NEGZ_SX && sxy.y == NEGZ_SY);
    CHECK(flag == NEGZ_FLAG);
}

static void test_nclip(void)
{
    int32_t area;

    gte_soft_reset();
    gte_ldsxy3(pack_sxy(0, 0), pack_sxy(10, 0), pack_sxy(0, 10));
    gte_nclip();
    gte_stopz(&area);
    CHECK(area == NCLIP_CW);
    gte_ldsxy3(pack_sxy(0, 0), pack_sxy(0, 10), pack_sxy(10, 0));
    gte_nclip();
    gte_stopz(&area);
    CHECK(area == -NCLIP_CW);
    CHECK(NormalClip((long)pack_sxy(0, 0), (long)pack_sxy(10, 0), (long)pack_sxy(0, 10)) == NCLIP_CW);
}

static void test_ncs(void)
{
    TestMatrix light;
    TestMatrix colour;
    TestSvector normal = { 0, 0, 2048, 0 };
    uint8_t rgb[4];
    uint32_t flag;

    gte_soft_reset();
    memset(&light, 0, sizeof(light));
    memset(&colour, 0, sizeof(colour));
    light.m[0][2] = 0x1000;
    colour.m[0][0] = 0x1000;
    colour.m[1][0] = 0x800;
    colour.m[2][0] = 0x400;
    gte_SetLightMatrix(&light);
    gte_SetColorMatrix(&colour);
    gte_SetBackColor(10, 20, 30);
    lom_gte.rgbc[3] = NCS_CODE;
    gte_ldv0(&normal);
    gte_ncs();
    gte_strgb(rgb);
    gte_stflg(&flag);
    CHECK(rgb[0] == NCS_R && rgb[1] == NCS_G && rgb[2] == NCS_B && rgb[3] == NCS_CODE);
    CHECK(flag == FLAG_NONE);

    gte_SetBackColor(NCS_SAT_R, 20, 30);
    gte_ncs();
    gte_strgb(rgb);
    gte_stflg(&flag);
    CHECK(rgb[0] == NCS_SAT_R && rgb[1] == NCS_G && rgb[2] == NCS_B);
    CHECK(flag == NCS_SAT_FLAG);
}

static void test_stflg_after_error(void)
{
    TestSvector v = { 10, 0, 400, 0 };
    TestSvector ok = { 100, -50, 700, 0 };
    int32_t flag;

    setup_projection();
    gte_ldv0(&v);
    gte_rtps();
    gte_stflg(&flag);
    CHECK((uint32_t)flag == DIVOVF_FLAG);
    gte_ldv0(&ok);
    gte_rtps();
    gte_stflg(&flag);
    CHECK(flag == FLAG_NONE);
}

static void test_libgte_matrix_stack(void)
{
    TestMatrix a;
    TestMatrix b;

    gte_soft_reset();
    set_identity(&a);
    a.t[0] = STACK_TR;
    memset(&b, 0, sizeof(b));
    gte_SetRotMatrix(&a);
    gte_SetTransMatrix(&a);
    PushMatrix();
    gte_SetRotMatrix(&b);
    gte_SetTransMatrix(&b);
    CHECK(lom_gte.rt[0][0] == 0 && lom_gte.tr[0] == 0);
    PopMatrix();
    CHECK(lom_gte.rt[0][0] == STACK_ONE && lom_gte.rt[2][2] == STACK_ONE && lom_gte.tr[0] == STACK_TR);
}

static void test_init_geom(void)
{
    gte_soft_reset();
    SetGeomOffset(5, 6);
    InitGeom();
    CHECK(lom_gte.zsf3 == INITGEOM_ZSF3);
    CHECK(lom_gte.zsf4 == INITGEOM_ZSF4);
    CHECK(lom_gte.h == INITGEOM_H);
    CHECK(lom_gte.dqa == INITGEOM_DQA);
    CHECK(lom_gte.dqb == INITGEOM_DQB);
    CHECK(lom_gte.ofx == INITGEOM_OFFSET && lom_gte.ofy == INITGEOM_OFFSET);
}

static void test_defined_divergences(void)
{
    VECTOR in = { DIVERGE_VNORM_IN, DIVERGE_VNORM_IN, DIVERGE_VNORM_IN, 0 };
    VECTOR out;
    MATRIX a;
    MATRIX b;
    MATRIX c;

    gte_soft_reset();
    CHECK(VectorNormal(&in, &out) == DIVERGE_VNORM_SUM);
    memset(&a, 0, sizeof(a));
    a.m[0][0] = a.m[1][1] = a.m[2][2] = STACK_ONE;
    b = a;
    a.t[0] = INT32_MAX_VALUE;
    b.t[0] = 1;
    CompMatrix(&a, &b, &c);
    CHECK(c.t[0] == DIVERGE_COMP_T && c.t[1] == 0 && c.t[2] == 0);
    CHECK(ratan2(0, INT32_MIN_VALUE) == DIVERGE_RATAN);
}

static void put_matrix(MATRIX *m, const long *a)
{
    int16_t pad = (int16_t)a[9];
    int i;

    for (i = 0; i < 9; i++)
    {
        m->m[i / 3][i % 3] = (short)a[i];
    }
    memcpy((uint8_t *)m + MATRIX_PAD_OFFSET, &pad, sizeof(pad));
    m->t[0] = a[10];
    m->t[1] = a[11];
    m->t[2] = a[12];
}

static void print_matrix(const MATRIX *m)
{
    int16_t pad;
    int i;

    memcpy(&pad, (const uint8_t *)m + MATRIX_PAD_OFFSET, sizeof(pad));
    for (i = 0; i < 9; i++)
    {
        printf(" %d", m->m[i / 3][i % 3]);
    }
    printf(" %d %ld %ld %ld", pad, m->t[0], m->t[1], m->t[2]);
}

static void put_vector(VECTOR *v, const long *a)
{
    v->vx = a[0];
    v->vy = a[1];
    v->vz = a[2];
    v->pad = 0;
}

static void run_case(const char *name, const long *a)
{
    MATRIX m0, m1, m2;
    VECTOR v0, v1;
    SVECTOR sv;

    memset(&v1, 0x11, sizeof(v1));
    if (strcmp(name, "rsin") == 0) printf("%d", rsin((int)a[0]));
    else if (strcmp(name, "rcos") == 0) printf("%d", rcos((int)a[0]));
    else if (strcmp(name, "csin") == 0) printf("%d", csin((int)a[0]));
    else if (strcmp(name, "ccos") == 0) printf("%d", ccos((int)a[0]));
    else if (strcmp(name, "SquareRoot0") == 0) printf("%ld", SquareRoot0(a[0]));
    else if (strcmp(name, "ratan2") == 0) printf("%ld", ratan2(a[0], a[1]));
    else if (strcmp(name, "VectorNormal") == 0)
    {
        put_vector(&v0, a);
        printf("%ld", VectorNormal(&v0, &v1));
        printf(" %ld %ld %ld %ld", v1.vx, v1.vy, v1.vz, v1.pad);
    }
    else if (strcmp(name, "VectorNormalS") == 0)
    {
        put_vector(&v0, a);
        memset(&sv, 0x11, sizeof(sv));
        printf("%ld", VectorNormalS(&v0, &sv));
        printf(" %d %d %d %d", sv.vx, sv.vy, sv.vz, sv.pad);
    }
    else if (strcmp(name, "RotMatrix") == 0 || strcmp(name, "RotMatrix_gte") == 0)
    {
        sv.vx = (short)a[0];
        sv.vy = (short)a[1];
        sv.vz = (short)a[2];
        put_matrix(&m0, a + 3);
        CHECK((name[9] ? RotMatrix_gte(&sv, &m0) : RotMatrix(&sv, &m0)) == &m0);
        print_matrix(&m0);
    }
    else if (strncmp(name, "RotMatrix", 9) == 0 && name[10] == '\0')
    {
        MATRIX *(*rot)(long, MATRIX *) = name[9] == 'X' ? RotMatrixX : name[9] == 'Y' ? RotMatrixY : RotMatrixZ;

        put_matrix(&m0, a + 1);
        CHECK(rot(a[0], &m0) == &m0);
        print_matrix(&m0);
    }
    else if (strcmp(name, "ApplyMatrixLV") == 0)
    {
        put_matrix(&m0, a);
        put_vector(&v0, a + MATRIX_FIELDS);
        CHECK(ApplyMatrixLV(&m0, &v0, &v1) == &v1);
        printf("%ld %ld %ld %ld", v1.vx, v1.vy, v1.vz, v1.pad);
    }
    else if (strcmp(name, "CompMatrix") == 0)
    {
        put_matrix(&m0, a);
        put_matrix(&m1, a + MATRIX_FIELDS);
        put_matrix(&m2, a + 2 * MATRIX_FIELDS);
        CHECK(CompMatrix(&m0, &m1, &m2) == &m2);
        print_matrix(&m2);
    }
    else if (strcmp(name, "ScaleMatrix") == 0)
    {
        put_matrix(&m0, a);
        put_vector(&v0, a + MATRIX_FIELDS);
        CHECK(ScaleMatrix(&m0, &v0) == &m0);
        print_matrix(&m0);
    }
    else
    {
        printf("unknown");
    }
    printf(" %d %d %d %d %ld %ld %ld %ld %ld\n", lom_gte.ir[0], lom_gte.ir[1], lom_gte.ir[2], lom_gte.ir[3],
           (long)lom_gte.mac[1], (long)lom_gte.mac[2], (long)lom_gte.mac[3], (long)lom_gte.lzcs, (long)lom_gte.lzcr);
}

static int run_cases(void)
{
    char line[CASE_LINE];

    while (fgets(line, sizeof(line), stdin) != NULL)
    {
        long args[CASE_ARGS];
        char *name = strtok(line, " \r\n");
        char *token;
        int count = 0;

        if (name == NULL)
        {
            continue;
        }
        while (count < CASE_ARGS && (token = strtok(NULL, " \r\n")) != NULL)
        {
            args[count++] = strtol(token, NULL, 10);
        }
        while (count < CASE_ARGS)
        {
            args[count++] = 0;
        }
        gte_soft_reset();
        run_case(name, args);
    }
    return failures == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--cases") == 0)
    {
        return run_cases();
    }
    test_unr_table();
    test_rtv0_identity();
    test_rtv0_rot90();
    test_rtv0tr();
    test_ir_saturation();
    test_sqr();
    test_rtps_projection();
    test_rtps_divide_overflow();
    test_rtps_sx_saturation();
    test_fifo_rtps_vs_rtpt();
    test_rtpt_flag_accumulates();
    test_rtps_divide_boundaries();
    test_mac44_overflow();
    test_mac0_overflow();
    test_depth_cue();
    test_negative_sz();
    test_nclip();
    test_ncs();
    test_stflg_after_error();
    test_libgte_matrix_stack();
    test_init_geom();
    test_defined_divergences();
    if (failures == 0)
    {
        printf("OK\n");
        return 0;
    }
    printf("%d failures\n", failures);
    return 1;
}
