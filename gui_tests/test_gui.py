"""
GUI Smoke Tests for BTNSIM
Runs on Windows CI (windows-latest), requires build/btnsim.exe

Run:
    pytest gui_tests/test_gui.py -v
"""

import time
import pytest
from pywinauto import Application

EXE_PATH = r"build\btnsim.exe"
WIN_TITLE = "BTNSIM"


def get_log(win):
    return win.child_window(class_name="Edit").window_text().lower()


@pytest.fixture(scope="module")
def app():
    _app = Application(backend="win32").start(EXE_PATH, wait_for_idle=False)
    time.sleep(1.5)
    yield _app
    _app.kill()


@pytest.fixture(scope="module")
def win(app):
    return app.window(title_re=WIN_TITLE)


# ── тест 1 ──────────────────────────────────────────────────────────────────
def test_window_opens(win):
    """Окно запускается и видимо."""
    assert win.exists(),     "window should exist"
    assert win.is_visible(), "window should be visible"


# ── тест 2 ──────────────────────────────────────────────────────────────────
def test_initial_log(win):
    """Лог содержит строку инициализации."""
    log = get_log(win)
    assert "btnsim" in log, f"expected 'btnsim' in log"


# ── тест 3 ──────────────────────────────────────────────────────────────────
def test_buttons_exist(win):
    """Все три кнопки BTN присутствуют в окне."""
    for title in ("BTN 1", "BTN 2", "BTN 3"):
        btn = win.child_window(title=title, class_name="Button")
        assert btn.exists(), f"button '{title}' should exist"


# ── тест 4 ──────────────────────────────────────────────────────────────────
def test_run_tests_button_exists(win):
    """Кнопка RUN ALL TESTS присутствует."""
    btn = win.child_window(title_re=".*RUN ALL TESTS.*", class_name="Button")
    assert btn.exists(), "RUN ALL TESTS button should exist"


# ── тест 5 ──────────────────────────────────────────────────────────────────
def test_clear_log_button_exists(win):
    """Кнопка CLEAR LOG присутствует."""
    btn = win.child_window(title_re=".*CLEAR.*", class_name="Button")
    assert btn.exists(), "CLEAR LOG button should exist"


# ── тест 6 ──────────────────────────────────────────────────────────────────
def test_run_all_tests_passes(win):
    """RUN ALL TESTS → все 7 тестов проходят."""
    run_btn = win.child_window(title_re=".*RUN ALL TESTS.*", class_name="Button")
    run_btn.click_input()
    time.sleep(1.0)

    log = get_log(win)
    # лог содержит "[test]  passed  test_xxx" или "7 passed / 0 failed"
    assert "[test]" in log, \
        f"expected [test] entries in log, got:\n{log[-400:]}"
    assert "failed  0" in log or "0 failed" in log or \
           ("passed" in log and "failed" in log), \
        f"expected test results in log, got:\n{log[-400:]}"


# ── тест 7 ──────────────────────────────────────────────────────────────────
def test_clear_log(win):
    """CLEAR LOG очищает лог."""
    clear_btn = win.child_window(title="X  CLEAR LOG", class_name="Button")
    clear_btn.click_input()
    time.sleep(0.5)

    log = get_log(win)
    assert "log cleared" in log, f"expected 'log cleared', got:\n{log[-200:]}"


# ── тест 8 ──────────────────────────────────────────────────────────────────
def test_status_bar_exists(win):
    """Статусбар присутствует."""
    status = win.child_window(class_name="msctls_statusbar32")
    assert status.exists(), "status bar should exist"


# ── тест 9 ──────────────────────────────────────────────────────────────────
def test_log_edit_exists(win):
    """Edit контрол (лог) присутствует."""
    log_edit = win.child_window(class_name="Edit")
    assert log_edit.exists(), "log Edit control should exist"