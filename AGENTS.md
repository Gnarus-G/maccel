# Agent Guidelines for maccel Repository

This document outlines the essential commands and style conventions for agentic coding in this repository.

## 1. Build, Lint, and Test Commands

*   **Build All:**
    *   `make build` (driver)
    *   `cargo build --bin maccel --release` (CLI)
    *   `astro check && astro build` (site)
*   **Run All Tests:**
    *   `make test` (C driver tests)
    *   `cargo test --all` (Rust tests)
*   **Run Single Test:**
    *   C Test: `TEST_NAME=<regex_pattern> make test` (e.g., `TEST_NAME=accel.test.c make test`)
    *   Rust Test: `cargo test <test_name>` (e.g., `cargo test my_specific_test_function`)
*   **Linting & Formatting:**
    *   `cargo fmt --all` (Rust formatting)
    *   `cargo clippy --fix --allow-dirty` (Rust linting with auto-fix)
    *   `astro check` (Astro/TypeScript type checking and linting)

## 2. Code Style Guidelines

*   **General:** Adhere to existing project conventions. Mimic surrounding code style, structure, and patterns.
*   **Rust:** Follow `rustfmt` and `clippy` conventions.
*   **C:** No explicit style guide, follow common C best practices.
*   **Astro/TypeScript/Tailwind (site):** Adhere to TypeScript best practices and Tailwind CSS utility-first approach.
*   **Imports:** Organize imports consistently with existing files.
*   **Naming Conventions:** Use `snake_case` for Rust functions/variables, `PascalCase` for Rust types/enums. Follow existing conventions for C and Astro/TypeScript.
*   **Error Handling:** Implement robust error handling appropriate for each language (e.g., Rust's `Result` type, C error codes, TypeScript `try-catch`).
*   **Comments:** Add comments sparingly, focusing on *why* rather than *what*.
*   **Commit Messages:**
    *   Short, descriptive subject line (<50 chars), imperative mood (e.g., "Fix:", "Add:", "Update:").
    *   Capitalize first letter.
    *   Blank line between subject and optional detailed body (~72 char line wrap).
    *   No period at subject line end.
