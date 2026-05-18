#include "btn_fsm.h"

void btn_init(BtnFSM *btn)
{
    btn->state = BTN_IDLE;
    btn->press_tick = 0;
    btn->last_release = 0;
    btn->click_count = 0;
    btn->long_fired = 0;
    btn->is_pressed = 0;
}

BtnEvent btn_press(BtnFSM *btn, uint32_t tick)
{
    if (btn->is_pressed)
        return EVENT_NONE;

    btn->is_pressed = 1;
    btn->press_tick = tick;
    btn->long_fired = 0;
    btn->state = BTN_PRESSED;

    return EVENT_PRESS;
}

BtnEvent btn_update(BtnFSM *btn, uint32_t tick)
{
    if (!btn->is_pressed)
        return EVENT_NONE;

    if (btn->state == BTN_PRESSED &&
        !btn->long_fired &&
        (tick - btn->press_tick) >= LONG_PRESS_MS)
    {
        btn->state = BTN_HELD;
        btn->long_fired = 1;

        return EVENT_LONG_PRESS;
    }

    return EVENT_NONE;
}

BtnEvent btn_release(BtnFSM *btn, uint32_t tick)
{
    if (!btn->is_pressed)
        return EVENT_NONE;

    btn->is_pressed = 0;

    if (btn->state == BTN_HELD) {
        btn->state = BTN_IDLE;
        btn->last_release = tick;

        return EVENT_LONG_PRESS_RELEASE;
    }

    uint32_t since = tick - btn->last_release;

    btn->state = BTN_IDLE;

    BtnEvent ev;

    if (btn->click_count > 0 &&
        since < DOUBLE_CLICK_MS)
    {
        btn->click_count = 0;
        ev = EVENT_DOUBLE_CLICK;
    }
    else {
        btn->click_count = 1;
        ev = EVENT_SHORT_CLICK;
    }

    btn->last_release = tick;

    return ev;
}

const char* btn_event_str(BtnEvent ev)
{
    switch (ev) {
    case EVENT_PRESS:
        return "PRESS";

    case EVENT_SHORT_CLICK:
        return "SHORT_CLICK";

    case EVENT_DOUBLE_CLICK:
        return "DOUBLE_CLICK";

    case EVENT_LONG_PRESS:
        return "LONG_PRESS";

    case EVENT_LONG_PRESS_RELEASE:
        return "LONG_PRESS_RELEASE";

    default:
        return "NONE";
    }
}

const char* btn_state_str(BtnState st)
{
    switch (st) {
    case BTN_IDLE:
        return "IDLE";

    case BTN_PRESSED:
        return "PRESSED";

    case BTN_HELD:
        return "HELD";

    default:
        return "UNKNOWN";
    }
}