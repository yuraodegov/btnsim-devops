#ifndef UI_LOGIC_H
#define UI_LOGIC_H

#include "../core/btn_fsm.h"

typedef struct {
    BtnFSM fsm;
    char last_log[128];
} UIButton;

void ui_btn_init(UIButton *btn);

void ui_btn_press(
    UIButton *btn,
    unsigned int tick);

void ui_btn_release(
    UIButton *btn,
    unsigned int tick);

#endif