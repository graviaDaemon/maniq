# Maniq

Maniq is a statically typed, ahead-of-time compiled programming language with C/Arduino-style syntax. One codebase is
meant to compile to **Windows, Linux, macOS, iOS and Android**, for both console programs and GUI applications.

The compiler, `maniq`, is written in C++26. It emits C11 and hands that to the platform's C compiler (clang, MSVC,
gcc, Xcode, the Android NDK), so one backend covers every target.

> **Status: pre-alpha.** This is a personal project to learn how compilers work, built one phase at a time. The lexer
> is done; the parser is next. Nothing compiles to a program yet.

## A taste

```text
@max(int a, int b) << int {
    if (a > b) {
        return a;
    }
    return b;
}

@main() << void {
    int biggest = max(3, 7);
    print(biggest);
}
```

More examples are in [`samples/`](samples). They show where the syntax is heading, not just what works today.

## Building

You need CMake 3.28 or newer and a C++26 compiler (GCC 15 is used for development).

```bash
cmake -S . -B build
cmake --build build
./build/maniq lex samples/hello.mq
```

## Tests

The tests are golden files: each `tests/lex/<name>.tokens` is the expected token dump of `samples/<name>.mq`.

```bash
ctest --test-dir build --output-on-failure
```

## Layout

| Path                | What                                                       |
|---------------------|------------------------------------------------------------|
| `src/`              | the compiler                                               |
| `samples/`          | example Maniq programs                                     |
| `tests/`            | golden-file tests                                          |
| `Writerside/topics` | language docs: overview, roadmap, grammar                  |
| `resources/`        | reading list and guides used while building                |

## Documentation

- [Overview](Writerside/topics/overview.md): the core design decisions
- [Roadmap](Writerside/topics/roadmap.md): the phases, from lexer to GUI
- [Grammar](Writerside/topics/grammar.md): the lexical and syntactic grammar in EBNF

## Feedback welcome

Ideas, questions, critique of the syntax or the design: all welcome. Please
[open an issue](../../issues/new/choose). Pull requests aren't accepted yet; see [CONTRIBUTING.md](CONTRIBUTING.md).

## License

All rights reserved for now; see [LICENSE](LICENSE). An open-source license will be chosen once Maniq reaches alpha.
