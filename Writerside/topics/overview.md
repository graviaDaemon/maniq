# Overview

Maniq is a statically typed, ahead-of-time compiled programming language. Source files use the `.mq` extension and are compiled by `maniq`, the Maniq compiler.

One Maniq codebase targets **Windows, Linux, macOS, iOS and Android**, for both console/back-end programs and GUI applications.

## Core decisions

- **Compiled, never interpreted** — no interpreter or VM stage.
- **Compiler written in C++26**, built with CMake.
- **Emits C11** — `maniq` generates C and hands it to the platform's C compiler (clang, MSVC, gcc, Xcode clang, Android NDK clang).
- **Android via the NDK** — Maniq runs as native code inside a thin Kotlin/Java host app.
- **Memory** — garbage collected by default, with explicit manual allocation/free when you want control.
- **Syntax** — C/Arduino family, statically typed.
- **GUI** — self-drawn on a canvas per platform (Flutter model), not native widgets.
- **Configuration** — TOML (`maniq.toml`).

Start with the [Roadmap](roadmap.md).
