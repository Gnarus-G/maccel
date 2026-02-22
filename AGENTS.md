# Agent Guidelines for maccel Repository

This document outlines essential commands and style conventions for agentic coding in this repository.

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
**All Tests:**
- `make test` - Run all C driver tests
- `cargo test --all` - Run all Rust tests

**Single Test:**
- `TEST_NAME=<pattern> make test` - Filter C tests by filename
- `cargo test <test_name>` - Run Rust test by name
- `cargo test --package maccel-core <test_name>` - Run test in specific crate

### Linting & Formatting
- `cargo fmt --all` - Format all Rust code
- `cargo clippy --all --fix --allow-dirty` - Rust linting with auto-fix

## 3. Code Style Guidelines

### General Principles
- Adhere to existing project conventions
- Mimic surrounding code style and patterns
- Add comments sparingly, focusing on _why_ rather than _what_
- No trailing whitespace

### Rust (CLI, Core, TUI)

**Naming:**
- `snake_case` - functions, variables, modules
- `PascalCase` - types, enums, traits
- `SCREAMING_SNAKE_CASE` - constants

**Imports (order matters):**
1. `std` imports first
2. External crates second
3. Internal modules third
4. Within each group, sort alphabetically

```rust
use std::{fmt::Debug, path::PathBuf};
use anyhow::Context;
use crate::params::Param;
```

**Error Handling:**
- Use `anyhow::Result<()>` for application-level code
- Use `thiserror` for library code when needed
- Propagate errors with `?` operator
- Add context with `.context()` for better messages
- Avoid `unwrap()` and `expect()` in production code
- Use `anyhow::bail!()` for early error returns

**Testing:**
- Unit tests in same file: `#[cfg(test)] mod tests { ... }`
- Use `#[test]` attribute for test functions

### C (Kernel Module)

**Naming:**
- `snake_case` - functions and variables
- `PascalCase` - types and structs

**Style:**
- Follow Linux kernel coding style
- Tabs for indentation
- Braces on same line (K&R style)
- Prefix internal/static functions with `_`

**Error Handling:**
- Return integer error codes (0 = success, negative = error)
- Use `goto` for cleanup on error (common kernel pattern)

**Testing:**
- Test files: `driver/tests/*.test.c`
- Use `assert_snapshot()` for snapshot testing

### TypeScript/Astro (Site)

- Use TypeScript strict mode
- Use Tailwind CSS utility-first approach
- `kebab-case` for component filenames
- `camelCase` for variables and functions

## 4. Naming Conventions Summary

| Language | Functions/Variables | Types/Enums | Files | Constants |
|----------|---------------------|-------------|-------|-----------|
| Rust | snake_case | PascalCase | snake_case.rs | SCREAMING_SNAKE_CASE |
| C | snake_case | PascalCase | snake_case.c | SCREAMING_SNAKE_CASE |
| TypeScript | camelCase | PascalCase | kebab-case.ts | SCREAMING_SNAKE_CASE |

## 5. Version Bumping

- **CLI changes:** Bump `cli/Cargo.toml` version AND create git tag
- **Driver changes:** Update `PKGBUILD` pkgver

**CLI version bump:**
1. Update `cli/Cargo.toml` version
2. `cargo update -p maccel-cli` - Update lock file
3. `git add -A && git commit -m "Bump CLI version to x.y.z"`
4. `git tag v<x.y.z> && git push origin v<x.y.z>`

**AUR Release:**
- Triggered automatically when a version tag (`v*`) is pushed
- Requires `AUR_SSH_KEY` secret in GitHub repository
- PKGBUILD `pkgver` must match the tag version (checked by workflow)

## 6. Commit Messages

- Short subject line (<50 chars), imperative mood
- Capitalize first letter
- Blank line between subject and body
- No period at subject line end

Example:
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
| Driver entry point | `driver/maccel.c` |
| Test utilities | `driver/tests/test_utils.h` |

## 8. Dependencies

- **Rust:** See `Cargo.toml` (workspace) and individual `Cargo.toml` files
- **C:** kernel headers, make, gcc
- **DKMS:** Required for driver installation
- **Site:** Node.js 24.x (`^24.0.0`), npm (see `site/package.json`)
