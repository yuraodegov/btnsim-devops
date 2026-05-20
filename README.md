<div align="center">

```
██████╗ ████████╗███╗   ██╗███████╗██╗███╗   ███╗
██╔══██╗╚══██╔══╝████╗  ██║██╔════╝██║████╗ ████║
██████╔╝   ██║   ██╔██╗ ██║███████╗██║██╔████╔██║
██╔══██╗   ██║   ██║╚██╗██║╚════██║██║██║╚██╔╝██║
██████╔╝   ██║   ██║ ╚████║███████║██║██║ ╚═╝ ██║
╚═════╝    ╚═╝   ╚═╝  ╚═══╝╚══════╝╚═╝╚═╝     ╚═╝
```

# BTNSIM-DEVOPS

**WinAPI Button Hardware Simulator · FSM Architecture · Full CI/CD Pipeline**

[![CI](https://github.com/yuraodegov/btnsim-devops/actions/workflows/ci.yml/badge.svg)](https://github.com/yuraodegov/btnsim-devops/actions/workflows/ci.yml)
[![C](https://img.shields.io/badge/language-C99-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Docker](https://img.shields.io/badge/Docker-enabled-2496ED?logo=docker&logoColor=white)](https://www.docker.com/)
[![Windows](https://img.shields.io/badge/target-Windows%20x64-0078D6?logo=windows&logoColor=white)](https://www.microsoft.com/windows)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](LICENSE)

</div>

---

## Что это?

**BTNSIM-DEVOPS** — симулятор аппаратной кнопки на C с FSM-архитектурой и WinAPI-интерфейсом.  
Демонстрирует embedded-стиль разработки с полным DevOps-пайплайном: Docker, GitHub Actions, кросс-компиляция Linux → Windows, GUI автотесты.

> Собирается на Linux. Запускается на Windows. Тестируется автоматически на обеих платформах.

---

## Возможности

| Фича | Описание |
|---|---|
| 🔄 **FSM ядро** | Конечный автомат: IDLE / PRESSED / HELD / PENDING |
| 🪟 **WinAPI симулятор** | Win32 GUI — 3 кнопки, лог событий, встроенные тесты |
| ⌨️ **Клавиатура** | Клавиши `1` `2` `3` — нажать, `Space` — отпустить все |
| 🧪 **Юнит-тесты** | 7 тестов в `test_btn.c` (Linux, C99) |
| 🖥️ **GUI тесты** | 9 smoke-тестов через pywinauto (Windows CI) |
| 🐳 **Docker** | Воспроизводимая сборка в изолированном контейнере |
| ⚙️ **GitHub Actions** | 4-стадийный пайплайн: анализ → тесты → сборка → GUI тесты |
| 🔁 **Кросс-компиляция** | `btnsim.exe` собирается из Linux через `mingw-w64` |
| 📦 **Артефакт** | Готовый `.exe` после прохождения всех тестов |

---

## Структура проекта

```
btnsim-devops/
├── .github/
│   └── workflows/
│       └── ci.yml              # GitHub Actions: 4 jobs
├── build/
│   ├── btnsim.exe              # Windows binary (CI artifact)
│   └── test_btn                # Linux test runner
├── core/
│   ├── btn_fsm.c               # FSM — единственный источник истины
│   └── btn_fsm.h               # Типы, константы, API
├── gui_tests/
│   ├── test_gui.py             # pywinauto GUI smoke tests (9 тестов)
│   └── requirements.txt        # pywinauto, pytest
├── simulator/
│   └── btnsim_win32.c          # WinAPI GUI, использует core/btn_fsm.c
├── tests/
│   └── test_btn.c              # CLI unit tests (7 тестов)
├── Dockerfile
├── Makefile
└── README.md
```

---

## FSM — архитектура

```
                PRESS
  IDLE ──────────────────► PRESSED ──── hold >= 800ms ──► HELD
   ▲                          │                             │
   │                          │ release                     │ release
   │                          ▼                             ▼
   │ timeout >= 400ms     PENDING                         IDLE
   └──────────────────────────┤
                              │ второй клик < 400ms
                              ▼
                        DOUBLE_CLICK → IDLE
```

| Состояние | Описание |
|---|---|
| `BTN_IDLE` | Кнопка не нажата |
| `BTN_PRESSED` | Зажата, ждём long press или release |
| `BTN_HELD` | Long press сработал |
| `BTN_PENDING` | Первый клик отпущен, ждём второй (400ms окно) |

> `SHORT_CLICK` никогда не возвращается сразу из `btn_release()` —  
> приходит из `btn_update()` после истечения double-click окна.

---

## Быстрый старт

### Docker

```bash
git clone https://github.com/yuraodegov/btnsim-devops.git
cd btnsim-devops

docker build -t btnsim .
docker run --rm btnsim make run-tests
```

### Локально (Linux / Codespaces)

```bash
make run-tests  # запуск unit tests
make analyze    # статический анализ (cppcheck)
make win-build  # кросс-компиляция btnsim.exe
make clean      # очистка
```

### Windows

Скачать `btnsim.exe` из **Actions → Artifacts → btnsim-windows-{sha}** и запустить.

---

## CI/CD пайплайн

```
push / pull_request → main
         │
         ▼
  ┌─────────────┐
  │  1. analyze │  cppcheck (ubuntu-latest)
  └──────┬──────┘
         │ success
         ▼
  ┌─────────────┐
  │  2. test    │  7 unit tests в Docker (ubuntu-latest)
  └──────┬──────┘
         │ success
         ▼
  ┌───────────────────┐
  │  3. build-windows │  mingw cross-compile → btnsim.exe
  └──────┬────────────┘
         │ success
         ▼
  ┌───────────────────┐
  │  4. gui-test      │  9 smoke tests via pywinauto (windows-latest)
  └──────┬────────────┘
         │ success
         ▼
      artifact
  btnsim-windows-{sha}
     (30 дней)
```

- Docker image кешируется через `type=gha`
- Каждый следующий job запускается только если предыдущий прошёл
- `.exe` артефакт передаётся между jobs через `upload-artifact` / `download-artifact`

---

## Тесты

### Unit tests (Linux)

```
=====================================
BTNSIM TEST RUNNER
=====================================
[PASS] test_short_click
[PASS] test_long_press
[PASS] test_double_click
[PASS] test_boundary
[PASS] test_no_double_after_timeout
[PASS] test_long_press_no_repeat
[PASS] test_idle_init
-------------------------------------
RESULT: 7/7 PASSED
=====================================
```

### GUI smoke tests (Windows CI)

```
gui_tests/test_gui.py::test_window_opens          PASSED
gui_tests/test_gui.py::test_initial_log           PASSED
gui_tests/test_gui.py::test_buttons_exist         PASSED
gui_tests/test_gui.py::test_run_tests_button_exists PASSED
gui_tests/test_gui.py::test_clear_log_button_exists PASSED
gui_tests/test_gui.py::test_run_all_tests_passes  PASSED
gui_tests/test_gui.py::test_clear_log             PASSED
gui_tests/test_gui.py::test_status_bar_exists     PASSED
gui_tests/test_gui.py::test_log_edit_exists       PASSED
9 passed in Xs
```

---

## Стек

| | |
|---|---|
| Язык | C99 |
| Сборка | GNU Make + Dockerfile |
| Кросс-компилятор | `x86_64-w64-mingw32-gcc` |
| Статический анализ | `cppcheck` |
| Unit tests | C99, самописный test runner |
| GUI tests | Python 3.12 + pywinauto |
| CI | GitHub Actions (`ubuntu-latest` + `windows-latest`) |
| Целевая ОС | Windows x64 |

---

## Лицензия

MIT — используй свободно, ссылка приветствуется.

---

<div align="center">

Made with 🔧 by [@yuraodegov](https://github.com/yuraodegov)

*Embedded logic. DevOps soul.*

</div>