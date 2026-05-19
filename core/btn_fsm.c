#include "btn_fsm.h"
#include <stdlib.h>
#include <stdio.h>

void btn_init(BtnFSM *btn)
{
    btn->state        = BTN_IDLE;
    btn->press_tick   = 0;
    btn->last_release = 0;
    btn->long_fired   = 0;
    btn->is_pressed   = 0;
    printf("HELLO TESTER\n");
}

BtnEvent btn_press(BtnFSM *btn, uint32_t tick)
{
    if (btn->is_pressed)
        return EVENT_NONE;

    btn->is_pressed = 1;
    btn->press_tick = tick;
    btn->long_fired = 0;
    btn->state      = BTN_PRESSED;

    return EVENT_PRESS;
}

BtnEvent btn_update(BtnFSM *btn, uint32_t tick)
{
    if (btn->is_pressed &&
        btn->state == BTN_PRESSED &&
        !btn->long_fired &&
        (tick - btn->press_tick) >= LONG_PRESS_MS)
    {
        btn->state      = BTN_HELD;
        btn->long_fired = 1;
        return EVENT_LONG_PRESS;
    }

    if (!btn->is_pressed &&
        btn->state == BTN_PENDING &&
        (tick - btn->last_release) >= DOUBLE_CLICK_MS)
    {
        btn->state        = BTN_IDLE;
        btn->last_release = 0;
        return EVENT_SHORT_CLICK;
    }

    return EVENT_NONE;
}

BtnEvent btn_release(BtnFSM *btn, uint32_t tick)
{
    if (!btn->is_pressed)
        return EVENT_NONE;

    btn->is_pressed = 0;

    if (btn->state == BTN_HELD) {
        btn->state        = BTN_IDLE;
        btn->last_release = tick;
        return EVENT_LONG_PRESS_RELEASE;
    }

    if (btn->state == BTN_PRESSED &&
        btn->last_release > 0 &&
        (tick - btn->last_release) < DOUBLE_CLICK_MS)
    {
        btn->state        = BTN_IDLE;
        btn->last_release = 0;
        return EVENT_DOUBLE_CLICK;
    }

    btn->state        = BTN_PENDING;
    btn->last_release = tick;
    return EVENT_NONE;
}

const char* btn_event_str(BtnEvent ev)
{
    switch (ev) {
    case EVENT_PRESS:              return "PRESS";
    case EVENT_SHORT_CLICK:        return "SHORT_CLICK";
    case EVENT_DOUBLE_CLICK:       return "DOUBLE_CLICK";
    case EVENT_LONG_PRESS:         return "LONG_PRESS";
    case EVENT_LONG_PRESS_RELEASE: return "LONG_PRESS_RELEASE";
    default:                       return "NONE";
    }
}

const char* btn_state_str(BtnState st)
{
    switch (st) {
    case BTN_IDLE:    return "IDLE";
    case BTN_PRESSED: return "PRESSED";
    case BTN_HELD:    return "HELD";
    case BTN_PENDING: return "PENDING";
    default:          return "UNKNOWN";
    }
}