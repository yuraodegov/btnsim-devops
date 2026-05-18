#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "../core/btn_fsm.h"

#define NUM_BUTTONS 3

static BtnFSM g_fsm[NUM_BUTTONS];

int WINAPI WinMain(HINSTANCE hInst,
                   HINSTANCE hPrev,
                   LPSTR lpCmd,
                   int nShow)
{
    MessageBoxA(
        NULL,
        "BTNSIM simulator build OK",
        "BTNSIM",
        MB_OK
    );

    return 0;
}