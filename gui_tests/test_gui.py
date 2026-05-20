"""
GUI Automation Tests for BTNSIM
Runs on Windows, requires btnsim.exe in build/

Run:
    pytest gui_tests/test_gui.py -v
"""

import time
import pytest
from pywinauto import Application
from pywinauto.keyboard import send_keys

EXE_PATH = r"build\btnsim.exe"
WIN_TITLE = "BTNSIM"


def focus(win):
    """Bring window to front and wait for it to be ready."""
    win.restore()
    win.set_focus()
    time.sleep(0.3)


@pytest.fixture(scope="module")
def app():
    _app = Application(backend="win32").start(EXE_PATH, wait_for_idle=False)
    time.sleep(1.5)
    yield _app
    _app.kill()


@pytest.fixture(scope="module")
def win(app):
    return app.window(title_re=WIN_TITLE)


def get_log(win):
    return win.child_window(class_name="Edit").window_text().lower()


# ── тест 1 ──────────────────────────────────────────────────────────────────
def test_window_opens(win):
    assert win.exists(),     "window should exist"
    assert win.is_visible(), "window should be visible"


# ── тест 2 ──────────────────────────────────────────────────────────────────
def test_initial_log(win):
    log = get_log(win)
    assert "btnsim" in log, f"expected 'btnsim' in log"


# ── тест 3 ──────────────────────────────────────────────────────────────────
def test_btn1_press_via_keyboard(win):
    focus(win)
    win.type_keys("1")
    time.sleep(0.2)
    win.type_keys(" ")       # space = release all
    time.sleep(0.6)          # wait for SHORT_CLICK timeout

    log = get_log(win)
    assert "btn=1" in log,  f"expected btn=1 in log, got:\n{log[-400:]}"
    assert "press" in log,  f"expected press in log"


# ── тест 4 ──────────────────────────────────────────────────────────────────
def test_short_click_via_keyboard(win):
    focus(win)
    win.type_keys("2")
    time.sleep(0.1)
    win.type_keys(" ")
    time.sleep(0.6)

    log = get_log(win)
    assert "short_click" in log, f"expected short_click in log, got:\n{log[-400:]}"


# ── тест 5 ──────────────────────────────────────────────────────────────────
def test_long_press_via_keyboard(win):
    focus(win)
    win.type_keys("3")
    time.sleep(1.1)          # hold > 800ms
    win.type_keys(" ")
    time.sleep(0.2)

    log = get_log(win)
    assert "long_press" in log, f"expected long_press in log, got:\n{log[-400:]}"


# ── тест 6 ──────────────────────────────────────────────────────────────────
def test_run_tests_button(win):
    run_btn = win.child_window(title_re=".*RUN ALL TESTS.*", class_name="Button")
    assert run_btn.exists(), "RUN ALL TESTS button should exist"

    run_btn.click()
    time.sleep(0.5)

    log = get_log(win)
    assert "7/7" in log or ("passed" in log and "failed" in log), \
        f"expected test results in log, got:\n{log[-400:]}"


# ── тест 7 ──────────────────────────────────────────────────────────────────
def test_clear_log_button(win):
    clear_btn = win.child_window(title_re=".*CLEAR.*", class_name="Button")
    assert clear_btn.exists(), "CLEAR LOG button should exist"

    clear_btn.click()
    time.sleep(0.3)

    log = get_log(win)
    assert "log cleared" in log, f"expected 'log cleared' after clear"


# ── тест 8 ──────────────────────────────────────────────────────────────────
def test_status_bar_updates(win):
    # сначала очистим статусбар нажав кнопку
    focus(win)
    win.type_keys("1")
    time.sleep(0.2)

    status = win.child_window(class_name="msctls_statusbar32")
    assert status.exists(), "status bar should exist"

    text = status.window_text().lower()
    assert "btn" in text or "pressed" in text or "pending" in text, \
        f"expected status update after press, got: '{text}'"

    win.type_keys(" ")
    time.sleep(0.6)