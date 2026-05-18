#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "../core/btn_fsm.h"

#define NUM_BUTTONS 3

#define BTN1_ID 1001
#define BTN2_ID 1002
#define BTN3_ID 1003

static BtnFSM g_fsm[NUM_BUTTONS];

static HWND g_buttons[NUM_BUTTONS];

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch(msg)
    {
        case WM_COMMAND:
        {
            int id = LOWORD(wParam);

            int idx = -1;

            if(id == BTN1_ID) idx = 0;
            if(id == BTN2_ID) idx = 1;
            if(id == BTN3_ID) idx = 2;

            if(idx >= 0)
            {
                BtnEvent ev =
                    btn_press(&g_fsm[idx],
                              GetTickCount());

                char buf[128];

                sprintf(
                    buf,
                    "BTN%d -> %s",
                    idx + 1,
                    btn_event_str(ev));

                MessageBoxA(
                    hwnd,
                    buf,
                    "FSM EVENT",
                    MB_OK);

                btn_release(
                    &g_fsm[idx],
                    GetTickCount());
            }

            break;
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(
        hwnd,
        msg,
        wParam,
        lParam);
}

int WINAPI WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    LPSTR lpCmd,
    int nShow)
{
    (void)hPrev;
    (void)lpCmd;

    for(int i = 0; i < NUM_BUTTONS; i++) {
        btn_init(&g_fsm[i]);
    }

    WNDCLASSA wc = {0};

    wc.lpfnWndProc = WndProc;
    wc.hInstance   = hInst;
    wc.lpszClassName = "BTNSIM_CLASS";

    RegisterClassA(&wc);

    HWND hwnd =
        CreateWindowExA(
            0,
            "BTNSIM_CLASS",
            "BTNSIM DEVOPS",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            100,
            100,
            500,
            300,
            NULL,
            NULL,
            hInst,
            NULL);

    g_buttons[0] =
        CreateWindowA(
            "BUTTON",
            "BUTTON 1",
            WS_VISIBLE | WS_CHILD,
            50,
            50,
            120,
            50,
            hwnd,
            (HMENU)BTN1_ID,
            hInst,
            NULL);

    g_buttons[1] =
        CreateWindowA(
            "BUTTON",
            "BUTTON 2",
            WS_VISIBLE | WS_CHILD,
            190,
            50,
            120,
            50,
            hwnd,
            (HMENU)BTN2_ID,
            hInst,
            NULL);

    g_buttons[2] =
        CreateWindowA(
            "BUTTON",
            "BUTTON 3",
            WS_VISIBLE | WS_CHILD,
            330,
            50,
            120,
            50,
            hwnd,
            (HMENU)BTN3_ID,
            hInst,
            NULL);

    ShowWindow(hwnd, nShow);

    MSG msg;

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}