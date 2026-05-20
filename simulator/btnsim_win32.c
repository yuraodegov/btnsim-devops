/*
 * BTNSIM - Button Hardware Simulator
 * WinAPI single-file C, uses core/btn_fsm.c for FSM logic
 *
 * Compile (from repo root, MSYS2 MINGW64):
 *   gcc simulator/btnsim_win32.c core/btn_fsm.c -o build/btnsim.exe \
 *       -lcomctl32 -lgdi32 -luser32 -Wall -O2 -Wl,--subsystem,windows
 *
 * Or via Makefile:
 *   make win-build
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/btn_fsm.h"

/* ── constants ─────────────────────────────────────────────────────────── */

#define NUM_BUTTONS     3
#define TIMER_POLL_ID   1
#define TIMER_POLL_MS   16
#define MAX_LOG_BYTES   (300*200)
#define LOG_LINE_MAX    256

#define ID_BTN1         101
#define ID_BTN2         102
#define ID_BTN3         103
#define ID_RUN_TESTS    110
#define ID_CLEAR_LOG    111
#define ID_EXPORT       112
#define ID_LOG_EDIT     120
#define ID_STATUSBAR    130

#define CLR_BG          RGB(10,  10,  15)
#define CLR_SURFACE     RGB(17,  17,  24)
#define CLR_ACCENT      RGB(0,   220, 170)
#define CLR_ACCENT2     RGB(123, 97,  255)
#define CLR_WARN        RGB(255, 209, 102)
#define CLR_DIM         RGB(74,  80,  96)

/* ── statistics ────────────────────────────────────────────────────────── */

typedef struct {
    int clicks;
    int double_clicks;
    int long_presses;
} BtnStats;

static BtnStats g_stats[NUM_BUTTONS];

/* last test run results */
static int g_test_passed = 0;
static int g_test_failed = 0;
static int g_test_ran    = 0;   /* 1 after first RUN */

/* ── globals ───────────────────────────────────────────────────────────── */

static HWND   g_hwnd;
static HWND   g_hlog;
static HWND   g_hstatus;
static HWND   g_hbtn[NUM_BUTTONS];
static HWND   g_hholdlabel[NUM_BUTTONS];
static HWND   g_hstatelabel[NUM_BUTTONS];

static BtnFSM g_fsm[NUM_BUTTONS];

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
static void ExportReport(void);

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

    WNDCLASSEXA wc  = {0};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "BtnSimClass";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExA(&wc);

    for (int i = 0; i < NUM_BUTTONS; i++) {
        btn_init(&g_fsm[i]);
        g_stats[i].clicks        = 0;
        g_stats[i].double_clicks = 0;
        g_stats[i].long_presses  = 0;
    }

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
            x, by, 160, 36, hwnd, (HMENU)ID_RUN_TESTS, hInst, NULL);
        SendMessage(hrun, WM_SETFONT, (WPARAM)g_font_bold, TRUE);

        HWND hclr = CreateWindowA("BUTTON", "X  CLEAR LOG",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            x+170, by, 130, 36, hwnd, (HMENU)ID_CLEAR_LOG, hInst, NULL);
        SendMessage(hclr, WM_SETFONT, (WPARAM)g_font_bold, TRUE);

        HWND hexp = CreateWindowA("BUTTON", "EXPORT REPORT",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            x+310, by, 150, 36, hwnd, (HMENU)ID_EXPORT, hInst, NULL);
        SendMessage(hexp, WM_SETFONT, (WPARAM)g_font_bold, TRUE);

        g_hstatus = CreateWindowA(STATUSCLASSNAME, NULL,
            WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP,
            0,0,0,0, hwnd, (HMENU)ID_STATUSBAR, hInst, NULL);
        SendMessage(g_hstatus, SB_SETTEXTA, 0,
            (LPARAM)"  BTNSIM ready  |  press a button or run tests");

        SetTimer(hwnd, TIMER_POLL_ID, TIMER_POLL_MS, NULL);

        AppendLog("[BTNSIM] simulator started");
        AppendLog("[BTNSIM] FSM: core/btn_fsm.c  |  long=800ms  double=400ms");
        AppendLog("[BTNSIM] -----------------------------------------------------------");
        break;
    }

    case WM_TIMER:
        if (wp == TIMER_POLL_ID) {
            DWORD now = GetTickCount();
            for (int i = 0; i < NUM_BUTTONS; i++) {
                BtnEvent ev = btn_update(&g_fsm[i], now);
                if (ev == EVENT_LONG_PRESS) {
                    g_stats[i].long_presses++;
                    char buf[LOG_LINE_MAX];
                    _snprintf(buf, sizeof(buf),
                        "[EVENT]  btn=%d  action=long_press  held_ms=%lu  state=HELD",
                        i+1, (unsigned long)(now - g_fsm[i].press_tick));
                    AppendLog(buf);
                    SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)"  LONG PRESS detected");
                    InvalidateRect(g_hbtn[i], NULL, TRUE);
                    UpdateStateLabel(i);
                }
                if (ev == EVENT_SHORT_CLICK) {
                    g_stats[i].clicks++;
                    char buf[LOG_LINE_MAX];
                    _snprintf(buf, sizeof(buf),
                        "[EVENT]  btn=%d  action=short_click  state=IDLE", i+1);
                    AppendLog(buf);
                    UpdateStateLabel(i);
                }
            }
            UpdateHoldLabels();
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
        int  pressed = g_fsm[idx].is_pressed;
        int  held    = (g_fsm[idx].state == BTN_HELD);

        COLORREF bgcol = pressed ? CLR_ACCENT : held ? CLR_ACCENT2 : CLR_BG;
        COLORREF fgcol = pressed ? RGB(0,0,0) : CLR_ACCENT;
        COLORREF brcol = held    ? CLR_ACCENT2 : CLR_ACCENT;

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
                    g_fsm[i].state == BTN_HELD     ? CLR_WARN    :
                    g_fsm[i].state == BTN_PRESSED  ? CLR_ACCENT  :
                    g_fsm[i].state == BTN_PENDING  ? CLR_ACCENT2 :
                                                      CLR_DIM);
                return (LRESULT)g_br_bg;
            }
            if (hctl == g_hholdlabel[i]) {
                SetBkColor(hdc, CLR_BG);
                SetTextColor(hdc, g_fsm[i].is_pressed ? CLR_WARN : CLR_DIM);
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
        case ID_RUN_TESTS:  RunAllTests();  break;
        case ID_EXPORT:     ExportReport(); break;
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
                if (g_fsm[i].is_pressed) OnBtnRelease(i);
        }
        break;

    case WM_KEYUP:
        if (wp == '1') OnBtnRelease(0);
        if (wp == '2') OnBtnRelease(1);
        if (wp == '3') OnBtnRelease(2);
        break;

    case WM_ERASEBKGND:
    {
        HDC  hdc = (HDC)wp;
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
        if (g_fsm[idx].is_pressed) OnBtnRelease(idx);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

/* ── FSM event handlers ────────────────────────────────────────────────── */

static void OnBtnPress(int idx)
{
    DWORD now = GetTickCount();
    BtnEvent ev = btn_press(&g_fsm[idx], now);
    if (ev == EVENT_NONE) return;

    char buf[LOG_LINE_MAX];
    _snprintf(buf, sizeof(buf),
        "[EVENT]  btn=%d  action=press  state=%s",
        idx+1, btn_state_str(g_fsm[idx].state));
    AppendLog(buf);

    InvalidateRect(g_hbtn[idx], NULL, TRUE);
    UpdateStateLabel(idx);

    char sb[64];
    _snprintf(sb, sizeof(sb), "  BTN%d  PRESSED", idx+1);
    SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)sb);
}

static void OnBtnRelease(int idx)
{
    DWORD now = GetTickCount();
    BtnEvent ev = btn_release(&g_fsm[idx], now);
    if (ev == EVENT_NONE) {
        UpdateStateLabel(idx);
        InvalidateRect(g_hbtn[idx], NULL, TRUE);
        char sb[64];
        _snprintf(sb, sizeof(sb), "  BTN%d  pending double-click...", idx+1);
        SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)sb);
        return;
    }

    /* update statistics */
    if (ev == EVENT_DOUBLE_CLICK)       g_stats[idx].double_clicks++;
    if (ev == EVENT_LONG_PRESS_RELEASE) { /* already counted in WM_TIMER */ }

    char buf[LOG_LINE_MAX];
    DWORD held = now - g_fsm[idx].press_tick;

    switch (ev) {
    case EVENT_LONG_PRESS_RELEASE:
        _snprintf(buf, sizeof(buf),
            "[EVENT]  btn=%d  action=long_press_release  held_ms=%lu  state=IDLE",
            idx+1, (unsigned long)held);
        break;
    case EVENT_DOUBLE_CLICK:
        _snprintf(buf, sizeof(buf),
            "[EVENT]  btn=%d  action=double_click  state=IDLE", idx+1);
        break;
    default:
        _snprintf(buf, sizeof(buf),
            "[EVENT]  btn=%d  action=%s  state=IDLE",
            idx+1, btn_event_str(ev));
        break;
    }

    AppendLog(buf);
    InvalidateRect(g_hbtn[idx], NULL, TRUE);
    UpdateStateLabel(idx);

    char sb[64];
    _snprintf(sb, sizeof(sb), "  BTN%d  %s", idx+1, btn_event_str(ev));
    SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)sb);
}

/* ── UI helpers ────────────────────────────────────────────────────────── */

static void UpdateStateLabel(int idx)
{
    SetWindowTextA(g_hstatelabel[idx], btn_state_str(g_fsm[idx].state));
    InvalidateRect(g_hstatelabel[idx], NULL, TRUE);
}

static void UpdateHoldLabels(void)
{
    DWORD now = GetTickCount();
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (g_fsm[i].is_pressed) {
            char buf[32];
            _snprintf(buf, sizeof(buf),
                "%lu ms", (unsigned long)(now - g_fsm[i].press_tick));
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
    _snprintf(ts, sizeof(ts), "[%02d:%02d:%02d.%03d]  ",
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

/* ── export report ─────────────────────────────────────────────────────── */

static void ExportReport(void)
{
    /* путь рядом с .exe */
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char *slash = strrchr(path, '\\');
    if (slash) *(slash+1) = 0;
    strncat(path, "btnsim_report.txt", MAX_PATH - strlen(path) - 1);

    FILE *f = fopen(path, "w");
    if (!f) {
        AppendLog("[EXPORT] ERROR: cannot open btnsim_report.txt");
        SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)"  EXPORT FAILED");
        return;
    }

    SYSTEMTIME st;
    GetLocalTime(&st);

    fprintf(f, "===================================================\n");
    fprintf(f, "  BTNSIM SESSION REPORT\n");
    fprintf(f, "===================================================\n");
    fprintf(f, "  Generated: %04d-%02d-%02d  %02d:%02d:%02d\n",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
    fprintf(f, "===================================================\n\n");

    fprintf(f, "--- STATISTICS ---------------------------------\n");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        fprintf(f, "  BTN %d:  clicks=%-4d  double=%-4d  long=%-4d\n",
            i+1,
            g_stats[i].clicks,
            g_stats[i].double_clicks,
            g_stats[i].long_presses);
    }
    fprintf(f, "\n");

    fprintf(f, "--- TEST RESULTS -------------------------------\n");
    if (g_test_ran) {
        fprintf(f, "  %d passed  /  %d failed  /  %d total\n",
            g_test_passed, g_test_failed,
            g_test_passed + g_test_failed);
    } else {
        fprintf(f, "  (tests not run yet)\n");
    }
    fprintf(f, "\n");

    fprintf(f, "--- FULL LOG -----------------------------------\n");
    /* конвертируем \r\n → \n для читаемости */
    const char *p = g_log_buf;
    while (*p) {
        if (*p == '\r') { p++; continue; }
        fputc(*p, f);
        p++;
    }
    fprintf(f, "\n===================================================\n");

    fclose(f);

    char msg[LOG_LINE_MAX];
    _snprintf(msg, sizeof(msg), "[EXPORT] report saved → %s", path);
    AppendLog(msg);

    char sb[MAX_PATH];
    _snprintf(sb, sizeof(sb), "  Exported → btnsim_report.txt");
    SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)sb);
}

/* ── unit tests ─────────────────────────────────────────────────────────── */

typedef struct { const char *name; int passed; char msg[256]; } TR;

#define CHK_INT(a, b, m) \
    if ((a) != (b)) { \
        _snprintf(r->msg, sizeof(r->msg), \
            "want %d got %d -- %s", (int)(b), (int)(a), (m)); \
        r->passed = 0; return; \
    }
#define CHK_EV(ev, expected, m) CHK_INT((int)(ev), (int)(expected), m)

static void test_short_click(TR *r) {
    r->passed = 1; strcpy(r->msg, "OK");
    BtnFSM b; btn_init(&b);
    btn_press(&b, 1000); btn_release(&b, 1100);
    BtnEvent ev = btn_update(&b, 1550);
    CHK_EV(ev, EVENT_SHORT_CLICK, "SHORT_CLICK after timeout");
    CHK_INT(b.state, BTN_IDLE, "IDLE");
}
static void test_long_press(TR *r) {
    r->passed = 1; strcpy(r->msg, "OK");
    BtnFSM b; btn_init(&b);
    btn_press(&b, 2000);
    BtnEvent ev = btn_update(&b, 2900);
    CHK_EV(ev, EVENT_LONG_PRESS, "LONG_PRESS");
    CHK_INT(b.state, BTN_HELD, "HELD");
    ev = btn_release(&b, 2950);
    CHK_EV(ev, EVENT_LONG_PRESS_RELEASE, "LONG_PRESS_RELEASE");
    CHK_INT(b.state, BTN_IDLE, "IDLE after release");
}
static void test_double_click(TR *r) {
    r->passed = 1; strcpy(r->msg, "OK");
    BtnFSM b; btn_init(&b);
    btn_press(&b, 3000); btn_release(&b, 3050);
    btn_press(&b, 3150);
    BtnEvent ev = btn_release(&b, 3200);
    CHK_EV(ev, EVENT_DOUBLE_CLICK, "DOUBLE_CLICK");
    CHK_INT(b.state, BTN_IDLE, "IDLE");
}
static void test_press_no_release(TR *r) {
    r->passed = 1; strcpy(r->msg, "OK");
    BtnFSM b; btn_init(&b);
    btn_press(&b, 5000);
    BtnEvent ev = btn_update(&b, 5100);
    CHK_EV(ev, EVENT_NONE, "no event yet");
    CHK_INT(b.state, BTN_PRESSED, "still PRESSED");
}
static void test_multi_btn(TR *r) {
    r->passed = 1; strcpy(r->msg, "OK");
    BtnFSM b1, b2; btn_init(&b1); btn_init(&b2);
    btn_press(&b1, 6000); btn_press(&b2, 6050);
    btn_release(&b1, 6120); btn_release(&b2, 6180);
    BtnEvent e1 = btn_update(&b1, 6600);
    BtnEvent e2 = btn_update(&b2, 6650);
    CHK_EV(e1, EVENT_SHORT_CLICK, "btn1 SHORT_CLICK");
    CHK_EV(e2, EVENT_SHORT_CLICK, "btn2 SHORT_CLICK");
}
static void test_boundary(TR *r) {
    r->passed = 1; strcpy(r->msg, "OK");
    BtnFSM b; btn_init(&b);
    btn_press(&b, 7000);
    BtnEvent ev = btn_update(&b, 7799);
    CHK_EV(ev, EVENT_NONE, "no LONG_PRESS at 799ms");
    CHK_INT(b.state, BTN_PRESSED, "PRESSED at 799ms");
    ev = btn_update(&b, 7800);
    CHK_EV(ev, EVENT_LONG_PRESS, "LONG_PRESS at 800ms");
}
static void test_idle_init(TR *r) {
    r->passed = 1; strcpy(r->msg, "OK");
    BtnFSM b; btn_init(&b);
    CHK_INT(b.state,      BTN_IDLE, "state IDLE");
    CHK_INT(b.is_pressed, 0,        "not pressed");
    CHK_INT(b.long_fired, 0,        "long_fired=0");
}

static void RunAllTests(void)
{
    AppendLog("");
    AppendLog("[TEST]  ==========================================");
    AppendLog("[TEST]  Running 7 tests  (core/btn_fsm.c)");

    typedef void(*TFn)(TR*);
    struct { const char *name; TFn fn; } tests[] = {
        {"test_short_click",      test_short_click},
        {"test_long_press",       test_long_press},
        {"test_double_click",     test_double_click},
        {"test_press_no_release", test_press_no_release},
        {"test_multi_btn",        test_multi_btn},
        {"test_boundary",         test_boundary},
        {"test_idle_init",        test_idle_init},
    };
    int n = (int)(sizeof(tests)/sizeof(tests[0]));
    int passed = 0, failed = 0;

    for (int i = 0; i < n; i++) {
        TR r = {0}; r.name = tests[i].name;
        tests[i].fn(&r);
        char buf[LOG_LINE_MAX];
        if (r.passed) {
            passed++;
            _snprintf(buf, sizeof(buf), "[TEST]  PASSED  %s", r.name);
        } else {
            failed++;
            _snprintf(buf, sizeof(buf), "[TEST]  FAILED  %s  --  %s",
                r.name, r.msg);
        }
        AppendLog(buf);
    }

    AppendLog("[TEST]  ------------------------------------------");
    char sum[LOG_LINE_MAX];
    _snprintf(sum, sizeof(sum),
        "[TEST]  %d passed  /  %d failed  /  %d total", passed, failed, n);
    AppendLog(sum);
    AppendLog("[TEST]  ==========================================");

    /* сохраняем для экспорта */
    g_test_passed = passed;
    g_test_failed = failed;
    g_test_ran    = 1;

    char sb[64];
    _snprintf(sb, sizeof(sb), "  Tests: %d passed, %d failed", passed, failed);
    SendMessage(g_hstatus, SB_SETTEXTA, 0, (LPARAM)sb);
}