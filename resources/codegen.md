# Code generation: from a checked tree to an executable

Goal of this step: `maniq build samples/v0-good.mq` writes `out.c`, runs `gcc` on it and produces `v0-good.exe`, and
running that prints what the program says it prints.

```text
source → lexer → parser → checker → codegen → out.c → gcc → v0-good.exe
                                     ^^^^^^^ this phase
```

You have already written a pass that walks the tree and prints text: `dump`. Code generation is the same walk, with C
as the output instead of an indented tree. Same `dynamic_cast` dispatch, same recursion, new file `src/codegen.{h,cpp}`.

Why C at all: gcc already does register allocation, optimization and every CPU you'll ever target. You get native
speed and every platform for the price of printing text. Nim does exactly this (see the reading list).

**Which C:** C23 (ISO/IEC 9899:2024), with `-std=c23`. gcc 15 supports all of it (it's even gcc's default dialect),
and so do recent clang versions (macOS, iOS, the Android NDK). MSVC's C23 support is incomplete, but that doesn't
matter: the installer will ship its own C compiler. What C23 changes for you: `bool`/`true`/`false` are keywords (no
`<stdbool.h>`), attributes use the C++ spelling (`[[noreturn]]`), and `()` means "no parameters" (no `(void)`). For
later: `<stdckdint.h>` has `ckd_add` and friends, which detect overflow, if you ever want overflow to become an error
instead of wrapping around.

> **About Milestone 1.** The roadmap says "Hello World", but v0 has no strings: `print` takes an `int` or a `bool`.
> The v0 milestone is `print(42);` running as a native exe. The real Hello World arrives with strings in phase 7.

Already decided (2026-10-27): the C goes to `out.c` in the current folder, next to `out.tokens`/`out.ast`/`out.types`.
`gcc` is hard-coded and found on PATH. The exe is named after the source file. Project folders, a configurable
compiler and an installer that ships a C compiler come later.

---

## 0. Decide first: what the program does at run time

The checker decided what is *allowed*. Now you decide what allowed code *does*. C has opinions here, and some of them
are dangerous. Write your answers in a request file, like you did for the semantics rules.

1. **How big is `int`?** The parser stores literals in a C++ `int` (32-bit), so `99999999999` is already rejected.
   Options: 32-bit (`int32_t`) or 64-bit (`int64_t`). Recommended: 32-bit for v0, because it matches the parser. Use
   the exact-size type from `<stdint.h>`: plain C `int` has no guaranteed size.
2. **What does `print` output?** Does it add a newline? (With no strings, a v0 program can't print one itself.) How does
   a `bool` look: `true`/`false` or `1`/`0`? Recommended: newline after each call, and `true`/`false`.
3. **What happens on overflow?** In C, signed overflow is *undefined behaviour*: gcc may assume it never happens and
   "optimize" your program into nonsense. Options: wrap around (two's complement, one gcc flag: `-fwrapv`), or
   check and stop with an error (needs the runtime, phase 8). Recommended: `-fwrapv` now.
4. **Division and `%` with negatives.** C truncates toward zero: `-7 / 2` is `-3` and `-7 % 2` is `-1`. Python floors
   (`-4` and `1`). Recommended: follow C for v0; it's free.
5. **Division by zero.** In C it's undefined too (on x86 it crashes the program). Recommended: leave it for v0, and
   add a runtime check in phase 8.

**Check:** you can say what `print(-7 / 2);` and `print(2147483647 + 1);` print, without running anything.

> **Decided (2026-10-27, `requests/codegen.md`):** `int` is 32-bit (`int32_t`; a 32/64 choice becomes a `maniq new`
> option later). `print` ends with a newline and shows `true`/`false`. Overflow wraps (`-fwrapv`). `/` and `%`
> follow C. Division by zero stops the program with the runtime error `MQ60001`. So `print(-7 / 2);` prints `-3`,
> and `print(2147483647 + 1);` prints `-2147483648`.

## 1. Write the C by hand first

Same exercise as the `.ast` and `.types` files: before writing the generator, write the C you *want* it to produce,
for `samples/v0-good.mq`, into `tests/c/v0-good.c`. Compile it yourself:

```bash
gcc -std=c23 -fwrapv -Wall tests/c/v0-good.c -o v0-good.exe && ./v0-good.exe
```

Once it compiles without warnings and prints the right thing, it's the golden file. Every section below is about one
part of that file.

## 2. The shape of `out.c`

Four parts, always in this order:

```c
#include <stdint.h>
#include <stdio.h>

/* 1. runtime helpers (print) */
/* 2. a prototype for every Maniq function */
/* 3. the function definitions, in source order */
/* 4. the C entry point, which calls the Maniq one */
```

**Why prototypes?** In Maniq you can call a function before its definition (your §0 rule from the semantics guide,
and `v0-good.mq` does it with `countDown`). C only knows functions it has already seen. So the generator does the same
as the checker: one loop over all functions that writes `int32_t mq_max(int32_t mq_a, int32_t mq_b);`, then a second
loop that writes the bodies. Two passes again, for the same reason.

**Why a separate C `main`?** Maniq's `@main` becomes an ordinary C function `mq_main`. C's own `main` is three lines that
call it and return 0. Later `main` grows: it will start the GC, pass `args`, and turn `@main() << int` into the
exit code.

**Check:** `@main() << void {}` produces a C file that compiles, and the exe runs and prints nothing.

## 3. Names

Every name from the user's program gets a prefix: `max` becomes `mq_max`, `count` becomes `mq_count`. This fixes three
problems at once:

- a user function called `printf`, `abs` or `main` would collide with C;
- a variable called `int max = max(1, 2);` is legal Maniq (functions and variables live in separate tables), but in C
  the variable would hide the function inside its own initializer (noted in the decisions log);
- a variable named after a C keyword (`register`, `unsigned`, `auto`) would be a syntax error.

Your own runtime helpers use a different prefix, `maniq_`, so they can never collide with a user name: those always
start with `mq_`.

## 4. Types

| Maniq  | C                                          |
|--------|--------------------------------------------|
| `int`  | `int32_t` (or `int64_t`, your §0 answer)  |
| `bool` | `bool` (a keyword since C23)               |
| `void` | `void`                                     |

The checker already stored a type on every expression (`resultType`). You need it in one place: §6, `print`.

## 5. Statements

Most statements turn into the same thing in C. One by one:

| Node       | C                                                                                         |
|------------|-------------------------------------------------------------------------------------------|
| `Block`    | `{` the statements `}`                                                                    |
| `Variable` | `int32_t mq_count = <value>;`                                                             |
| `Assign`   | `mq_count = <value>;`                                                                     |
| `ExprStmt` | the call, then `;`                                                                        |
| `If`       | `if (<cond>) <block>` plus `else <block>`; an `else if` is an `If` as the else body       |
| `While`    | `while (<cond>) <block>`                                                                  |
| `Return`   | `return <value>;` or `return;`                                                            |

> **Pitfall:** `int x;` is allowed in Maniq and starts at `0` (your rule). In C, an uninitialized local holds whatever
> was in memory. Always write the default yourself: `int32_t mq_x = 0;`, `bool mq_b = false;`.

Indent the output (4 spaces per block level, like `dump`). You will read a lot of generated C while debugging, and
"keep the output readable" is on the roadmap for that reason.

**Check:** a `while` that counts down from 3 compiles and prints `3 2 1`.

## 6. Expressions

| Node     | C                                                                     |
|----------|-----------------------------------------------------------------------|
| `Int`    | the number                                                            |
| `Bool`   | `true` / `false`                                                      |
| `Name`   | `mq_<name>`                                                           |
| `Unary`  | `-<operand>`, `!<operand>`                                            |
| `Binary` | `<left> <op> <right>`; every v0 operator has the same spelling in C   |
| `/`, `%` | `maniq_div(<left>, <right>, line, col)`, `maniq_mod(...)`             |
| `Call`   | `mq_<name>(<args>)`                                                   |

**Parentheses:** the tree already encodes the grouping, and C has its own precedence table. The rule: an operand of a
binary operator gets parentheses if it is itself a binary operator; an operand of a unary operator gets them if it is a
binary *or* a unary operator. Nothing else gets them (`/` and `%` are calls, see below). A unary operand of a binary
operator needs none, because C's unary operators bind tighter than every binary one: `!even || x` is already
`(!even) || x`. Then the C can never be grouped differently from the tree. `(2 + 3) * 4 - 6 / 2` becomes
`((2 + 3) * 4) - maniq_div(6, 2, ...)`, which is a few parentheses more than needed, and that's fine. The rule also
protects you from a nasty one: `- -5` printed without parentheses is `--5`, and that's C's decrement operator.

**Division:** `/` and `%` are not printed as operators but as calls to a runtime helper, because C can't do what you
decided. The helper stops with `MQ60001` when the right side is `0`. It also has to handle `INT32_MIN / -1`: the
answer doesn't fit in 32 bits, and x86 *crashes* on it even with `-fwrapv`. The helper returns the wrapped value
(and `0` for `%`). The node's position is baked into the call, so the runtime error can point at the right `/`.

**`print`** is not a user function, so it has no `mq_` name. The argument's `resultType` picks the helper:
`print(count)` becomes `maniq_print_int(mq_count)`, and `print(even)` becomes `maniq_print_bool(mq_even)`. The two
helpers are a few lines of `printf` at the top of `out.c` (your §0 answer decides the format). In phase 8 they move
into a real runtime library that is linked in, instead of being pasted into every file.

**Check:** `print((2 + 3) * 4 - 6 / 2 + -5);` prints `12`.

## 7. Calling gcc

After writing `out.c`, `maniq` starts gcc as a separate process and waits for it:

```text
gcc -std=c23 -fwrapv out.c -o v0-good.exe
```

`std::system` takes one command string and returns the exit code. The exe is named after the source file
(`v0-good.mq` → `v0-good.exe`), as decided at the top of this guide.

> **Pitfall:** your repo lives in `D:\.GIT Personal`, a path with a space. `std::system` hands the string to the shell,
> which splits it at spaces. Put quotes around every path in the command.

Two things can go wrong, and neither is the user's fault:

- **gcc isn't installed or isn't on PATH.** That's an environment problem: report it as a build error (the `MQ5xxxx`
  range) with a hint to install gcc.
- **gcc rejects `out.c`.** Your generator produced invalid C. That's a compiler bug: report it as `MQ4xxxx`, say that
  it's an internal error, and print gcc's output underneath, because you'll need it to fix the bug.

> **Think about it:** should `maniq` pass `-Wall` to gcc? Warnings on generated code are usually *your* bugs (an unused
> variable is fine, an uninitialized one isn't). Turn it on while you develop; you can hide it later.

**Check:** break the generator on purpose (leave out a `;`). `maniq build` reports an `MQ4xxxx` internal error, not a
wall of gcc text with no explanation.

## 8. Tests

Two new golden files per valid program:

- `tests/c/<name>.c`: the expected `out.c` (from §1). This catches every change to the generated code, including
  harmless ones. That's the point: you look at each diff and decide.
- `tests/run/<name>.out`: what the exe prints. This catches the wrong *behaviour*, whatever the C looks like.

Programs with an error don't get either file: the build stops before code generation.

`tests/golden.cmake` gets two more modes, like `types`: build in a work folder and compare `out.c`, then run the exe
and compare its stdout. The full test runner is phase 6. Keep this minimal.

## 9. Order of work

| Step | Scratch input                                        | You add                                          |
|------|------------------------------------------------------|--------------------------------------------------|
| 1    | `@main() << void {}`                                 | the file shape (§2), C `main`, the gcc call (§7) |
| 2    | `print(42); print(true);`                            | `Int`, `Bool`, the print helpers                 |
| 3    | `int x; int y = 2; x = y; print(x);`                 | `Variable` with defaults, `Assign`, `Name`       |
| 4    | `print((2 + 3) * 4 - 6 / 2 + -5);`                   | `Binary`, `Unary`, parentheses                   |
| 5    | a function called before its definition              | prototypes, parameters, `Call`, `Return`         |
| 6    | `if` / `else if` / `else`, `while`                   | `If`, `While`                                    |
| 7    | `samples/v0-good.mq`                                 | the `.c` and `.out` golden tests                 |

---

## Walkthrough: `v0-good.mq` → `tests/c/v0-good.c`

You asked for the C to be written for you (`requests/codegen.md`), so `tests/c/v0-good.c` is the §1 file. It compiles
with `gcc -std=c23 -fwrapv -Wall -Wextra -pedantic` without a single warning and prints what
`tests/run/v0-good.out` says. Read it next to the sample; this walkthrough tells you where each section shows up.

**§2, the shape.** From top to bottom: `#include`s, the runtime helpers, four prototypes, four definitions, C `main`.
The includes are fixed: `stdint.h` (`int32_t`), `stdio.h` (`printf`, `puts`), `stdlib.h`
(`exit`), and `inttypes.h` (`PRId32`, the `printf` format that matches `int32_t` on every platform).

**The runtime helpers.** They're the same in every program; only the file name in `maniq_source` changes:

```c
static const char maniq_source[] = "v0-good.mq";
[[noreturn]] void maniq_runtime_error(...)/* prints the error in our format, exit code 1 */
void maniq_print_int(int32_t value)       /* printf + newline */
void maniq_print_bool(bool value)         /* "true" / "false" + newline */
int32_t maniq_div(...), maniq_mod(...)    /* §6: zero check, INT32_MIN / -1 */
```

`maniq_runtime_error` calls `fflush(stdout)` first. Without it, the program's earlier `print`s can show up *after* the
error, because stdout is buffered when it goes into a file or a pipe (the tests do exactly that). `[[noreturn]]` tells
gcc the function never returns, with the same spelling as in C++.

**§2, prototypes.** One line per function, in source order. `mq_main` calls `mq_countDown`, which is defined later;
without the prototype, gcc refuses (C has had no implicit declarations since C99).

```text
@max(int a, int b) << int          →  int32_t mq_max(int32_t mq_a, int32_t mq_b);
@main() << void                    →  void mq_main();
```

In C23 an empty `()` means "no parameters", as in C++. (Before C23 it meant "unknown parameters", and you had to write `(void)`; you will see that in older C code.)

**§3 and §4, names and types.** Every user name has `mq_`, every helper `maniq_`. `int` → `int32_t`, `bool` →
`bool`, `void` → `void`, in parameters, variables and return types alike.

**§5, statements.** Line for line:

```text
int biggest = max(3, 7);           →  int32_t mq_biggest = mq_max(3, 7);
count = count - 1;                 →  mq_count = mq_count - 1;
countDown(3);                      →  mq_countDown(3);
if (...) {...} else if (...) {...} →  if (...) {...} else if (...) {...}
while (count > 0) {...}            →  while (mq_count > 0) {...}
return a;                          →  return mq_a;
```

The `else if` comes for free: the else body is an `If` node, so the generator writes `else ` and then the nested `if`
as usual. Blank lines inside a function are not kept: the tree doesn't have them.

**§6, expressions.** The interesting lines:

```text
return n % 2 == 0;                 →  return maniq_mod(mq_n, 2, 9, 14) == 0;
if (even && biggest > 5)           →  if (mq_even && (mq_biggest > 5))
!even || biggest <= 0              →  !mq_even || (mq_biggest <= 0)
print((2 + 3) * 4 - 6 / 2 + negative);
    →  maniq_print_int((((2 + 3) * 4) - maniq_div(6, 2, 28, 27)) + mq_negative);
print(even...)  /  print(0)        →  maniq_print_bool(...)  /  maniq_print_int(0)
```

`9, 14` and `28, 27` are the positions of the `%` and `/` tokens (see `tests/lex/v0-good.tokens`). The condition of the
`if` has no extra parentheses around it: only *operands* get them.

**§2 again, C `main`.** It calls `mq_main()` and returns `0`.

**What it prints:** `3 2 1` from `countDown`, `7`, then `false` (7 is odd, so the `else if` branch runs), then
`20 - 3 + -7 = 10`. Each value goes on its own line.

**Not in this file, but tested by hand:** `maniq_div(1, 0, ...)` prints `v0-good.mq:3:15` /
`    ==> Runtime-Error[MQ60001]: division by zero` after all earlier output and exits with 1; `INT32_MIN / -1` gives
`-2147483648`, `INT32_MIN % -1` gives `0`, `-7 / 2` gives `-3`, `-7 % 2` gives `-1`, and `2147483647 + 1` gives
`-2147483648` (gcc warns about that one when both numbers are literals: the C is still correct).

---

## Summary checklist

- [x] the five run-time rules from §0 decided and written down
- [x] `tests/c/v0-good.c` written by hand; it compiles without warnings and runs correctly
- [x] `src/codegen.{h,cpp}`: includes, helpers, prototypes, definitions, C `main`
- [x] every user name prefixed `mq_`, runtime helpers `maniq_`
- [x] variables without a value start at their default
- [x] a `Binary`/`Unary` operand in parentheses; `/` and `%` through `maniq_div`/`maniq_mod`
- [x] `print` picks its helper from `resultType`
- [x] gcc called with quoted paths; gcc missing → `MQ5xxxx`, gcc rejects `out.c` → `MQ4xxxx`
- [x] `maniq build` produces `<name>.exe`; `.c` and `.out` golden tests pass for every valid sample
