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
[![C](https://img.shields.io/badge/language-C-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Docker](https://img.shields.io/badge/Docker-enabled-2496ED?logo=docker&logoColor=white)](https://www.docker.com/)
[![Windows](https://img.shields.io/badge/target-Windows%20x64-0078D6?logo=windows&logoColor=white)](https://www.microsoft.com/windows)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](LICENSE)

</div>

---

## 🧠 What is this?

**BTNSIM-DEVOPS** is a hardware button simulator written in C, built around a **Finite State Machine** architecture with a **WinAPI** backend. It demonstrates how to design embedded-style event-driven logic and ship it through a complete, production-grade DevOps pipeline — all from a Linux CI environment.

> Cross-compiled on Linux. Runs on Windows. Tested automatically. Delivered as artifact.

---

## ⚡ Features

| Feature | Details |
|---|---|
| 🔄 **FSM Core** | Finite State Machine with clean state transitions for button press, hold, release |
| 🪟 **WinAPI Simulator** | Win32-compatible hardware abstraction layer — no real hardware needed |
| 🧪 **Unit Tests** | Full test suite in C with pass/fail reporting via `test_btn.c` |
| 🐳 **Dockerized Builds** | Reproducible cross-compilation in an isolated container |
| ⚙️ **GitHub Actions CI/CD** | Automated build, test, and artifact pipeline on every push |
| 🔁 **Cross-compilation** | Windows `.exe` built from Linux using `mingw-w64` |
| 📦 **Artifact Delivery** | Compiled `btnsim.exe` uploaded as a downloadable CI artifact |

---

## 🗂 Project Structure

```
btnsim-devops/
├── .github/
│   └── workflows/
│       └── ci.yml              # GitHub Actions pipeline
├── build/
│   ├── btnsim.exe              # Compiled Windows binary (CI artifact)
│   └── test_btn                # Test runner binary (Linux)
├── core/
│   ├── btn_fsm.c               # FSM implementation
│   └── btn_fsm.h               # FSM state definitions & API
├── simulator/
│   └── btnsim_win32.c          # WinAPI hardware simulator layer
├── tests/
│   └── test_btn.c              # Unit tests
├── Dockerfile                  # Build environment
├── Makefile                    # Build targets
└── README.md
```

---

## 🔬 FSM Architecture

The button logic is modeled as a deterministic finite state machine:

```
         PRESS                 HOLD_TIMEOUT
  IDLE ─────────► PRESSED ──────────────────► HELD
   ▲                │                          │
   │    RELEASE     │           RELEASE        │
   └────────────────┘◄─────────────────────────┘
```

**States:** `IDLE` → `PRESSED` → `HELD`  
**Events:** `BTN_PRESS`, `BTN_RELEASE`, `BTN_HOLD_TIMEOUT`

The FSM is implemented in `core/btn_fsm.c` and is fully decoupled from the hardware layer, making it portable to any platform.

---

## 🚀 Getting Started

### Prerequisites

- Docker (recommended)
- OR: `gcc`, `make`, `mingw-w64` (for cross-compilation)

### Build with Docker

```bash
# Clone the repo
git clone https://github.com/yuraodegov/btnsim-devops.git
cd btnsim-devops

# Build inside Docker
docker build -t btnsim .
docker run --rm -v $(pwd)/build:/app/build btnsim
```

### Build locally (Linux)

```bash
make all        # Build everything
make test       # Run unit tests
make clean      # Clean build artifacts
```

### Run on Windows

After CI completes, download `btnsim.exe` from **Actions → Artifacts** and run it directly — no install needed.

---

## ⚙️ CI/CD Pipeline

The GitHub Actions workflow triggers on every `push` and `pull_request`:

```
push / PR
    │
    ▼
┌─────────────────────────────┐
│  1. Checkout code           │
│  2. Build Docker image      │
│  3. Compile (cross to Win)  │
│  4. Run unit tests          │
│  5. Upload btnsim.exe       │  ◄─── downloadable artifact
└─────────────────────────────┘
```

Pipeline config: [`.github/workflows/ci.yml`](.github/workflows/ci.yml)

---

## 🧪 Tests

Unit tests cover all major FSM transitions:

```bash
$ make test
[PASS] test_idle_to_pressed
[PASS] test_pressed_to_held
[PASS] test_held_to_idle_on_release
[PASS] test_invalid_event_ignored
...
All tests passed.
```

---

## 🛠 Tech Stack

- **Language:** C (C99)
- **Build:** GNU Make + Dockerfile
- **Cross-compiler:** `x86_64-w64-mingw32-gcc`
- **CI:** GitHub Actions (`ubuntu-latest`)
- **Target OS:** Windows x64

---

## 📄 License

MIT — use freely, credit appreciated.

---

<div align="center">

Made with 🔧 by [@yuraodegov](https://github.com/yuraodegov)

*Embedded logic. DevOps soul.*

</div>