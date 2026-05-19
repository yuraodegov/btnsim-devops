#ifndef BTN_FSM_H
#define BTN_FSM_H

#include <stdint.h>

#define LONG_PRESS_MS   800
#define DOUBLE_CLICK_MS 400

typedef enum {
    BTN_IDLE = 0,
    BTN_PRESSED,
    BTN_HELD,
    BTN_PENDING
} BtnState;

typedef enum {
    EVENT_NONE = 0,
    EVENT_PRESS,
    EVENT_SHORT_CLICK,
    EVENT_DOUBLE_CLICK,
    EVENT_LONG_PRESS,
    EVENT_LONG_PRESS_RELEASE
} BtnEvent;

typedef struct {
    BtnState  state;
    uint32_t  press_tick;
    uint32_t  last_release;
    int       long_fired;
    int       is_pressed;
} BtnFSM;

void     btn_init(BtnFSM *btn);
BtnEvent btn_press(BtnFSM *btn, uint32_t tick);
BtnEvent btn_release(BtnFSM *btn, uint32_t tick);
BtnEvent btn_update(BtnFSM *btn, uint32_t tick);

const char* btn_event_str(BtnEvent ev);
const char* btn_state_str(BtnState st);

#endif