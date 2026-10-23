# Resources

Reading list for building Maniq, grouped by roadmap phase. Drop your own PDFs and notes in this folder.

Each entry: what it is, cost, **read** (what matters) and **skip** (what doesn't apply to Maniq).

> *Crafting Interpreters* builds interpreters, but the front end (lexer, parser, AST, scopes) is identical in a compiler. Read those chapters 1:1; skip every chapter that *executes* code (tree-walk evaluation, the bytecode VM).

---

## General / big picture

- **Crafting Interpreters** — Robert Nystrom · free · https://craftinginterpreters.com/contents.html
  Read: ch. 1–3 (Part I), especially [ch. 2 "A Map of the Territory"](https://craftinginterpreters.com/a-map-of-the-territory.html).
- **Writing a C Compiler** — Nora Sandler (No Starch, 2024) · paid · https://nostarch.com/writing-c-compiler
  A real compiler built incrementally and test-first. Read: ch. 1–3 for the "grow a compiler one feature at a time" rhythm. The assembly output does not apply to the main path (we emit C); it becomes relevant in phase 12.
- **chibicc** — Rui Ueyama · free · https://github.com/rui314/chibicc
  A small C compiler built commit by commit. Read: the commit history from the first commit, to see how a compiler grows. Skip: the x86-64 codegen (until phase 12).
- **Engineering a Compiler** — Cooper & Torczon · paid · https://www.elsevier.com/books/engineering-a-compiler/cooper/978-0-12-815412-0
  Reference book; look things up, don't read cover to cover.

## Phase 0 — Toolchain setup

- **toolchain-setup.md** (this folder) — How to open, build and run the `maniq` project in CLion and from the command line. Start here.
- **CMake tutorial** · free · https://cmake.org/cmake/help/latest/guide/tutorial/index.html — Read: steps 1–3. Skip: packaging/installing for now.
- **learncpp.com** · free · https://www.learncpp.com/ — For C++ depth as you need it: `std::string_view`, `std::variant`, `std::unique_ptr`, move semantics.
- **LLVM/clang downloads** · free · https://releases.llvm.org/download.html — Clang for Windows (MSVC from Visual Studio also works).
- **Compiler Explorer** · free · https://godbolt.org/ — See what C compiles to; handy to check your generated C.
- **doctest** · free · https://github.com/doctest/doctest — Optional single-header unit-test library. Alternative: **Catch2** https://github.com/catchorg/Catch2.

## Phase 1 — Design the language on paper

- **ebnf.md** (this folder) — What EBNF is, how to read `Writerside/topics/grammar.md`, and how to grow it. Start here.
- **Extended Backus–Naur form** (Wikipedia) · free · https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form — Read: "Basics" and the table of symbols. Enough to write your grammar.
- **Crafting Interpreters ch. 3 "The Lox Language"** · free · https://craftinginterpreters.com/the-lox-language.html — Example of a small language described in a few pages. Skip: dynamic typing discussions (Maniq is static).

## Phase 2 — Lexer

- **lexer.md** (this folder) — How to start the `Token` struct and the lexer loop, step by step, until `tests/lex/hello.tokens` passes. Start here.
- **Crafting Interpreters ch. 4 "Scanning"** · free · https://craftinginterpreters.com/scanning.html — Read fully.

## Phase 3 — Parser + AST

- **Crafting Interpreters ch. 5 "Representing Code"** · free · https://craftinginterpreters.com/representing-code.html — Read; in C++ use `std::variant` or a class hierarchy instead of the Java visitor generator.
- **Crafting Interpreters ch. 6 "Parsing Expressions"** · free · https://craftinginterpreters.com/parsing-expressions.html — Read fully (recursive descent, error recovery).
- **Crafting Interpreters ch. 8 "Statements and State"** · free · https://craftinginterpreters.com/statements-and-state.html — Read: the parsing parts. Skip: the "Environment"/execution parts. Skip ch. 7 entirely (it evaluates).
- **Crafting Interpreters ch. 17 "Compiling Expressions"** · free · https://craftinginterpreters.com/compiling-expressions.html — Read: the Pratt parser sections. Skip: bytecode emission.
- **Simple but Powerful Pratt Parsing** — matklad · free · https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html — Short, clear Pratt explanation with binding powers.

## Phase 4 — Semantic analysis

- **Crafting Interpreters ch. 11 "Resolving and Binding"** · free · https://craftinginterpreters.com/resolving-and-binding.html — Read: scopes and name resolution. Type checking is not covered (Lox is dynamic).
- **Writing a C Compiler** — the chapters on semantic analysis / identifier resolution and type checking (see table of contents).

## Phase 5 — C code generation

- **Nim backends** · free · https://nim-lang.org/docs/backends.html — A real language that compiles to C. Read: the C backend sections, to see how a compiler drives the C compiler.
- **Compiler Explorer** · https://godbolt.org/ — Paste your generated C to check it compiles and what it turns into.

## Phase 6 — Test harness

- **Writing a C Compiler** — the test-suite approach (run every test program, compare output).
- **chibicc** `test/` folder · https://github.com/rui314/chibicc — See how tests grow with each commit.

## Phase 7 — Grow the language

- **Crafting Interpreters ch. 8–13** (parsing and resolving parts only) · https://craftinginterpreters.com/contents.html — functions, classes as feature-by-feature examples.
- **chibicc** commit history — structs, arrays, strings added one commit at a time.

## Phase 8 — Runtime & memory

- **Baby's First Garbage Collector** — Nystrom · free · https://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/ — Read fully. Mark-and-sweep in ~100 lines of C.
- **Crafting Interpreters ch. 26 "Garbage Collection"** · free · https://craftinginterpreters.com/garbage-collection.html — Read: roots, tracing, tri-color marking, sweeping. Skip: VM-specific details.
- **The Garbage Collection Handbook** — Jones, Hosking, Moss · paid · https://gchandbook.org/ — Reference; ch. 2 (mark-sweep) is the relevant start.
- **Boehm GC** · free · https://www.hboehm.info/gc/ — Allowed temporary stand-in before your own GC.

## Phase 9 — Standard library + build tool

- **TOML v1.0.0 spec** · free · https://toml.io/en/v1.0.0 — Read fully; it's short.
- **toml++** · free · https://github.com/marzer/tomlplusplus — Header-only C++ TOML parser for `maniq.toml`.

## Phase 10 — Platforms

- **WSL install** · free · https://learn.microsoft.com/en-us/windows/wsl/install — Linux target on the Windows machine.
- **Android NDK guides** · free · https://developer.android.com/ndk/guides — Read: "Get started", "CMake". Also https://developer.android.com/ndk/guides/cmake and samples https://developer.android.com/ndk/samples.
- **Apple: Creating a static framework** · free · https://developer.apple.com/documentation/xcode/creating-a-static-framework — Packaging the compiled C for an Xcode host app.
- **Apple: Using imported C functions in Swift** · free · https://developer.apple.com/documentation/swift/using-imported-c-functions-in-swift — Calling into Maniq's C output from a Swift host.

## Phase 11 — Graphics

- **SDL3 wiki** · free · https://wiki.libsdl.org/SDL3/FrontPage — Read: windows, renderer, events. Platform notes: https://wiki.libsdl.org/SDL3/README-android, https://wiki.libsdl.org/SDL3/README-ios.
- **Flutter architectural overview** · free · https://docs.flutter.dev/resources/architectural-overview — Read: the layer model, rendering and layout. Skip: Dart-specific and platform-channel parts.

## Phase 12 — Later / optional

- **Writing a C Compiler** — Part I–II assembly generation: the base for an own x86-64 backend.
- **chibicc** — x86-64 codegen as a second reference.
- **LLVM Kaleidoscope tutorial** · free · https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html — Only if you ever reconsider LLVM as a backend.
