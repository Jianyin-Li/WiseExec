# AGENTS.md - QtAppLauncher

## Building
- Use CMake presets: `cmake --preset <name>` then `cmake --build --preset <name>`
- Presets: `default`, `msvc`, `mingw`, `linux`, `macos`
- Outputs go to `build/<preset>/bin/`
- **Do not attempt local builds** - this machine lacks Qt
- CI now uses explicit `cmake -B build/<platform> -G <generator>` (not presets directly)

## Testing
- Tests disabled in all presets (`BUILD_TESTS: OFF`)
- Enable with `-DBUILD_TESTS=ON` if needed

## Code Quality
- Tools: `clang-tidy`, `cppcheck`, `valgrind`
- Linux CI runs: `cppcheck --std=c++17 -I include`
- Format: `clang-format -i include/*.h src/*.cpp`

## CI/CD
- GitHub Actions workflow at `.github/workflows/ci.yml`
- **macOS (Clang)**: ✅ Working
- **Linux (GCC)**: ✅ Working
- **Windows (MinGW)**: ❌ GCC 15 / Qt ABI incompatibility (`_Float16` ODR violation)
- All builds are `continue-on-error: true` - failures don't block other jobs
- Uses `aqtinstall` to fetch Qt on Windows; system pkg mgr on Linux/macOS
- macOS needs `BUNDLE DESTINATION` in install() rules (macOS bundles)
- CMAKE_INSTALL_*DIR vars must use `FORCE` (cmake 4.x behavior)

## Tech Stack
- CMake 3.16+, C++17
- Qt 5.15 or Qt 6.x (auto-detected)
- Uses AUTOUIC/AUTOMOC/AUTORCC