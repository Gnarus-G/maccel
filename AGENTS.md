# Agent Guidelines for maccel Repository

This document outlines the essential commands and style conventions for agentic coding in this repository.

## 1. Project Structure

- `driver/` - Linux kernel module (C) for mouse acceleration
- `cli/` - Rust CLI binary for controlling the driver
- `crates/core/` - Core Rust library shared by CLI and TUI
- `tui/` - Terminal UI component using ratatui
- `site/` - Documentation website (Astro/TypeScript)
- `udev_rules/` - udev rules for device permissions
- `PKGBUILD` - Arch Linux DKMS package definition
- `install.sh` - Installation script

## 2. Build, Lint, and Test Commands

### Driver (C Kernel Module)
- `make build` - Build the kernel module
- `make build_debug` - Build with debug symbols (`-g -DDEBUG`)
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

#### Run All Tests
- `make test` - Run all C driver tests
- `cargo test --all` - Run all Rust tests

#### Run Single Test

**C Driver Tests:**
- `TEST_NAME=<pattern> make test` - Filter tests by filename pattern
- Examples:
  - `TEST_NAME=accel.test.c make test` - Run only acceleration tests
  - `TEST_NAME=input_speed make test` - Run input speed tests

**Rust Tests:**
- `cargo test <test_name>` - Run tests matching name
- `cargo test --package maccel-core <test_name>` - Run tests in specific crate
- Examples:
  - `cargo test format_param_value_works` - Run specific test function
  - `cargo test --package maccel-core` - Run only core library tests

### Site (Astro/TypeScript)
- `cd site && npx astro check` - Type check and lint
- `cd site && npx astro build` - Build for production
- `cd site && npx astro dev` - Development server

### Linting & Formatting
- `cargo fmt --all` - Format all Rust code
- `cargo clippy --fix --allow-dirty` - Rust linting with auto-fix
- `cargo clippy --all` - Check all Rust code without auto-fix

## 3. Code Style Guidelines

### General Principles
- Adhere to existing project conventions
- Mimic surrounding code style, structure, and patterns
- Add comments sparingly, focusing on _why_ rather than _what_
- No trailing whitespace

### Rust (CLI, Core Library, TUI)

**Naming:**
- `snake_case` for functions, variables, and module names
- `PascalCase` for types, enums, and traits
- `SCREAMING_SNAKE_CASE` for constants

**Imports (order matters):**
1. `std` imports first
2. External crates second
3. Internal modules third
4. Within each group, sort alphabetically

```rust
use std::{
    fmt::{Debug, Display},
    path::PathBuf,
};

use anyhow::{anyhow, Context};
use clap::Parser;

use crate::{
    fixedptc::Fpt,
    params::{AccelMode, Param},
};
```

**Error Handling:**
- Use `anyhow::Result<()>` for application-level code (cli/, tui/)
- Use `thiserror` for library code when custom error types are needed
- Propagate errors with `?` operator
- Add context with `.context()` for better error messages
- Avoid `unwrap()` and `expect()` in production code
- Use `anyhow::bail!()` for early returns with errors

**Async:**
- Use `tokio` for async operations when needed

**Testing:**
- Unit tests in same file as code: `#[cfg(test)] mod tests { ... }`
- Use `#[test]` attribute for test functions

### C (Kernel Module)

**Naming:**
- `snake_case` for functions and variables
- `PascalCase` for types and structs

**Style:**
- Follow Linux kernel coding style
- Tabs for indentation
- Braces on same line (K&R style)
- Prefix internal/static functions with `_`

**Error Handling:**
- Return integer error codes
- 0 for success, negative for error
- Use `goto` for cleanup on error (common kernel pattern)

**Testing:**
- Test files: `driver/tests/*.test.c`
- Use snapshot testing via `assert_snapshot()` function
- Run with `make test` or `TEST_NAME=<pattern> make test`

### TypeScript/Astro (Site)

- Use TypeScript strict mode
- Use Tailwind CSS utility-first approach
- Follow Astro component patterns
- `kebab-case` for component filenames
- `camelCase` for variables and functions in scripts

## 4. Naming Conventions Summary

| Language | Functions/Variables | Types/Enums | Files | Constants |
|----------|---------------------|-------------|-------|-----------|
| Rust     | snake_case          | PascalCase  | snake_case.rs | SCREAMING_SNAKE_CASE |
| C        | snake_case          | PascalCase  | snake_case.c | SCREAMING_SNAKE_CASE |
| TypeScript | camelCase         | PascalCase  | kebab-case.ts | SCREAMING_SNAKE_CASE |

## 5. Version Bumping

Only bump versions when the respective component changes:

- **Driver-only bug fixes:** No version bump needed (install.sh clones from source)
- **CLI changes:** Bump `cli/Cargo.toml` version AND create git tag (triggers release)
- **Driver version bump:** Update `PKGBUILD` pkgver (used by DKMS)

When bumping CLI version:
1. Update `cli/Cargo.toml` version field
2. Create and push tag: `git tag v<x.y.z> && git push origin v<x.y.z>`

## 6. Commit Messages

- Short, descriptive subject line (<50 chars), imperative mood
- Capitalize first letter
- Blank line between subject and optional body (~72 char line wrap)
- No period at subject line end

Examples:
```
Add new acceleration curve algorithm

Implements a cubic bezier curve for smoother acceleration
at high DPI values.
```

## 7. Key Files Reference

| Purpose | File |
|---------|------|
| CLI entry point | `cli/src/main.rs` |
| Core library exports | `crates/core/src/lib.rs` |
| Parameter definitions | `crates/core/src/params.rs` |
| SysFS persistence | `crates/core/src/persist.rs` |
| Driver entry point | `driver/maccel.c` |
| Test utilities | `driver/tests/test_utils.h` |
| Workspace dependencies | `Cargo.toml` |

## 8. Dependencies

- **Rust:** See `Cargo.toml` (workspace) and individual `Cargo.toml` files
- **C:** kernel headers, make, gcc
- **DKMS:** Required for driver installation
- **Site:** Node.js 24.x (`^24.0.0`), npm (see `site/package.json`)