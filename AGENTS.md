# AGENTS.md - QtAppLauncher

## Building
- `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` then `cmake --build build`
- Output binary at `build/bin/QtAppLauncher`
- Qt 6.8.2 is available on this machine (confirmed)
- CI uses explicit cmake commands per platform (not presets directly); `CMakePresets.json` has per-platform presets as reference
- Tests are opt-in: add `-DBUILD_TESTS=ON`; test sources in `test/`, `tests/`, or `src/*_test.cpp`

## Code Quality
- Format: `clang-format -i include/*.h src/*.cpp`
- Lint: `clang-tidy include/*.h src/*.cpp -- -Iinclude`
- Cppcheck (CI style): `cppcheck --enable=all --suppress=missingIncludeSystem --inline-suppr --std=c++17 -I include src/`
- CI also installs `valgrind` but does not run it by default

## CI/CD
- Workflow: `.github/workflows/ci.yml`
- Triggers: push to `main`/`master`, or any tag matching `v*`
- Builds: Linux (GCC), Linux ARM64, macOS (Clang), Windows (MinGW)
- Release: created only on tagged pushes (`v*`); bundles binary only (no headers/docs)
- Tag name: `v` + version number (e.g. tag `v2.1.0`); commit message is bare version number

## Source Structure
- `src/main.cpp` entrypoint, includes `"mainwindow.h"` (no `include/` prefix)
- Headers in `include/`, sources in `src/`, Qt UI files in `ui/`
- AUTOUIC generates to build dir; sources include generated UI as `"../ui/ui_*.h"`
- `resources.qrc` at project root registers resources (`style.qss`, icons)
- `config.json` loaded at runtime from CWD (not compiled in); app metadata in `include/config.h`
- `VERSION` file at repo root - contains current project version number
- `tools/test-tools/` has standalone test utilities (e.g. `test_icon_generator.cpp`)
- **Translation**: `QuickStart_zh_CN.ts` in root; loaded from `:/i18n/` resource path; compile `.ts` → `.qm` and bundle in qrc to enable
- `IconGenerator` utility class for programmatic icon creation
