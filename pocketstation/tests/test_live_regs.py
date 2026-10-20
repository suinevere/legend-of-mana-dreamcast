"""----------------------
 | test_live_regs.py
 | Description: Tests the register liveness pass over R1-style register-level C.
 | Author: suinevere
 | Dependencies: pocketstation/tools/live_regs.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))
import live_regs  # noqa: E402

SRC = """
void caller(PskCpu *c) {
    c->r[0] = 0x00000005u;
    psk_call(c, 0x02000101u, 0x02000020u);
    c->r[2] = psk_add_flags(c, c->r[0], c->r[7]);
    c->r[7] = 0x00000000u;
    psk_w8(c->r[2], (uint8_t)c->r[7]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

void leaf(PskCpu *c) {
    c->r[7] = psk_add_flags(c, c->r[6], 0x00000020u);
    c->r[5] = 0x00000001u;
    c->r[0] = psk_r8(c->r[7]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

void flagged(PskCpu *c) {
    psk_sub_flags(c, c->r[0], 0x00000001u);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

void flag_user(PskCpu *c) {
    psk_call(c, 0x02000201u, 0x02000030u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000040;
    }
    c->r[1] = 0x00000001u;
a_02000040:
    c->r[15] = psk_bx(c->r[14]);
    return;
}

void head(PskCpu *c) {
    c->r[3] = 0x00000001u;
    c->r[15] = 0x02000301u;
    return;
}

void tail(PskCpu *c) {
    psk_w8(c->r[3], 0u);
    c->r[15] = psk_bx(c->r[14]);
    return;
}
"""

HOOKS = {0x02000000: "caller", 0x02000100: "leaf", 0x02000200: "flagged", 0x02000210: "flag_user",
         0x02000280: "head", 0x02000300: "tail"}


def analysis():
    return live_regs.analyse(SRC, HOOKS, tables=lambda base: [], roots={"caller", "flag_user", "head"})


def test_a_leafs_live_outputs_are_what_its_caller_reads_before_writing():
    assert live_regs.names(analysis().live_out["leaf"]) == ["r0", "r7", "r13"]


def test_flags_are_live_out_when_a_caller_branches_on_them():
    assert "flags" in live_regs.names(analysis().live_out["flagged"])


def test_a_root_returns_nothing_but_the_stack_pointer():
    assert live_regs.names(analysis().live_out["caller"]) == ["r13"]


def test_a_continuation_target_inherits_its_predecessors_live_out_and_feeds_its_live_in():
    a = analysis()
    assert "r3" in live_regs.names(a.live_in["tail"])
    assert live_regs.names(a.live_out["tail"]) == live_regs.names(a.live_out["head"])


def test_outputs_are_live_registers_the_routine_writes():
    a = analysis()
    assert live_regs.names(a.outputs["leaf"]) == ["r0", "r7"]
    assert live_regs.names(a.outputs["flagged"]) == ["flags"]


def test_inputs_are_registers_read_before_written():
    assert live_regs.names(analysis().inputs["leaf"]) == ["r6", "r13"]


def test_nz_setting_kills_the_flags_an_eq_branch_reads():
    src = """
void caller(PskCpu *c) {
    psk_call(c, 0x02000101u, 0x02000020u);
    psk_nz(c, c->r[1] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000030;
    }
a_02000030:
    c->r[15] = psk_bx(c->r[14]);
    return;
}

void leaf(PskCpu *c) {
    psk_sub_flags(c, c->r[0], 0x00000001u);
    c->r[1] = 0x00000002u;
    c->r[15] = psk_bx(c->r[14]);
    return;
}
"""
    a = live_regs.analyse(src, {0x02000000: "caller", 0x02000100: "leaf"}, tables=lambda b: [], roots={"caller"})
    assert live_regs.names(a.outputs["leaf"]) == ["r1"]


def test_a_veneer_through_r12_is_a_direct_jump_and_a_popped_register_is_a_return():
    src = """
void veneer(PskCpu *c) {
    c->r[12] = 0x02000101u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

void leaf(PskCpu *c) {
    c->r[4] = 0x00000001u;
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

void caller(PskCpu *c) {
    psk_call(c, 0x02000001u, 0x02000020u);
    psk_w8(c->r[4], 0u);
    c->r[15] = psk_bx(c->r[14]);
    return;
}
"""
    a = live_regs.analyse(src, {0x02000000: "veneer", 0x02000100: "leaf", 0x02000200: "caller"},
                          tables=lambda b: [], roots={"caller"})
    assert live_regs.names(a.outputs["leaf"]) == ["r4"]
    assert live_regs.names(a.outputs["veneer"]) == ["r4"]


def test_a_register_only_saved_and_restored_by_a_later_routine_is_not_an_output():
    src = """
void caller(PskCpu *c) {
    psk_call(c, 0x02000101u, 0x02000020u);
    psk_call(c, 0x02000201u, 0x02000030u);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

void clobber(PskCpu *c) {
    c->r[5] = 0x00000007u;
    c->r[15] = psk_bx(c->r[14]);
    return;
}

void saver(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[5]);
        c->r[13] = a - 4u;
    }
    {
        uint32_t a = c->r[13];
        c->r[5] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}
"""
    a = live_regs.analyse(src, {0x02000000: "caller", 0x02000100: "clobber", 0x02000200: "saver"},
                          tables=lambda b: [], roots={"caller"})
    assert live_regs.names(a.outputs["clobber"]) == []
