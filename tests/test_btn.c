#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../core/btn_fsm.h"

typedef struct {
    const char *name;
    int         passed;
    char        msg[256];
} TestResult;

#define ASSERT_EQ_INT(a, b, m) \
    if ((a) != (b)) { \
        snprintf(r->msg, sizeof(r->msg), \
            "FAIL: %s (got %d, expected %d)", (m), (int)(a), (int)(b)); \
        r->passed = 0; \
        return; \
    }

/* ── тест 1 ────────────────────────────────────────────────────────────
   SHORT_CLICK: нажать → отпустить → подождать > 400ms → SHORT_CLICK
   (не сразу на release, а через btn_update после таймаута)           */
static void test_short_click(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;
    btn_init(&btn);

    btn_press(&btn, 1000);
    btn_release(&btn, 1100);              /* → BTN_PENDING, EVENT_NONE  */
    BtnEvent ev = btn_update(&btn, 1550); /* 450ms > 400ms → SHORT_CLICK */

    ASSERT_EQ_INT(ev,        EVENT_SHORT_CLICK, "expected SHORT_CLICK");
    ASSERT_EQ_INT(btn.state, BTN_IDLE,          "expected IDLE");
}

/* ── тест 2 ────────────────────────────────────────────────────────────
   LONG_PRESS: удержать 900ms → LONG_PRESS, отпустить → LONG_PRESS_RELEASE */
static void test_long_press(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;
    btn_init(&btn);

    btn_press(&btn, 1000);
    BtnEvent ev = btn_update(&btn, 1900);

    ASSERT_EQ_INT(ev,        EVENT_LONG_PRESS, "expected LONG_PRESS");
    ASSERT_EQ_INT(btn.state, BTN_HELD,         "expected HELD");

    ev = btn_release(&btn, 2000);

    ASSERT_EQ_INT(ev,        EVENT_LONG_PRESS_RELEASE, "expected LONG_PRESS_RELEASE");
    ASSERT_EQ_INT(btn.state, BTN_IDLE,                 "expected IDLE after release");
}

/* ── тест 3 ────────────────────────────────────────────────────────────
   DOUBLE_CLICK: два клика в пределах 400ms                            */
static void test_double_click(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;
    btn_init(&btn);

    btn_press(&btn, 1000);
    btn_release(&btn, 1050);   /* первый клик → BTN_PENDING             */
    btn_press(&btn, 1150);
    BtnEvent ev = btn_release(&btn, 1200); /* второй клик в 400ms окне  */

    ASSERT_EQ_INT(ev,        EVENT_DOUBLE_CLICK, "expected DOUBLE_CLICK");
    ASSERT_EQ_INT(btn.state, BTN_IDLE,           "expected IDLE");
}

/* ── тест 4 ────────────────────────────────────────────────────────────
   BOUNDARY: 799ms — нет long press, 800ms — есть                      */
static void test_boundary(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;
    btn_init(&btn);

    btn_press(&btn, 1000);

    BtnEvent ev = btn_update(&btn, 1799);
    ASSERT_EQ_INT(ev,        EVENT_NONE,    "no LONG_PRESS at 799ms");
    ASSERT_EQ_INT(btn.state, BTN_PRESSED,  "still PRESSED at 799ms");

    ev = btn_update(&btn, 1800);
    ASSERT_EQ_INT(ev, EVENT_LONG_PRESS, "LONG_PRESS at 800ms");
}

/* ── тест 5 ────────────────────────────────────────────────────────────
   Два клика далеко друг от друга → два SHORT_CLICK, не DOUBLE         */
static void test_no_double_after_timeout(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;
    btn_init(&btn);

    btn_press(&btn, 1000);
    btn_release(&btn, 1050);
    BtnEvent ev = btn_update(&btn, 1500);  /* 450ms → SHORT_CLICK       */
    ASSERT_EQ_INT(ev, EVENT_SHORT_CLICK, "first click = SHORT");

    btn_press(&btn, 1600);
    btn_release(&btn, 1650);
    ev = btn_update(&btn, 2100);           /* тоже SHORT                */
    ASSERT_EQ_INT(ev, EVENT_SHORT_CLICK, "second click = SHORT too");
}

/* ── тест 6 ────────────────────────────────────────────────────────────
   LONG_PRESS не должен срабатывать дважды при повторных btn_update    */
static void test_long_press_no_repeat(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;
    btn_init(&btn);

    btn_press(&btn, 1000);
    BtnEvent ev = btn_update(&btn, 1900);
    ASSERT_EQ_INT(ev, EVENT_LONG_PRESS, "first update = LONG_PRESS");

    ev = btn_update(&btn, 2000);
    ASSERT_EQ_INT(ev, EVENT_NONE, "second update = NONE (no repeat)");
}

/* ── тест 7 ────────────────────────────────────────────────────────────
   Начальное состояние после btn_init                                   */
static void test_idle_init(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;
    btn_init(&btn);

    ASSERT_EQ_INT(btn.state,      BTN_IDLE, "initial state = IDLE");
    ASSERT_EQ_INT(btn.is_pressed, 0,        "not pressed");
    ASSERT_EQ_INT(btn.long_fired, 0,        "long_fired = 0");
}

/* ── main ──────────────────────────────────────────────────────────────── */

static void test_stdout_output(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    FILE *fp =
        freopen("test_output.txt", "w", stdout);

    if (!fp) {
        r->passed = 0;
        strcpy(r->msg, "stdout redirect failed");
        return;
    }

    BtnFSM btn;

    btn_init(&btn);

    fflush(stdout);

    fclose(stdout);

    FILE *f =
        fopen("test_output.txt", "r");

    if (!f) {
        r->passed = 0;
        strcpy(r->msg, "cannot open output file");
        return;
    }

    char buffer[256] = {0};

    size_t n =
    fread(buffer,
          1,
          sizeof(buffer)-1,
          f);

    (void)n;

    fclose(f);

    if (strstr(buffer,
        "HELLO TESTER") == NULL)
    {
        r->passed = 0;

        strcpy(
            r->msg,
            "HELLO TESTER missing");
    }
}

int main(void)
{
    struct {
        const char   *name;
        void        (*fn)(TestResult *);
    } tests[] = {
        {"test_short_click",            test_short_click},
        {"test_long_press",             test_long_press},
        {"test_double_click",           test_double_click},
        {"test_boundary",               test_boundary},
        {"test_no_double_after_timeout",test_no_double_after_timeout},
        {"test_long_press_no_repeat",   test_long_press_no_repeat},
        {"test_idle_init",              test_idle_init},
        {"test_stdout_output", test_stdout_output},
    };

    int total  = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;

    printf("\n=====================================\n");
    printf("BTNSIM TEST RUNNER\n");
    printf("=====================================\n");

    for (int i = 0; i < total; i++) {
        TestResult r = {0};
        tests[i].fn(&r);
        if (r.passed) {
            passed++;
            printf("[PASS] %s\n", tests[i].name);
        } else {
            printf("[FAIL] %s -- %s\n", tests[i].name, r.msg);
        }
    }

    printf("-------------------------------------\n");
    printf("RESULT: %d/%d PASSED\n", passed, total);
    printf("=====================================\n\n");

    return (passed == total) ? 0 : 1;
}