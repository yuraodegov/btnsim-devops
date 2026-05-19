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
Демонстрирует embedded-стиль разработки с полным DevOps-пайплайном: Docker, GitHub Actions, кросс-компиляция Linux → Windows.

> Собирается на Linux. Запускается на Windows. Тестируется автоматически.

---

## Возможности

| Фича | Описание |
|---|---|
| 🔄 **FSM ядро** | Конечный автомат с состояниями IDLE / PRESSED / HELD / PENDING |
| 🪟 **WinAPI симулятор** | Win32 GUI — 3 кнопки, лог событий, запуск тестов прямо в UI |
| ⌨️ **Клавиатура** | Клавиши `1` `2` `3` — нажать, `Space` — отпустить все |
| 🧪 **Юнит-тесты** | 7 тестов в `test_btn.c` и встроенные тесты в UI |
| 🐳 **Docker** | Воспроизводимая сборка в изолированном контейнере |
| ⚙️ **GitHub Actions** | 3-стадийный пайплайн: анализ → тесты → сборка |
| 🔁 **Кросс-компиляция** | `btnsim.exe` собирается из Linux через `mingw-w64` |
| 📦 **Артефакт** | Готовый `.exe` скачивается из CI без сборки |

---

## Структура проекта

```
btnsim-devops/
├── .github/
│   └── workflows/
│       └── ci.yml              # GitHub Actions: 3 jobs
├── build/
│   ├── btnsim.exe              # Windows binary (CI artifact)
│   └── test_btn                # Linux test runner
├── core/
│   ├── btn_fsm.c               # FSM — единственный источник истины
│   └── btn_fsm.h               # Типы, константы, API
├── simulator/
│   └── btnsim_win32.c          # WinAPI GUI, использует core/btn_fsm.c
├── tests/
│   └── test_btn.c              # CLI unit tests
├── Dockerfile
├── Makefile
└── README.md
```

---

## FSM — архитектура

Кнопка моделируется как детерминированный конечный автомат с 4 состояниями:

```
                PRESS
  IDLE ──────────────────► PRESSED ──── hold >= 800ms ──► HELD
   ▲                          │                             │
   │                          │ release                     │ release
   │                          ▼                             ▼
   │ timeout            PENDING                           IDLE
   │ >= 400ms               │
   └────────────────────────┘
          или второй клик в течение 400ms → DOUBLE_CLICK → IDLE
```

**Состояния:**

| Состояние | Описание |
|---|---|
| `BTN_IDLE` | Кнопка не нажата |
| `BTN_PRESSED` | Кнопка зажата, ждём long press или release |
| `BTN_HELD` | Long press сработал |
| `BTN_PENDING` | Первый клик отпущен, ждём второй (double-click окно 400ms) |

**События:**

| Событие | Условие |
|---|---|
| `EVENT_SHORT_CLICK` | Отпущена, второго клика не было за 400ms |
| `EVENT_DOUBLE_CLICK` | Два клика в пределах 400ms |
| `EVENT_LONG_PRESS` | Удержание >= 800ms |
| `EVENT_LONG_PRESS_RELEASE` | Отпущена после long press |

> `SHORT_CLICK` никогда не возвращается сразу на `btn_release()` —  
> он приходит из `btn_update()` после истечения double-click окна.  
> Это ключевое отличие от наивной реализации.

---

## Быстрый старт

### Docker (рекомендуется)

```bash
git clone https://github.com/yuraodegov/btnsim-devops.git
cd btnsim-devops

docker build -t btnsim .
docker run --rm btnsim make run-tests
```

### Локально (Linux)

```bash
make test       # сборка + компиляция тестов
make run-tests  # запуск тестов
make analyze    # статический анализ (cppcheck)
make win-build  # кросс-компиляция btnsim.exe
make clean      # очистка
```

### Windows

Скачать `btnsim.exe` из **Actions → Artifacts → btnsim-windows-{sha}** и запустить — установка не нужна.

---

## CI/CD пайплайн

```
push / pull_request → main
         │
         ▼
  ┌─────────────┐
  │  1. analyze │  cppcheck --error-exitcode=1
  └──────┬──────┘
         │ success
         ▼
  ┌─────────────┐
  │  2. test    │  make run-tests  (7 тестов)
  └──────┬──────┘
         │ success
         ▼
  ┌───────────────────┐
  │  3. build-windows │  mingw → btnsim.exe → artifact (30 дней)
  └───────────────────┘
```

- Docker image кешируется через `type=gha` — повторные пуши собираются за секунды
- Windows сборка запускается **только** если тесты прошли
- Артефакт именуется `btnsim-windows-{git-sha}` для трассировки

---

## Тесты

```
=====================================
BTNSIM TEST RUNNER
=====================================
[PASS] test_short_click
[PASS] test_long_press
[PASS] test_double_click
[PASS] test_press_no_release
[PASS] test_multi_btn
[PASS] test_boundary
[PASS] test_idle_init
-------------------------------------
RESULT: 7/7 PASSED
=====================================
```

---

## Стек

| | |
|---|---|
| Язык | C99 |
| Сборка | GNU Make + Dockerfile |
| Кросс-компилятор | `x86_64-w64-mingw32-gcc` |
| Статический анализ | `cppcheck` |
| CI | GitHub Actions (`ubuntu-latest`) |
| Целевая ОС | Windows x64 |

---

## Лицензия

MIT — используй свободно, ссылка приветствуется.

---

<div align="center">

Made with 🔧 by [@yuraodegov](https://github.com/yuraodegov)

*Embedded logic. DevOps soul.*

</div>