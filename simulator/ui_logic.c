#include "ui_logic.h"

#include <stdio.h>
#include <string.h>

void ui_btn_init(UIButton *btn)
{
    btn_init(&btn->fsm);

    strcpy(
        btn->last_log,
        "UI INIT");
}

void ui_btn_press(
    UIButton *btn,
    unsigned int tick)
{
    BtnEvent ev =
        btn_press(&btn->fsm, tick);

    sprintf(
        btn->last_log,
        "PRESS:%s",
        btn_event_str(ev));
}

void ui_btn_release(
    UIButton *btn,
    unsigned int tick)
{
    BtnEvent ev =
        btn_release(&btn->fsm, tick);

    sprintf(
        btn->last_log,
        "RELEASE:%s",
        btn_event_str(ev));
}