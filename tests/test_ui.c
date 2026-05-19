#include <stdio.h>
#include <string.h>

#include "../simulator/ui_logic.h"

static int fails = 0;

#define ASSERT_STR(a,b,msg) \
    if(strcmp((a),(b)) != 0){ \
        printf("[FAIL] %s\n", msg); \
        fails++; \
    }

static void test_ui_press(void)
{
    UIButton btn;

    ui_btn_init(&btn);

    ui_btn_press(&btn, 1000);

    ASSERT_STR(
        btn.last_log,
        "PRESS:PRESS",
        "ui press log");
}

int main(void)
{
    printf("UI TESTS\n");

    test_ui_press();

    if(fails == 0) {
        printf("[PASS] ALL UI TESTS\n");
        return 0;
    }

    return 1;
}