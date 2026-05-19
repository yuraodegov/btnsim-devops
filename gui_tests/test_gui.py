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


@pytest.fixture(scope="module")
def app():
    """Launch btnsim.exe once for all tests in this module."""
    _app = Application(backend="win32").start(EXE_PATH, wait_for_idle=False)
    time.sleep(1.0)   # wait for window to appear
    yield _app
    _app.kill()


@pytest.fixture(scope="module")
def win(app):
    """Get main window handle."""
    return app.window(title_re=WIN_TITLE)


def get_log(win):
    """Return current text from the event log Edit control."""
    return win.child_window(class_name="Edit").window_text().lower()


# ── тест 1 ──────────────────────────────────────────────────────────────────
def test_window_opens(win):
    """Окно BTNSIM запускается и видимо."""
    assert win.exists(), "window should exist"
    assert win.is_visible(), "window should be visible"


# ── тест 2 ──────────────────────────────────────────────────────────────────
def test_initial_log(win):
    """Лог при старте содержит строку инициализации."""
    log = get_log(win)
    assert "btnsim" in log, f"expected 'btnsim' in log, got: {log[:200]}"


# ── тест 3 ──────────────────────────────────────────────────────────────────
def test_btn1_press_via_keyboard(win):
    """Клавиша '1' генерирует событие press для BTN 1."""
    win.set_focus()
    send_keys("1")
    time.sleep(0.1)
    send_keys("{VK_SPACE}")   # release
    time.sleep(0.5)           # wait for SHORT_CLICK timeout (400ms)

    log = get_log(win)
    assert "btn=1" in log, f"expected btn=1 event in log"
    assert "press" in log,  f"expected press action in log"


# ── тест 4 ──────────────────────────────────────────────────────────────────
def test_short_click_via_keyboard(win):
    """Короткое нажатие клавиши '2' → short_click в логе."""
    win.set_focus()
    send_keys("2")
    time.sleep(0.1)
    send_keys("{VK_SPACE}")
    time.sleep(0.5)   # wait for DOUBLE_CLICK_MS timeout

    log = get_log(win)
    assert "short_click" in log, f"expected short_click in log"


# ── тест 5 ──────────────────────────────────────────────────────────────────
def test_long_press_via_keyboard(win):
    """Удержание клавиши '3' > 800ms → long_press в логе."""
    win.set_focus()
    send_keys("3")
    time.sleep(1.0)   # hold > LONG_PRESS_MS (800ms)
    send_keys("{VK_SPACE}")
    time.sleep(0.1)

    log = get_log(win)
    assert "long_press" in log, f"expected long_press in log"


# ── тест 6 ──────────────────────────────────────────────────────────────────
def test_run_tests_button(win):
    """Кнопка RUN ALL TESTS запускает тесты и показывает результат."""
    # найти кнопку по тексту
    run_btn = win.child_window(title_re=".*RUN ALL TESTS.*", class_name="Button")
    assert run_btn.exists(), "RUN ALL TESTS button should exist"

    run_btn.click()
    time.sleep(0.3)

    log = get_log(win)
    assert "7/7" in log or "passed" in log, \
        f"expected test results in log, got: {log[-300:]}"


# ── тест 7 ──────────────────────────────────────────────────────────────────
def test_clear_log_button(win):
    """Кнопка CLEAR LOG очищает лог."""
    clear_btn = win.child_window(title_re=".*CLEAR.*", class_name="Button")
    assert clear_btn.exists(), "CLEAR LOG button should exist"

    clear_btn.click()
    time.sleep(0.2)

    log = get_log(win)
    # после очистки лог содержит только строку "log cleared"
    assert "log cleared" in log, f"expected 'log cleared' after clear"


# ── тест 8 ──────────────────────────────────────────────────────────────────
def test_status_bar_updates(win):
    """Статусбар обновляется после нажатия кнопки."""
    win.set_focus()
    send_keys("1")
    time.sleep(0.1)

    status = win.child_window(class_name="msctls_statusbar32")
    assert status.exists(), "status bar should exist"

    text = status.window_text().lower()
    assert "btn" in text or "pressed" in text, \
        f"expected status bar update, got: {text}"

    send_keys("{VK_SPACE}")
    time.sleep(0.5)