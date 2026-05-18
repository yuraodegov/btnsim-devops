#include <stdio.h>
#include <string.h>

#include "../core/btn_fsm.h"

typedef struct {
    const char *name;
    int passed;
    char msg[256];
} TestResult;

#define ASSERT_EQ_STR(a,b,m) \
    if(strcmp((a),(b)) != 0){ \
        sprintf(r->msg, "FAIL: %s", m); \
        r->passed = 0; \
        return; \
    }

#define ASSERT_EQ_INT(a,b,m) \
    if((a)!=(b)){ \
        sprintf(r->msg, "FAIL: %s", m); \
        r->passed = 0; \
        return; \
    }

static void test_short_click(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;

    btn_init(&btn);

    btn_press(&btn, 1000);

    BtnEvent ev =
        btn_release(&btn, 1100);

    ASSERT_EQ_INT(ev, EVENT_SHORT_CLICK,
        "expected short click");

    ASSERT_EQ_INT(btn.state, BTN_IDLE,
        "expected idle");
}

static void test_long_press(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;

    btn_init(&btn);

    btn_press(&btn, 1000);

    BtnEvent ev =
        btn_update(&btn, 1900);

    ASSERT_EQ_INT(ev, EVENT_LONG_PRESS,
        "expected long press");

    ASSERT_EQ_INT(btn.state, BTN_HELD,
        "expected held");

    ev = btn_release(&btn, 2000);

    ASSERT_EQ_INT(ev,
        EVENT_LONG_PRESS_RELEASE,
        "expected long release");
}

static void test_double_click(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;

    btn_init(&btn);

    btn_press(&btn, 1000);
    btn_release(&btn, 1050);

    btn_press(&btn, 1200);

    BtnEvent ev =
        btn_release(&btn, 1250);

    ASSERT_EQ_INT(ev,
        EVENT_DOUBLE_CLICK,
        "expected double click");
}

static void test_boundary(TestResult *r)
{
    r->passed = 1;
    strcpy(r->msg, "OK");

    BtnFSM btn;

    btn_init(&btn);

    btn_press(&btn, 1000);

    BtnEvent ev =
        btn_update(&btn, 1799);

    ASSERT_EQ_INT(ev,
        EVENT_NONE,
        "should not long fire");

    ASSERT_EQ_INT(btn.state,
        BTN_PRESSED,
        "still pressed");

    ev = btn_update(&btn, 1800);

    ASSERT_EQ_INT(ev,
        EVENT_LONG_PRESS,
        "should long fire");
}

int main(void)
{
    struct {
        const char *name;
        void (*fn)(TestResult *);
    } tests[] = {

        {"test_short_click", test_short_click},
        {"test_long_press",  test_long_press},
        {"test_double_click",test_double_click},
        {"test_boundary",    test_boundary},
    };

    int total =
        sizeof(tests)/sizeof(tests[0]);

    int passed = 0;

    printf("\n");
    printf("=====================================\n");
    printf("BTNSIM TEST RUNNER\n");
    printf("=====================================\n");

    for (int i = 0; i < total; i++)
    {
        TestResult r = {0};

        tests[i].fn(&r);

        if (r.passed) {
            passed++;

            printf("[PASS] %s\n",
                tests[i].name);
        }
        else {
            printf("[FAIL] %s -- %s\n",
                tests[i].name,
                r.msg);
        }
    }

    printf("-------------------------------------\n");
    printf("RESULT: %d/%d PASSED\n",
        passed,
        total);

    printf("=====================================\n");

    return (passed == total) ? 0 : 1;
}