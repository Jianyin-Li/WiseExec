# AGENTS.md - QtAppLauncher

## Building
- `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` then `cmake --build build`
- Output binary at `build/bin/QtAppLauncher`
- **Do not attempt local builds** - this machine lacks Qt
- CI uses explicit cmake commands per platform (not presets directly)

## Code Quality
- Format: `clang-format -i include/*.h src/*.cpp`
- Lint: `clang-tidy include/*.h src/*.cpp -- -Iinclude`
- Cppcheck: `cppcheck --std=c++17 -I include src/`

## CI/CD
- Workflow: `.github/workflows/ci.yml`
- Triggers: push to `main`/`master`, or any tag matching `v*`
- Builds: Linux (GCC), Linux ARM64, macOS (Clang), Windows (MinGW)
- Release: created only on tagged pushes; bundles binary only (no headers/docs)
- Tag name matches the release number (e.g. tag `2.1.0` for commit msg `2.1.0`)

## Commit Conventions
- Release: bare version number (e.g. `2.1.0`)
- Feature/fix: descriptive imperative sentence (e.g. `Add closeEvent and hide parent when opening child`)
- Tag name = same as version number for releases

## Source Structure
- `src/main.cpp` entrypoint, includes `"mainwindow.h"` (no `include/` prefix)
- Headers in `include/`, sources in `src/`, Qt UI files in `ui/`
- AUTOUIC generates to build dir; sources include generated UI as `"../ui/ui_*.h"`
- `config.json` loaded at runtime from CWD (not compiled in)
- `VERSION` file at repo root - contains current project version number
