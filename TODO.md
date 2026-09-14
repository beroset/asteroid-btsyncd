# TODO: C++23/CMake Improvements

## CMake Configuration

### Modern CMake Practices
- [ ] Use `add_library` syntax instead of `add_executable` for consistency
- [ ] Add explicit `CMAKE_CXX_STANDARD 23` in project() call
- [ ] Add `target_compile_options()` for C++23 flags (e.g., `-std=c++2b` if needed)
- [ ] Use `target_link_options()` for linking flags
- [ ] Use `target_include_directories()` with proper PUBLIC/PRIVATE visibility
- [ ] Add `CMakeConfig.cmake.in` for installable packages
- [ ] Add `CMakePackageConfig.cmake.in` for module installation
- [ ] Add `CPack` configuration for source distribution

### Build System Improvements
- [ ] Add `CMakePresets.json` for IDE integration
- [ ] Add `CMakePackageConfigHelpers` for proper package generation
- [ ] Add `install(EXPORT ...)` for proper module installation

## Code Quality

### C++23 Features
- [ ] Replace `#define` constants with `constexpr` (especially UUIDs)
- [ ] Use `nullptr` instead of `0` for null pointers
- [ ] Use `std::string_view` for string comparisons where appropriate
- [ ] Use `if constexpr` for compile-time branching
- [ ] Use `std::optional` where appropriate
- [ ] Use `std::filesystem` for file operations
- [ ] Use `std::string_view` for string parameters
- [ ] Use `std::string_view` for string arguments
- [ ] Use `std::string_view` for string parameters
- [ ] Use `std::string_view` for string arguments

### Code Style
- [ ] Replace `printf`/`fprintf` with Qt's `qInfo`/`qWarning`
- [ ] Use C++23 `std::string_view` for string comparisons
- [ ] Use `std::move` for efficient transfers
- [ ] Use `constexpr` for compile-time constants
- [ ] Add `const` qualifiers where appropriate
- [ ] Use `std::optional` for nullable values

### Documentation
- [ ] Add `README.md` with build/run instructions
- [ ] Add `CONTRIBUTING.md` beyond `HACKING.md`
- [ ] Add `CHANGELOG.md` or release notes
- [ ] Add `docs/` directory for additional documentation

## Codebase Issues

### Architecture
- [ ] Add separate header-only library for `remote/` shared utilities
- [ ] Add `CMakeConfig.cmake.in` for module installation
- [ ] Add `CMakePackageConfig.cmake.in` for proper package generation
- [ ] Add `CPack` configuration for source distribution

### Remote Features
- [ ] Add separate header-only library for `remote/` shared utilities
