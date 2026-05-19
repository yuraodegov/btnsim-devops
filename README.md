# BTNSIM DEVOPS

BTNSIM DEVOPS is a WinAPI button hardware simulator project
with a complete CI/CD pipeline.

The project demonstrates:

- FSM (Finite State Machine) architecture
- WinAPI simulator
- Unit testing in C
- Dockerized builds
- GitHub Actions CI/CD
- Windows cross-compilation from Linux
- Artifact delivery pipeline

---

# Project Structure

```text
core/
    btn_fsm.c
    btn_fsm.h

simulator/
    btnsim_win32.c

tests/
    test_btn.c

.github/workflows/
    ci.yml
Features
Button FSM engine
Short press detection
Double click detection
Long press detection
Standalone unit tests
Docker build environment
Windows EXE generation
GitHub Actions automation
Build Locally
Linux tests
make
make run-tests
Docker Build
docker build -t btnsim-ci .
Run Tests Inside Docker
docker run --rm btnsim-ci
Build Windows EXE
docker run --rm \
-v $(pwd)/build:/app/build \
btnsim-ci \
make win-build
CI/CD Pipeline

Pipeline stages:

Docker build
Static analysis
Unit tests
Windows cross-build
Artifact upload
Technologies
C
WinAPI
Docker
GitHub Actions
mingw-w64
cppcheck
Author

Yura Odegov
