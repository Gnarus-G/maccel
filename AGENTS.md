# Agent Guidelines for maccel Repository

This document outlines the essential commands and style conventions for agentic coding in this repository.

## 1. Project Structure

- `driver/` - Linux kernel module (C) for mouse acceleration
- `cli/` - Rust CLI binary for controlling the driver
- `crates/core/` - Core Rust library shared by CLI and TUI
- `tui/` - Terminal UI component
- `site/` - Documentation website (Astro/TypeScript)
- `udev_rules/` - udev rules for device permissions
- `PKGBUILD` - Arch Linux DKMS package definition
- `install.sh` - Installation script

## 2. Build, Lint, and Test Commands

### Driver (C Kernel Module)
- `make build` - Build the kernel module
- `make build_debug` - Build with debug symbols (`-g -DDEBUG`)
- `make test` - Run driver unit tests
- `make test_debug` - Run tests with debug symbols
- `make install` - Build and load the kernel module
- `make install_debug` - Build debug version and install
- `make reinstall` - Uninstall and reinstall module
- `make uninstall` - Remove module from kernel

### CLI (Rust)
- `cargo build --bin maccel --release` - Build CLI binary
- `make dev_cli` - Run CLI with auto-reload using cargo-watch
- `make install_cli` - Build and install CLI to /usr/local/bin
- `make uninstall_cli` - Remove CLI binary

### udev Rules
- `make udev_install` - Install udev rules
- `make udev_uninstall` - Remove udev rules
- `make udev_trigger` - Reload udev and trigger device discovery

### Tests
- **Run all tests:**
  - `make test` (C driver tests)
  - `cargo test --all` (Rust tests)
- **Run single test:**
  - C: `TEST_NAME=<regex_pattern> make test` (e.g., `TEST_NAME=accel.test.c make test`)
  - Rust: `cargo test <test_name>` (e.g., `cargo test my_specific_test_function`)

### Site (Astro)
- `npx astro check` - Type check and lint
- `npx astro build` - Build for production

### Linting & Formatting
- `cargo fmt --all` - Rust formatting
- `cargo clippy --fix --allow-dirty` - Rust linting with auto-fix
- `astro check` - Astro/TypeScript type checking and linting

## 3. Code Style Guidelines

### General Principles
- Adhere to existing project conventions
- Mimic surrounding code style, structure, and patterns
- Add comments sparingly, focusing on _why_ rather than _what_

### Rust (CLI, Core Library, TUI)
- Follow `rustfmt` and `clippy` conventions
- Use `snake_case` for functions, variables, and module names
- Use `PascalCase` for types, enums, and traits
- **Error Handling:**
  - Use `anyhow::Result<()>` for application-level code (cli/, tui/)
  - Use `thiserror` for library code when custom error types are needed
  - Propagate errors with `?` operator, add context with `.context()`
  - Avoid `unwrap()`, `expect()` in production code
- **Imports:** Group by std, external crates, then internal modules
  ```rust
  use std::path::PathBuf;
  
  use anyhow::Context;
  use clap::Parser;
  
  use maccel_core::{Param, SysFsStore};
  ```
- **Async:** Use `tokio` for async operations when needed

### C (Kernel Module)
- Follow existing patterns in `driver/` directory
- Use `snake_case` for functions and variables
- Use `PascalCase` for types and structs
- **Error Handling:** Return integer error codes (0 for success, negative for error)
- Prefix internal functions with `_` (e.g., `_my_function`)
- Use kernel coding style: tabs for indentation, braces on same line

### TypeScript/Astro (Site)
- Use TypeScript strict mode
- Use Tailwind CSS utility-first approach
- Follow Astro component patterns
- Use `kebab-case` for component props and event handlers

## 4. Naming Conventions

| Language | Functions/Variables | Types/Enums | Files |
|----------|---------------------|-------------|-------|
| Rust     | snake_case          | PascalCase  | snake_case.rs |
| C        | snake_case          | PascalCase  | snake_case.c |
| TypeScript | camelCase         | PascalCase  | kebab-case.ts |

## 5. Version Bumping

Only bump versions when the respective component changes:

- **Driver-only bug fixes:** No version bump needed (install.sh clones from source)
- **CLI changes:** Bump `cli/Cargo.toml` version AND create git tag (triggers release)
- **Driver version bump:** Update `PKGBUILD` pkgver (used by DKMS)

When bumping:
1. Update `PKGBUILD` (pkgver) and/or `cli/Cargo.toml` (version)
2. Create and push tag: `git tag v<x.y.z> && git push origin v<x.y.z>`

## 6. Commit Messages

- Short, descriptive subject line (<50 chars), imperative mood
- Capitalize first letter
- Blank line between subject and optional detailed body (~72 char line wrap)
- No period at subject line end

Examples:
```
Add new acceleration curve algorithm

Implements a cubic bezier curve for smoother acceleration
at high DPI values.
```

## 7. Testing Strategy

### Driver Tests
- Located in `driver/tests/*.test.c`
- Tests are compiled and run via Makefile
- Use `TEST_NAME` to run specific test files

### Rust Tests
- Unit tests in same file as code (`#[cfg(test)] mod tests`)
- Integration tests in `tests/` directory
- Run with `cargo test`

## 8. Dependencies

- **Rust:** See `Cargo.toml` files
- **C:** kernel headers, make, gcc
- **DKMS:** Required for driver installation
- **Site:** Node.js, npm (see `site/package.json`)
