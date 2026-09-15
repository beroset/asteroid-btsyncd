# Improvements

## Code Quality

### C++23 Features
- [X] Use `nullptr` instead of `0` for null pointers
- [X] Replace `#define` constants with `constexpr` (especially UUIDs)
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
