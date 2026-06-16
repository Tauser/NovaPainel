---
name: novapanel-esp-idf-build-debug
description: Debug NovaPanel ESP-IDF setup, VS Code ESP-IDF extension, target selection, CMakeLists errors, OpenOCD configuration, build, flash, monitor, and partition/build failures. Use when the user reports IDF, VS Code, target, OpenOCD, idf.py, or build issues.
---

# NovaPanel ESP-IDF Build Debug

Use this workflow for build/setup/debug issues.

## Known-good baseline

- Open the ESP-IDF project folder: `D:\Projetos\NovaPanel\firmware`
- Do not open `D:\Projetos\NovaPanel` as the ESP-IDF project folder.
- Target: `esp32p4`
- ESP-IDF: `v5.5.4`
- IDF path: `C:\esp\v5.5.4\esp-idf`
- Python env: `C:\Espressif\tools\python\v5.5.4\venv`
- Debug config: `ESP32-P4 chip (via builtin USB-JTAG)` unless using ESP-PROG hardware.

## Commands

From `firmware/`:

```bat
idf.py --version
idf.py set-target esp32p4
idf.py build
```

Full known-good build command:

```bat
cmd.exe /c "set IDF_PYTHON_ENV_PATH=C:\Espressif\tools\python\v5.5.4\venv&& call C:\esp\v5.5.4\esp-idf\export.bat && idf.py build"
```

## Common diagnoses

- `CMakeLists.txt not found in project directory`: VS Code is pointed at repo root instead of `firmware/`.
- Target shows `esp32` or another SoC: run `idf.py set-target esp32p4`.
- User asks about ESP32-C6 target: keep target `esp32p4`; C6 is the network coprocessor.
- Partition size error: check `firmware/partitions.csv` and flash size in `sdkconfig.defaults`.
- OpenOCD config question: choose builtin USB-JTAG for the board unless the user connected ESP-PROG.

## Validation report

Always report:

- folder used for the command
- target detected by CMake
- ESP-IDF version
- success/failure summary
- first real error line if build failed
