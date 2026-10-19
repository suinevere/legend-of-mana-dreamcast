/*----------------------
 | test_scene_sweep.c
 | Description: Host test for the scene sweep's state machine (jump, arrive, settle, timeout, leaving, busy transitions, clamping) and its range parser.
 | Author: suinevere
 | Dependencies: scene_sweep.h, scene_sweep.c
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "scene_sweep.h"

static int fails;

static void expect(int line, SweepEvent e, int kind, int scene, int reached)
{
    if (e.kind != kind || (kind != SWEEP_EVENT_NONE && kind != SWEEP_EVENT_DONE && e.scene != scene) ||
        (kind == SWEEP_EVENT_STUCK && e.reached != reached))
    {
        printf("line %d: got kind %d scene %d reached %d, want %d %d %d\n", line, e.kind, e.scene, e.reached, kind, scene, reached);
        fails++;
    }
}

#define EXPECT(e, k, s, r) expect(__LINE__, (e), (k), (s), (r))

static void test_walk(void)
{
    SceneSweep s;
    int i;

    scene_sweep_start(&s, 5, 6);
    EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_JUMP, 5, 0);
    EXPECT(scene_sweep_step(&s, 19, 1), SWEEP_EVENT_NONE, 0, 0);
    EXPECT(scene_sweep_step(&s, 5, 1), SWEEP_EVENT_NONE, 0, 0);
    EXPECT(scene_sweep_step(&s, 5, 0), SWEEP_EVENT_ARRIVE, 5, 0);
    for (i = 0; i < SCENE_SWEEP_SETTLE_VBLANKS - 1; i++)
    {
        EXPECT(scene_sweep_step(&s, 5, 0), SWEEP_EVENT_NONE, 0, 0);
    }
    EXPECT(scene_sweep_step(&s, 5, 0), SWEEP_EVENT_OK, 5, 0);
    EXPECT(scene_sweep_step(&s, 5, 0), SWEEP_EVENT_JUMP, 6, 0);
}

static void test_timeout(void)
{
    SceneSweep s;
    int i;

    scene_sweep_start(&s, 7, 7);
    EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_JUMP, 7, 0);
    for (i = 0; i < SCENE_SWEEP_ARRIVE_VBLANKS - 1; i++)
    {
        EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_NONE, 0, 0);
    }
    EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_STUCK, 7, 19);
    EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_DONE, 0, 0);
    EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_NONE, 0, 0);
    EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_NONE, 0, 0);
}

static void test_leaving(void)
{
    SceneSweep s;

    scene_sweep_start(&s, 8, 8);
    EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_JUMP, 8, 0);
    EXPECT(scene_sweep_step(&s, 8, 0), SWEEP_EVENT_ARRIVE, 8, 0);
    EXPECT(scene_sweep_step(&s, 9, 0), SWEEP_EVENT_STUCK, 8, 9);
}

static void test_busy_holds_jump(void)
{
    SceneSweep s;

    scene_sweep_start(&s, 1, 1);
    EXPECT(scene_sweep_step(&s, 19, 1), SWEEP_EVENT_NONE, 0, 0);
    EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_JUMP, 1, 0);
}

static void test_clamp(void)
{
    SceneSweep s;

    scene_sweep_start(&s, -4, 5000);
    EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_JUMP, 0, 0);
    if (s.last != SCENE_SWEEP_LAST)
    {
        printf("clamp: last %d\n", s.last);
        fails++;
    }
    scene_sweep_start(&s, 10, 3);
    EXPECT(scene_sweep_step(&s, 19, 0), SWEEP_EVENT_NONE, 0, 0);
}

static void parse(int line, const char *text, unsigned len, int want_rc, int want_first, int want_last)
{
    int first = -99, last = -99;
    int rc = scene_sweep_parse_range(text, len, &first, &last);

    if (rc != want_rc || (rc == 0 && (first != want_first || last != want_last)) || (rc != 0 && (first != -99 || last != -99)))
    {
        printf("line %d: parse rc %d (%d, %d)\n", line, rc, first, last);
        fails++;
    }
}

static void test_parse(void)
{
    char block[512];

    parse(__LINE__, "0 929\n", 6, 0, 0, 929);
    parse(__LINE__, "12\t40", 5, 0, 12, 40);
    parse(__LINE__, "300 200\n", 8, -1, 0, 0);
    parse(__LINE__, "abc", 3, -1, 0, 0);
    parse(__LINE__, "5", 1, -1, 0, 0);
    parse(__LINE__, "-3 9", 4, -1, 0, 0);
    parse(__LINE__, "0 5000\n", 7, 0, 0, 929);
    memset(block, 0, sizeof block);
    memcpy(block, "20 60\n", 6);
    parse(__LINE__, block, sizeof block, 0, 20, 60);
}

int main(void)
{
    test_walk();
    test_timeout();
    test_leaving();
    test_busy_holds_jump();
    test_clamp();
    test_parse();
    if (fails)
    {
        printf("FAIL %d\n", fails);
        return 1;
    }
    printf("OK\n");
    return 0;
}
