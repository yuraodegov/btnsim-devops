/*
 * BTNSIM - Button Hardware Simulator
 * WinAPI single-file C, ANSI build
 *
 * Compile (MSYS2 MINGW64 terminal):
 *   gcc btnsim.c -o btnsim.exe -lcomctl32 -lgdi32 -luser32 -Wall -O2 -Wl,--subsystem,windows
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── constants ─────────────────────────────────────────────────────────── */

#define NUM_BUTTONS     3
#define LONG_PRESS_MS   800
#define DOUBLE_CLICK_MS 400
#define TIMER_POLL_ID   1
#define TIMER_POLL_MS   30
#define MAX_LOG_BYTES   (300*200)
#define LOG_LINE_MAX    256

#define ID_BTN1         101
#define ID_BTN2         102
#define ID_BTN3         103
#define ID_RUN_TESTS    110
#define ID_CLEAR_LOG    111
#define ID_LOG_EDIT     120
#define ID_STATUSBAR    130

#define CLR_BG          RGB(10,  10,  15)
#define CLR_SURFACE     RGB(17,  17,  24)
#define CLR_ACCENT      RGB(0,   220, 170)
#define CLR_ACCENT2     RGB(123, 97,  255)
#define CLR_WARN        RGB(255, 209, 102)
#define CLR_DIM         RGB(74,  80,  96)

/* ── FSM ───────────────────────────────────────────────────────────────── */

typedef enum { BTN_IDLE=0, BTN_PRESSED, BTN_HELD } BtnState;

typedef struct {
    BtnState state;
    DWORD    press_tick;
    DWORD    last_release;
    int      click_count;
    int      long_fired;
    int      is_held;
} BtnFSM;

/* ── globals ───────────────────────────────────────────────────────────── */

static HWND   g_hwnd;
static HWND   g_hlog;
static HWND   g_hstatus;
static HWND   g_hbtn[NUM_BUTTONS];
static HWND   g_hholdlabel[NUM_BUTTONS];
static HWND   g_hstatelabel[NUM_BUTTONS];

static BtnFSM g_fsm[NUM_BUTTONS];
static int    g_btn_pressed[NUM_BUTTONS];

static HFONT  g_font_mono;
static HFONT  g_font_bold;
static HFONT  g_font_small;

static HBRUSH g_br_bg;
static HBRUSH g_br_surface;

static char   g_log_buf[MAX_LOG_BYTES];

/* ── forward decls ─────────────────────────────────────────────────────── */

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK BtnSubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
static void AppendLog(const char *line);
static void OnBtnPress(int idx);
static void OnBtnRelease(int idx);
static void UpdateHoldLabels(void);
static void UpdateStateLabel(int idx);
static void RunAllTests(void);

/* ── WinMain ───────────────────────────────────────────────────────────── */

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    (void)hPrev; (void)lpCmd;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    g_br_bg      = CreateSolidBrush(CLR_BG);
    g_br_surface = CreateSolidBrush(CLR_SURFACE);

    g_font_mono  = CreateFontA(16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                    FIXED_PITCH|FF_MODERN,"Consolas");
    g_font_bold  = CreateFontA(14,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                    FIXED_PITCH|FF_MODERN,"Consolas");
    g_font_small = CreateFontA(12,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                    FIXED_PITCH|FF_MODERN,"Consolas");

    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "BtnSimClass";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExA(&wc);

    g_hwnd = CreateWindowExA(0, "BtnSimClass",
        "BTNSIM  //  hardware button simulator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 780, 680,
        NULL, NULL, hInst, NULL);

    ShowWindow(g_hwnd, nShow);
    UpdateWindow(g_hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    DeleteObject(g_font_mono);
    DeleteObject(g_font_bold);
    DeleteObject(g_font_small);
    DeleteObject(g_br_bg);
    DeleteObject(g_br_surface);
    return (int)msg.wParam;
}

/* ── WndProc ───────────────────────────────────────────────────────────── */

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        HINSTANCE hInst = GetModuleHandle(NULL);
        int x = 20, y = 60;

        HWND hl = CreateWindowA("STATIC", "// INPUT CONTROLS",
            WS_CHILD|WS_VISIBLE|SS_LEFT, x,20,400,20, hwnd,NULL,hInst,NULL);
        SendMessage(hl, WM_SETFONT, (WPARAM)g_font_small, TRUE);

        const char *labels[3] = {"BTN 1","BTN 2","BTN 3"};
        for (int i = 0; i < NUM_BUTTONS; i++) {
            int bx = x + i*170;

            g_hbtn[i] = CreateWindowA("BUTTON", labels[i],
                WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
                bx, y, 110, 110,
                hwnd, (HMENU)(UINT_PTR)(ID_BTN1+i), hInst, NULL);
            SendMessage(g_hbtn[i], WM_SETFONT, (WPARAM)g_font_bold, TRUE);
            SetWindowSubclass(g_hbtn[i], BtnSubclassProc, i, (DWORD_PTR)i);

            g_hholdlabel[i] = CreateWindowA("STATIC", "--",
                WS_CHILD|WS_VISIBLE|SS_CENTER,
                bx, y+115, 110, 18, hwnd,NULL,hInst,NULL);
            SendMessage(g_hholdlabel[i], WM_SETFONT, (WPARAM)g_font_small, TRUE);

            g_hstatelabel[i] = CreateWindowA("STATIC", "IDLE",
                WS_CHILD|WS_VISIBLE|SS_CENTER,
                bx, y+135, 110, 18, hwnd,NULL,hInst,NULL);
            SendMessage(g_hstatelabel[i], WM_SETFONT, (WPARAM)g_font_small, TRUE);
        }

        HWND hkb = CreateWindowA("STATIC",
            "Keyboard: hold [1] [2] [3]   |   [Space] release all",
            WS_CHILD|WS_VISIBLE|SS_LEFT,
            x, y+158, 500, 18, hwnd,NULL,hInst,NULL);
        SendMessage(hkb, WM_SETFONT, (WPARAM)g_font_small, TRUE);

        int logY = y+185;

        HWND hl2 = CreateWindowA("STATIC", "// EVENT LOG",
            WS_CHILD|WS_VISIBLE|SS_LEFT, x,logY,400,20, hwnd,NULL,hInst,NULL);
        SendMessage(hl2, WM_SETFONT, (WPARAM)g_font_small, TRUE);

        g_hlog = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD|WS_VISIBLE|WS_VSCROLL|
            ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,
            x, logY+22, 720, 300,
            hwnd, (HMENU)ID_LOG_EDIT, hInst, NULL);
        SendMessage(g_hlog, WM_SETFONT, (WPARAM)g_font_mono, TRUE);

        int by = logY+335;

        HWND hrun = CreateWindowA("BUTTON", ">> RUN ALL TESTS",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            x, by, 180, 36, hwnd, (HMENU)ID_RUN_TESTS, hInst, NULL);
        SendMessage(hrun, WM_SETFONT, (WPARAM)g_font_bold, TRUE);

        HWND hclr = CreateWindowA("BUTTON", "X  CLEAR LOG",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            x+190, by, 140, 36, hwnd, (HMENU)ID_CLEAR_LOG, hInst, NULL);
        SendMessage(hclr, WM_SETFONT, (WPARAM)g_font_bold, TRUE);

        g_hstatus = CreateWindowA(STATUSCLASSNAME, NULL,
            WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP,
            0,0,0,0, hwnd, (HMENU)ID_STATUSBAR, hInst, NULL);
        SendMessage(g_hstatus, SB_SETTEXTA, 0,
            (LPARAM)"  BTNSIM ready  |  press a button or run tests");

        SetTimer(hwnd, TIMER_POLL_ID, TIMER_POLL_MS, NULL);

        AppendLog("[BTNSIM] simulator started");
        AppendLog("[BTNSIM] long_press_threshold: 800ms  |  double_click_window: 400ms");
        AppendLog("[BTNSIM] -----------------------------------------------------------");
        break;
    }

    case WM_KEYDOWN:
{
    switch(wp)
    {
    case '1':
        OnBtnPress(0);
        return 0;

    case '2':
        OnBtnPress(1);
        return 0;

    case '3':
        OnBtnPress(2);
        return 0;
    }

    break;
}

case WM_KEYUP:
{
    switch(wp)
    {
    case '1':
        OnBtnRelease(0);
        return 0;

    case '2':
        OnBtnRelease(1);
        return 0;

    case '3':
        OnBtnRelease(2);
        return 0;

    case VK_SPACE:

        for(int i = 0; i < NUM_BUTTONS; i++)
        {
            OnBtnRelease(i);
        }

        return 0;
    }

    break;
}

    case WM_TIMER:
        if (wp == TIMER_POLL_ID) {
            UpdateHoldLabels();
            DWORD now = GetTickCount();
            for (int i = 0; i < NUM_BUTTONS; i++) {
                if (g_btn_pressed[i] &&
                    g_fsm[i].state == BTN_PRESSED &&
                    !g_fsm[i].long_fired &&
                    (now - g_fsm[i].press_tick) >= LONG_PRESS_MS)
                {
                    g_fsm[i].state      = BTN_HELD;
                    g_fsm[i].long_fired = 1;
                    g_fsm[i].is_held    = 1;
                    char buf[LOG_LINE_MAX];
                    sprintf(buf, "[EVENT]  btn=%d  action=long_press  held_ms=%lu  state=HELD",
                        i+1, (unsigned long)(now - g_fsm[i].press_tick));
                    AppendLog(buf);
                    SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)"  LONG PRESS detected");
                    InvalidateRect(g_hbtn[i], NULL, TRUE);
                    UpdateStateLabel(i);
                }
            }
        }
        break;

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT *di = (DRAWITEMSTRUCT*)lp;
        int idx = -1;
        for (int i = 0; i < NUM_BUTTONS; i++)
            if (di->hwndItem == g_hbtn[i]) { idx = i; break; }
        if (idx < 0) break;

        HDC  hdc     = di->hDC;
        RECT rc      = di->rcItem;
        int  pressed = g_btn_pressed[idx];
        int  held    = g_fsm[idx].is_held;

        COLORREF bgcol = pressed ? CLR_ACCENT : held ? CLR_ACCENT2 : CLR_BG;
        COLORREF fgcol = pressed ? RGB(0,0,0) : CLR_ACCENT;
        COLORREF brcol = held ? CLR_ACCENT2 : CLR_ACCENT;

        SetBkMode(hdc, TRANSPARENT);

        HBRUSH hbr  = CreateSolidBrush(bgcol);
        HPEN   hpen = CreatePen(PS_SOLID, 2, brcol);
        SelectObject(hdc, hbr);
        SelectObject(hdc, hpen);
        Ellipse(hdc, rc.left+2, rc.top+2, rc.right-2, rc.bottom-2);
        DeleteObject(hbr);
        DeleteObject(hpen);

        HPEN hpIn = CreatePen(PS_SOLID, 1, RGB(0,60,45));
        SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
        SelectObject(hdc, hpIn);
        Ellipse(hdc, rc.left+10, rc.top+10, rc.right-10, rc.bottom-10);
        DeleteObject(hpIn);

        const char *lbl[3] = {"BTN 1","BTN 2","BTN 3"};
        SetTextColor(hdc, fgcol);
        SelectObject(hdc, g_font_bold);
        DrawTextA(hdc, lbl[idx], -1, &rc, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        return TRUE;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC  hdc  = (HDC)wp;
        HWND hctl = (HWND)lp;
        for (int i = 0; i < NUM_BUTTONS; i++) {
            if (hctl == g_hstatelabel[i]) {
                SetBkColor(hdc, CLR_BG);
                SetTextColor(hdc,
                    g_fsm[i].state == BTN_HELD    ? CLR_WARN :
                    g_fsm[i].state == BTN_PRESSED ? CLR_ACCENT :
                                                     CLR_DIM);
                return (LRESULT)g_br_bg;
            }
            if (hctl == g_hholdlabel[i]) {
                SetBkColor(hdc, CLR_BG);
                SetTextColor(hdc, g_btn_pressed[i] ? CLR_WARN : CLR_DIM);
                return (LRESULT)g_br_bg;
            }
        }
        SetBkColor(hdc, CLR_BG);
        SetTextColor(hdc, CLR_DIM);
        return (LRESULT)g_br_bg;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wp;
        SetBkColor(hdc, CLR_SURFACE);
        SetTextColor(hdc, CLR_ACCENT);
        return (LRESULT)g_br_surface;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_RUN_TESTS:  RunAllTests(); break;
        case ID_CLEAR_LOG:
            g_log_buf[0] = 0;
            SetWindowTextA(g_hlog, "");
            AppendLog("[BTNSIM] log cleared");
            break;
        }
        break;

    case WM_KEYDOWN:
        if (lp & (1<<30)) break;
        if (wp == '1') OnBtnPress(0);
        if (wp == '2') OnBtnPress(1);
        if (wp == '3') OnBtnPress(2);
        if (wp == VK_SPACE) {
            for (int i = 0; i < NUM_BUTTONS; i++)
                if (g_btn_pressed[i]) OnBtnRelease(i);
        }
        break;

    case WM_KEYUP:
        if (wp == '1') OnBtnRelease(0);
        if (wp == '2') OnBtnRelease(1);
        if (wp == '3') OnBtnRelease(2);
        break;

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wp;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, g_br_bg);
        return 1;
    }

    case WM_SIZE:
        SendMessage(g_hstatus, WM_SIZE, 0, 0);
        break;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_POLL_ID);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* ── button subclass ───────────────────────────────────────────────────── */

LRESULT CALLBACK BtnSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                  UINT_PTR id, DWORD_PTR data)
{
    int idx = (int)data;
    switch (msg) {
    case WM_LBUTTONDOWN: SetCapture(hwnd); OnBtnPress(idx);   return 0;
    case WM_LBUTTONUP:   ReleaseCapture(); OnBtnRelease(idx); return 0;
    case WM_CAPTURECHANGED:
        if (g_btn_pressed[idx]) OnBtnRelease(idx);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

/* ── FSM logic ─────────────────────────────────────────────────────────── */

static void OnBtnPress(int idx)
{
    if (g_btn_pressed[idx]) return;
    g_btn_pressed[idx]    = 1;
    g_fsm[idx].press_tick = GetTickCount();
    g_fsm[idx].long_fired = 0;
    g_fsm[idx].is_held    = 0;
    g_fsm[idx].state      = BTN_PRESSED;

    char buf[LOG_LINE_MAX];
    sprintf(buf, "[EVENT]  btn=%d  action=press  state=PRESSED", idx+1);
    AppendLog(buf);

    InvalidateRect(g_hbtn[idx], NULL, TRUE);
    UpdateStateLabel(idx);

    char sb[64];
    sprintf(sb, "  BTN%d  PRESSED", idx+1);
    SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)sb);
}

static void OnBtnRelease(int idx)
{
    if (!g_btn_pressed[idx]) return;
    g_btn_pressed[idx] = 0;

    DWORD now  = GetTickCount();
    DWORD held = now - g_fsm[idx].press_tick;
    char  buf[LOG_LINE_MAX];

    if (g_fsm[idx].state == BTN_HELD) {
        sprintf(buf, "[EVENT]  btn=%d  action=long_press_release  held_ms=%lu  state=IDLE",
            idx+1, (unsigned long)held);
        AppendLog(buf);
    } else {
        DWORD since = now - g_fsm[idx].last_release;
        if (g_fsm[idx].click_count > 0 && since < DOUBLE_CLICK_MS) {
            g_fsm[idx].click_count = 0;
            sprintf(buf, "[EVENT]  btn=%d  action=double_click  held_ms=%lu  state=IDLE",
                idx+1, (unsigned long)held);
            AppendLog(buf);
        } else {
            g_fsm[idx].click_count = 1;
            sprintf(buf, "[EVENT]  btn=%d  action=short_click  held_ms=%lu  state=IDLE",
                idx+1, (unsigned long)held);
            AppendLog(buf);
        }
    }

    g_fsm[idx].last_release = now;
    g_fsm[idx].state        = BTN_IDLE;
    g_fsm[idx].is_held      = 0;

    InvalidateRect(g_hbtn[idx], NULL, TRUE);
    UpdateStateLabel(idx);

    char sb[64];
    sprintf(sb, "  BTN%d  IDLE  (held %lums)", idx+1, (unsigned long)held);
    SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)sb);
}

/* ── UI helpers ────────────────────────────────────────────────────────── */

static void UpdateStateLabel(int idx)
{
    const char *s =
        g_fsm[idx].state == BTN_HELD    ? "HELD"    :
        g_fsm[idx].state == BTN_PRESSED ? "PRESSED" : "IDLE";
    SetWindowTextA(g_hstatelabel[idx], s);
    InvalidateRect(g_hstatelabel[idx], NULL, TRUE);
}

static void UpdateHoldLabels(void)
{
    DWORD now = GetTickCount();
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (g_btn_pressed[i]) {
            char buf[32];
            sprintf(buf, "%lu ms", (unsigned long)(now - g_fsm[i].press_tick));
            SetWindowTextA(g_hholdlabel[i], buf);
        } else {
            SetWindowTextA(g_hholdlabel[i], "--");
        }
        InvalidateRect(g_hholdlabel[i], NULL, TRUE);
    }
}

static void AppendLog(const char *line)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    char ts[32];
    sprintf(ts, "[%02d:%02d:%02d.%03d]  ",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    char full[LOG_LINE_MAX];
    _snprintf(full, sizeof(full)-1, "%s%s\r\n", ts, line);
    full[sizeof(full)-1] = 0;

    size_t cur = strlen(g_log_buf);
    size_t add = strlen(full);
    size_t cap = sizeof(g_log_buf) - 1;

    if (cur + add >= cap) {
        size_t half = cap / 2;
        memmove(g_log_buf, g_log_buf + half, cap - half);
        g_log_buf[cap - half] = 0;
        cur = strlen(g_log_buf);
    }
    strncat(g_log_buf, full, cap - cur);

    SetWindowTextA(g_hlog, g_log_buf);
    int lines = (int)SendMessage(g_hlog, EM_GETLINECOUNT, 0, 0);
    SendMessage(g_hlog, EM_LINESCROLL, 0, lines);
}

/* ── unit tests ────────────────────────────────────────────────────────── */

typedef struct {
    BtnFSM fsm;
    int    pressed;
    char   last_action[64];
} TestCtx;

static void t_press(TestCtx *c, DWORD tick) {
    c->pressed=1; c->fsm.press_tick=tick;
    c->fsm.long_fired=0; c->fsm.is_held=0;
    c->fsm.state=BTN_PRESSED;
    strcpy(c->last_action,"press");
}
static void t_advance(TestCtx *c, DWORD tick) {
    if (c->pressed && c->fsm.state==BTN_PRESSED && !c->fsm.long_fired &&
        (tick-c->fsm.press_tick)>=LONG_PRESS_MS) {
        c->fsm.state=BTN_HELD; c->fsm.long_fired=1; c->fsm.is_held=1;
        strcpy(c->last_action,"long_press");
    }
}
static void t_release(TestCtx *c, DWORD tick) {
    if (!c->pressed) return;
    c->pressed=0;
    if (c->fsm.state==BTN_HELD) {
        strcpy(c->last_action,"long_press_release");
    } else {
        DWORD since=tick-c->fsm.last_release;
        if (c->fsm.click_count>0 && since<DOUBLE_CLICK_MS) {
            c->fsm.click_count=0;
            strcpy(c->last_action,"double_click");
        } else {
            c->fsm.click_count=1;
            strcpy(c->last_action,"short_click");
        }
    }
    c->fsm.last_release=tick; c->fsm.state=BTN_IDLE; c->fsm.is_held=0;
}

typedef struct { const char *name; int passed; char msg[256]; } TR;

#define CHK_STR(a,b,m) \
    if(strcmp(a,b)!=0){_snprintf(r->msg,sizeof(r->msg),"want '%s' got '%s' — %s",b,a,m);r->passed=0;return;}
#define CHK_STATE(c,e,m) \
    if((c).fsm.state!=(e)){_snprintf(r->msg,sizeof(r->msg),"state want %d got %d — %s",e,(c).fsm.state,m);r->passed=0;return;}

static void test_short_click(TR *r){
    r->passed=1; strcpy(r->msg,"OK"); TestCtx c={0};
    t_press(&c,1000); t_advance(&c,1100); t_release(&c,1150);
    CHK_STR(c.last_action,"short_click","action");
    CHK_STATE(c,BTN_IDLE,"state");
}
static void test_long_press(TR *r){
    r->passed=1; strcpy(r->msg,"OK"); TestCtx c={0};
    t_press(&c,2000); t_advance(&c,2900);
    CHK_STR(c.last_action,"long_press","long fired");
    CHK_STATE(c,BTN_HELD,"held");
    t_release(&c,2950);
    CHK_STR(c.last_action,"long_press_release","release");
    CHK_STATE(c,BTN_IDLE,"idle");
}
static void test_double_click(TR *r){
    r->passed=1; strcpy(r->msg,"OK"); TestCtx c={0};
    t_press(&c,3000); t_release(&c,3080);
    t_press(&c,3200); t_release(&c,3260);
    CHK_STR(c.last_action,"double_click","double");
}
static void test_press_no_release(TR *r){
    r->passed=1; strcpy(r->msg,"OK"); TestCtx c={0};
    t_press(&c,5000); t_advance(&c,5100);
    CHK_STATE(c,BTN_PRESSED,"still pressed");
}
static void test_multi_btn(TR *r){
    r->passed=1; strcpy(r->msg,"OK");
    TestCtx c1={0},c2={0};
    t_press(&c1,6000); t_press(&c2,6050);
    t_release(&c1,6120); t_release(&c2,6180);
    CHK_STR(c1.last_action,"short_click","btn1");
    CHK_STR(c2.last_action,"short_click","btn2");
}
static void test_boundary(TR *r){
    r->passed=1; strcpy(r->msg,"OK");
    TestCtx c={0};
    t_press(&c,7000); t_advance(&c,7799);
    if(c.fsm.state==BTN_HELD){strcpy(r->msg,"long fired too early at 799ms");r->passed=0;return;}
    t_release(&c,7799);
    CHK_STR(c.last_action,"short_click","799ms=short");
    TestCtx c2={0};
    t_press(&c2,8000); t_advance(&c2,8801);
    CHK_STR(c2.last_action,"long_press","801ms=long");
}
static void test_idle_init(TR *r){
    r->passed=1; strcpy(r->msg,"OK");
    TestCtx c={0};
    CHK_STATE(c,BTN_IDLE,"initial");
}

static void RunAllTests(void)
{
    AppendLog("");
    AppendLog("[TEST]  ==========================================");
    AppendLog("[TEST]  Running 7 tests...");

    typedef void(*TFn)(TR*);
    struct { const char *name; TFn fn; } tests[] = {
        {"test_short_click",          test_short_click},
        {"test_long_press",           test_long_press},
        {"test_double_click",         test_double_click},
        {"test_press_no_release",     test_press_no_release},
        {"test_multi_btn",            test_multi_btn},
        {"test_boundary",             test_boundary},
        {"test_idle_init",            test_idle_init},
    };
    int n = (int)(sizeof(tests)/sizeof(tests[0]));
    int passed=0, failed=0;

    for (int i = 0; i < n; i++) {
        TR r = {0}; r.name = tests[i].name;
        tests[i].fn(&r);
        char buf[LOG_LINE_MAX];
        if (r.passed) {
            passed++;
            _snprintf(buf,sizeof(buf),"[TEST]  PASSED  %s", r.name);
        } else {
            failed++;
            _snprintf(buf,sizeof(buf),"[TEST]  FAILED  %s  --  %s", r.name, r.msg);
        }
        AppendLog(buf);
    }

    AppendLog("[TEST]  ------------------------------------------");
    char sum[LOG_LINE_MAX];
    _snprintf(sum, sizeof(sum), "[TEST]  %d passed  /  %d failed  /  %d total",
        passed, failed, n);
    AppendLog(sum);
    AppendLog("[TEST]  ==========================================");

    char sb[64];
    _snprintf(sb, sizeof(sb), "  Tests: %d passed, %d failed", passed, failed);
    SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)sb);
}