# Roadmap

Where to start, what to build first, and how to proceed. One phase at a time; each ends with a concrete **Done when**.

The full reading list, with what to read and what to skip per resource, lives in `resources/README.md` at the repository root, grouped by the same phase numbers.

## How a compiler works

```
source.mq → lexer → tokens → parser → AST → type checker → C codegen → C compiler → executable
```

- **Lexer** — cuts text into tokens (`int`, `x`, `=`, `42`, `;`).
- **Parser** — turns tokens into a tree (the AST) that matches your grammar.
- **Type checker** — resolves names, checks types, reports errors.
- **C codegen** — walks the checked AST and writes C11 text; the platform's C compiler does the rest.

Everything up to the type checker is the *front end*. It is the same for interpreters and compilers, which is why interpreter books are still useful here.

## Phase 0 — Toolchain setup

Get a working C++26 build and a way to test before writing compiler code.

- Set up a C++26 compiler (CLion's bundled MinGW/GCC), CMake and Git on Windows. Step-by-step how-to: `resources/toolchain-setup.md` at the repository root.
- Create an empty `maniq` CMake project that builds and prints its version.
- Decide the test approach: **golden-file tests** — `tests/*.mq` next to the expected output. Optionally doctest or Catch2 for unit tests.
- Bookmark Compiler Explorer for experiments.

**Done when:** `cmake --build` produces `maniq.exe`, and you can explain how a golden-file test will work.

**Resources:** [CMake tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html) · [learncpp.com](https://www.learncpp.com/) · [Compiler Explorer](https://godbolt.org/) · [doctest](https://github.com/doctest/doctest)

## Phase 1 — Design the language on paper

Decide what Maniq looks like before building anything.

- Write 5–10 sample `.mq` programs: hello world, arithmetic, if/loops, functions, structs, arrays, strings, and one manual vs GC allocation example.
- Distill an informal grammar in EBNF from those samples.
- Mark a tiny **v0** subset: `int`, `bool`, functions, `if`/`while`, `print`. Everything else waits.

**Done when:** the samples exist, the v0 grammar fits on one page, and each v0 sample can be derived from it by hand.

**Resources:** [EBNF (Wikipedia)](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form) · [Crafting Interpreters ch. 3](https://craftinginterpreters.com/the-lox-language.html)

## Phase 2 — Lexer

Source text → tokens, each with line and column.

- Define the token kinds for v0 (keywords, identifiers, literals, operators, punctuation, end-of-file).
- Scan one character at a time; skip whitespace and comments; report unknown characters with their position.
- Add `maniq lex file.mq` that dumps tokens, one per line.

**Done when:** every v0 sample tokenizes, and token-dump golden tests pass.

**Resources:** [Crafting Interpreters ch. 4 "Scanning"](https://craftinginterpreters.com/scanning.html) · [Writing a C Compiler](https://nostarch.com/writing-c-compiler)

## Phase 3 — Parser + AST

Tokens → AST.

- Define AST node types (C++: `std::variant` or a small class hierarchy).
- **Recursive descent** for statements and declarations; **Pratt parsing** for expressions (precedence and associativity).
- Add an AST pretty-printer (`maniq parse file.mq`) for golden tests.
- Errors carry line and column: `hello.mq:3:7: expected ';'`.

**Done when:** every v0 sample parses, AST-dump golden tests pass, and broken inputs give located errors.

**Resources:** [CI ch. 5](https://craftinginterpreters.com/representing-code.html) · [CI ch. 6](https://craftinginterpreters.com/parsing-expressions.html) · [CI ch. 17 (Pratt)](https://craftinginterpreters.com/compiling-expressions.html) · [Pratt parsing — matklad](https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html)

## Phase 4 — Semantic analysis

Check that a parsed program makes sense.

- Scopes and name resolution: every identifier points to its declaration.
- Type checking: every expression gets a type; mismatches are errors.
- Clear messages: what is wrong, where, and what was expected.

**Done when:** each invalid test program produces the expected error, and every valid one passes the checker.

**Resources:** [CI ch. 11 "Resolving and Binding"](https://craftinginterpreters.com/resolving-and-binding.html) · [Writing a C Compiler](https://nostarch.com/writing-c-compiler)

## Phase 5 — C code generation

Checked AST → C11 text → native executable.

- Walk the AST and emit C; keep the output readable (you will debug it).
- Write the `.c` file to a build folder and invoke clang/MSVC from `maniq`.
- Report C compiler failures as `maniq` bugs, not user errors.

**Milestone 1: `maniq build hello.mq` → native `hello.exe` printing Hello World.**

**Done when:** Milestone 1 works and every v0 sample compiles and runs correctly.

**Resources:** [Nim backends](https://nim-lang.org/docs/backends.html) · [Compiler Explorer](https://godbolt.org/)

## Phase 6 — Test harness

Automate everything from phase 2–5.

- One command runs every `tests/*.mq`: compile, run, compare stdout and exit code — or the compile error — against the expected files.
- Run it on every change from here on.

**Done when:** a single command runs the whole suite and reports pass/fail per file.

**Resources:** [chibicc tests](https://github.com/rui314/chibicc) · [Writing a C Compiler](https://nostarch.com/writing-c-compiler)

## Phase 7 — Grow the language

Add features one at a time: functions, structs, arrays, strings, modules/imports, then whatever you design.

For every feature, the same loop:

1. sample program
2. grammar
3. lexer / parser
4. checker
5. codegen
6. tests

**Done when:** the phase 1 samples beyond v0 all compile and run, each covered by tests.

**Resources:** [chibicc commit history](https://github.com/rui314/chibicc) · [Crafting Interpreters](https://craftinginterpreters.com/contents.html) (parsing and resolving parts)

## Phase 8 — Runtime & memory

A small C runtime linked into every Maniq program.

- Start with manual memory: explicit allocate/free in the language, backed by the runtime.
- Then add a simple mark-and-sweep GC of your own. Boehm GC is allowed as a temporary stand-in.
- Define how they coexist: GC objects are traced from roots; manually allocated memory is never touched by the GC; decide what happens when one points to the other.

**Done when:** the manual vs GC sample runs correctly, and a test allocating in a loop stays at stable memory.

**Resources:** [Baby's First Garbage Collector](https://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/) · [CI ch. 26](https://craftinginterpreters.com/garbage-collection.html) · [GC Handbook](https://gchandbook.org/) · [Boehm GC](https://www.hboehm.info/gc/)

## Phase 9 — Standard library + build tool

- Core library (I/O, strings, collections), written increasingly in Maniq itself.
- `maniq.toml` project manifest (TOML), parsed with toml++.
- `maniq build --target <platform>` reads the manifest and builds the project.

**Done when:** a multi-file project builds with `maniq build` from its `maniq.toml`.

**Resources:** [TOML spec](https://toml.io/en/v1.0.0) · [toml++](https://github.com/marzer/tomlplusplus)

## Phase 10 — Platforms

Same program, more targets. In order:

1. **Linux** — WSL with gcc/clang.
2. **macOS** — clang on the MacBook.
3. **iOS** — Maniq output as a static library inside an Xcode host project.
4. **Android** — NDK clang, loaded by a thin Kotlin host app.

**Milestone 2: the same console program runs on all desktop targets.**

**Done when:** Milestone 2 holds, and a Maniq function is callable from the iOS and Android host apps.

**Resources:** [WSL](https://learn.microsoft.com/en-us/windows/wsl/install) · [Android NDK](https://developer.android.com/ndk/guides) · [Apple: static framework](https://developer.apple.com/documentation/xcode/creating-a-static-framework)

## Phase 11 — Graphics

Self-drawn UI on a canvas (Flutter model).

- Window + canvas through SDL3, exposed to Maniq.
- Event loop, drawing primitives, text.
- Then widgets and layout, written in Maniq.

**Milestone 3: one Maniq GUI app runs on Windows, macOS and Android.**

**Done when:** Milestone 3 holds.

**Resources:** [SDL3 wiki](https://wiki.libsdl.org/SDL3/FrontPage) · [Flutter architecture](https://docs.flutter.dev/resources/architectural-overview)

## Phase 12 — Later / optional

- Own x86-64 backend, as a learning track next to the C backend.
- Editor support: syntax highlighting, then a language server (LSP).
- Self-hosting: the Maniq compiler written in Maniq.

**Resources:** [Writing a C Compiler](https://nostarch.com/writing-c-compiler) · [chibicc](https://github.com/rui314/chibicc) · [LLVM Kaleidoscope](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html) (only if you reconsider LLVM)

## How to work through this

- One phase at a time; don't start the next until **Done when** holds.
- Keep the test suite green; a failing test is the next task.
- Bring designs and questions to Claude for review before building them.
- Record every language-design choice in these docs as you make it.
