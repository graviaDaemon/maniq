# Samples review (phase 1)

## Short answer: yes, this is what phase 1 asks for

Phase 1 is "design the language on paper": write programs the way you *want* Maniq to look, before any compiler
exists. That is exactly what `samples/` is. The comments explaining your intent (`for`, `foreach`, the access
symbols) are the most useful part, because they are the design notes the grammar will be built from.

In `func.mq` you wrote that you did not expect to make design choices yet. Making them is the point of this phase.
The grammar is only a precise summary of the choices in the samples, so every open choice now turns into a hole
in the grammar later. You don't have to decide *everything*. You can also defer a feature (variadic `%params`,
say) by leaving it out of v0.

So the samples are about **two thirds of the way there**. Here's what's missing:

1. **Consistency.** The samples contradict each other in a few places. A grammar can only describe one language.
2. **Coverage.** A few roadmap items are missing or only half there.
3. **A v0 subset.** The roadmap asks you to mark the tiny subset that gets built first. That's not done yet.

## Coverage against the roadmap list

| Roadmap item            | Sample              | Status                                                            |
|-------------------------|---------------------|-------------------------------------------------------------------|
| hello world             | `hello.mq`          | ✓                                                                 |
| arithmetic              | `arith.mq`          | ✓ (no operator precedence/parentheses case, no `%`)               |
| if / loops              | `conditional.mq`    | `if`/`else if`/`for`/`foreach` ✓, **`while` missing**            |
| functions               | `func.mq`, `struct` | half: **no return types**, no top-level function returning a value |
| structs                 | `struct.mq`         | ✓ (it's a `class`, see below)                                     |
| arrays                  | `types.mq`          | half: literal only, **no indexing** (`list[0]`), no length        |
| strings                 | `hello.mq`          | half: literal only, no concatenation/escapes (`"\n"`, `"\""`)     |
| manual vs GC allocation | `types.mq`          | present, but the example doesn't do what the comments say (see below) |
| booleans                | `types.mq`          | half: `true` exists, **no `==`, `!=`, `&&`, `\|\|`, `!`**          |

## Conflicts to resolve

Ordered from "will break the lexer/parser" to "a matter of taste".

### 1. `*` means two different things
`struct.mq` uses `*int protectedInteger;` for *protected*. `types.mq` uses `*int allocMemory` for *pointer to int*.
Inside a class, `*int x;` could mean either one, and nothing in the source tells them apart. `-` and `+` have the
same problem in a weaker form, since they are also arithmetic operators (`-x`).

Options:
- keywords: `pub int x;`, `priv int x;`, `prot int x;` (no collisions, easy to read)
- keep the symbols for access and pick a different pointer syntax (`int* p`, `ptr<int> p`, `&int p`)

### 2. Access modifiers: prefix on fields, suffix on functions
Fields are `-int x;`, but functions are `#name(...) { ... }+`. Two rules for one concept. The suffix also means a
reader only learns a method's visibility after reading the whole body. Pick one position, prefix, for both.

### 3. `#` means "function" and also "`#define`"
`#print(...)` declares a function, while `#define Numbers from 'struct';` imports something. With `#` meaning "a
function follows", `#define` reads as a function named `define`. Also, `#define` already means "text macro" to
every C/C++ reader. Suggestion: give imports their own keyword (`import Numbers from "struct";` or `use struct.Numbers;`).

### 4. Functions have no return type
`#addImplicitPublic(int adder) { return publicInteger + adder; }` returns an `int`, but that's written nowhere.
Maniq is statically typed, so you have to pick one of these:
- write it: `#add(int a, int b) -> int { ... }` or C-style `int add(int a, int b) { ... }`
- infer it from the `return` statements (simpler to write, harder to build, and errors get worse)

C-style `int add(...)` would also make `#` unnecessary. Whether you want that is a matter of taste.

### 5. Quotes: `'struct'` vs `'W'`
`'W'` is a `char`, and `"Hello"` is a `str`. `'struct'` uses single quotes for several characters, which
contradicts that. Choose one meaning for `'...'` (a single char) and use `"struct"` for the import path.

### 6. Missing/extra semicolons
You use `;` almost everywhere, so you have effectively decided on semicolons. Write that down as a decision. These
lines break the rule:
- `conditional.mq`: `Numbers nums = Numbers(1, 2.3, 4.5)` (no `;`)
- `types.mq`: `delete::allocMemory` (no `;`)

### 7. `open p = ...` in `arith.mq`
What is `open`? If it means "infer the type" (like C++ `auto` or C# `var`), that's fine and common in static
languages, but say so in a comment. If it means "dynamic type", it conflicts with static typing.

### 8. `class` in a file called `struct.mq`
That's only a naming question, but decide whether Maniq has `class`, `struct`, or both. If both, what's the
difference? In C# a struct is a value type and a class is a reference type, and that ties directly into the GC
question below.

### 9. Loops
- `for ([i]def 0..n)`: `[` already starts array literals (`[1, 2, 3]`), so `[i]` here is ambiguous, and `def` adds
  a third piece. Something like `for (i in 0..n)` says the same with less.
- Is `0..n` inclusive or exclusive of `n`? Decide and write it down. Some languages offer both (`0..n` and `0..<n`).
- `foreach (item|list)`: `|` is "bitwise or" in the C family, so readers will stumble. If `for (i in 0..n)` exists,
  then `for (item in list)` covers `foreach` with the same syntax: one loop keyword instead of two.
- You already reuse `..` for ranges in `func.mq` (`'{'..'}'`). That kind of consistency is good. Keep it.

### 10. Memory example (`types.mq`) doesn't do what the comments say
- `*int allocMemory = 0xff;` makes a pointer that points **to address 0xff**. It doesn't allocate a byte holding
  `0xff`. Allocation needs an explicit operation, for example `*int p = alloc int(255);` or `new`.
- `delete::allocMemory`: `::` usually means "scope lookup" (C++ `std::cout`). A statement such as `free(p);` or
  `delete p;` reads more clearly.
- `int gcCollectorAutoClean = 20;`: a plain `int` lives on the stack and disappears at the end of its scope. The GC
  never touches it. The GC cleans up **heap** values (class instances, arrays, strings). A better example is
  `Numbers n = Numbers(1, 2.0, 3.0);` inside a block, with no `delete`.
- Use real blocks `{ ... }` instead of `// Scope starts` comments. Then the grammar has to support nested blocks,
  which it needs anyway.

### 11. Small bugs in the samples
- `conditional.mq` calls `nums.addExplicitlyPublic(3)`, but the function takes two parameters `(int a, int b)`.
- `func.mq` declares `print` twice with different parameters. That's **overloading**: do you want it? Also, in
  `hello.mq` `print` is used without being declared, so is it built in or imported?
- `arith.mq` uses `pow` and `sqrt`. Built in, or from an import (`import math;`)?

## Decisions to write down before the grammar

Answer these (one line each is enough), ideally as a "Language decisions" topic in `Writerside/`:

1. Statement terminator: `;` (your samples already say yes).
Yes
2. Access modifiers: keywords or symbols, and which position?
Symbols, changed in the samples
3. Pointer syntax (must not collide with 2).
Changed in the samples
4. Function declaration form, including the return type.
Changed in the samples
5. Import syntax.
Changed in the samples
6. `'...'` = char only? `"..."` = string?
Changed in the samples, in short. Yes '...' is CHAR and "..." is string
7. `float` / `decimal` / `double`: three distinct types or synonyms? (still open from last session)
These are distinct types. Floating points (same-ish as double in C# same enough here to use only one), Decimal, Integers.
8. `open` / type inference: yes or no?
Explained in comments, yes.
9. Loop forms: `for`, `while`, `foreach`? Range inclusive or exclusive?
10. `class`, `struct`, or both?
11. Program entry point: are top-level statements allowed (like your samples, and C# top-level statements) or is
    there a `main`? Your samples imply top-level statements, which is fine, but make it a decision.
I think this won't really be a scripting language, so we would use a Main. Should I include this in every file?
12. Overloading: yes or no?
Yes

## Marking v0

The roadmap's v0 is: `int`, `bool`, functions, `if`/`while`, `print`. None of the current samples is v0-only: each one
uses classes, strings, floats or imports. Add one or two small samples that use **only** v0 features, so you have
something to derive by hand from the v0 grammar. For example, a `while` loop counting down and a function
`max(a, b)` with an `if`. Name them so they're easy to spot, e.g. `samples/v0-countdown.mq`.

Everything else stays in `samples/` as the "full language" vision. The grammar for it can grow later, one feature at
a time (roadmap phase 7).

## Aside: `tests/lex/hello.tokens`

You'll need this in phase 2, so here's a note now. A lexer turns `"Hello, world!"` into **one** token
(`STRING_LIT  "Hello, world!"`), not one `CHAR` per letter. The same goes for `'W'` (`CHAR_LIT  'W'`). The space
inside the string isn't `\r` either. That's a carriage return, part of a Windows line ending. Also, naming the
keyword `str` as token `STRING` will clash with the string *literal* token, so something like `KW_STR` vs
`STRING_LIT` keeps them apart. The same applies to `FUNC print`: the lexer can't know `print` is a function, so it's
just `IDENT`. The parser figures out it's a call from the `(` that follows.

## Next step: the EBNF grammar

Once the decisions above are made and the samples are updated to match them, start the grammar in
`Writerside/topics/grammar.md`. Work top-down for the v0 subset only:

1. `program = { declaration } ;`
2. declarations: variable, function
3. statements: block, `if`, `while`, `return`, expression statement
4. expressions, **one rule per precedence level** (lowest first: `||`, `&&`, equality, comparison, `+ -`, `* /`,
   unary, call/primary). This layering is what makes `1 + 2 * 3` parse correctly.

Done when every v0 sample can be derived from it by hand. Bring the first draft for review.

---

# Round 2 review (2026-10-23)

Big improvement. Most of the conflicts are resolved, and the result is one consistent idea: `#` = compiler
directive (`#use`, `#define`), `@` = function, `$` = constructor, `-` `+` `*` = access, `&` = manual memory. The
lexer handles all of this without trouble (see `tests/lex/*.tokens`).

## Your answers, checked

- **1. `;`**: yes. Recorded.
- **2. Access symbols, prefix**: works now that `*` no longer means pointer. Inside a class body a member can only
  be a declaration, so `*int x;` and `-@f()` are unambiguous there.
- **3. Pointers as `&T`**: fine. `&` only appears in type position, so it won't clash with `&&` or a later
  address-of operator.
- **4. `@name(...) << type`**: works. One remark: in the C family `<<` reads as "shift left", and `->` is the more
  common choice (C++, Rust, Swift). The parser doesn't care, since `<<` only appears after `)` in a declaration.
  It's your call; I've used `<<` in the grammar.
- **5. `#use X from <path>`**: works. The lexer reads `<...>` as one `PATH_LIT` token only directly after `from`,
  the same trick C uses for `#include <stdio.h>`. That's needed because `library-defined-in-toml-file` contains
  `-`, so the lexer can't read it as identifiers.
- **6. `'c'` / `"s"`**: yes.
- **7. `float`, `decimal`, `int` are distinct types**: fine. Two things to know for later. C has no decimal type,
  so `decimal` needs runtime-library support (phase 8/9). And you still need to decide the size of `int`, 32 or
  64 bits (phase 4/5).
- **8. `open` = inference**: good.
- **9, 10**: not answered in the file, but the samples answer them: `for`, `foreach`, `while` and `do-while` all
  exist, and both `class` and `struct` exist. Still open: **is `0..n` inclusive or exclusive?** I recommend
  *exclusive* (`0..3` gives 0, 1, 2), because then `0..list.size` walks every index of a list.
- **11. `main`**: no, not in every file. A program has **exactly one** `@main`, in the file you pass to
  `maniq build`, as in C and C#. Other files contain only declarations (functions, classes, structs, `#define`,
  `#use`). I've written it as `@main() << void` in `samples/v0-example.mq`. This means the top-level statements in
  `hello.mq`, `arith.mq` etc. will need to move inside a `@main` once their features reach the grammar.
- **12. Overloading**: yes. Recorded.

## Remaining issues in the samples

None of these block v0. Fix them when the feature reaches the grammar.

- `conditional.mq` line 2 still has the old `#define Numbers from 'struct';`. The lexer produces an `ERROR` token for
  `'struct'` (see `tests/lex/conditional.tokens`). That's actually a useful test case to keep around.
- `struct.mq`: `#addImplicitPublic` still uses `#` instead of `@`. The methods have no `<< int` return type, and the
  doc comment still says modifiers come *after* the declaration.
- **Directive endings**: `#define ... ;` and the first `#use ... ;` end with `;`, but the other `#use` lines don't.
  I recommend `;` everywhere, consistent with answer 1.
- `do { } while (first < second)` has no `;`. Also, `first` never changes, so both loops run forever. That's fine
  for a sketch, but not for a test that runs.
- `conditional.mq`: the comment above `for` still describes the old `[i]def` syntax.
- `func.mq`: `for (i in 0..params)` should probably be `0..params.size`.
- `types.mq`: `&int allocMemory = int(someArray[2]);` — what makes this allocate? `int(x)` reads like a conversion.
  An explicit `new` keeps it clear and mirrors the GC case nicely:

  ```text
  Numbers a = Numbers(1, 2.3, 4.5);         // GC-managed
  &Numbers b = new Numbers(1, 2.3, 4.5);    // manual, needs delete b;
  ```

- `release` (Rust-style ownership transfer) is an interesting idea, but ownership rules plus a GC is a big design
  topic. Park it until phase 8.
- **Structs**: being a value type means *copied on assignment*. It doesn't mean "no methods": C#, Rust and Swift
  structs all have methods. "Public data only" is still a valid choice (C does it that way), just not a required one.
  Also, `open key;` without an initializer has nothing to infer from. That's what generics are for
  (`struct Dict<K, V>`), which you already mentioned as a later step.
- `20e4` (in `types.mq`) is scientific notation. The lexer supports it as a `FLOAT_LIT`.

## What was added this round

- `samples/v0-example.mq`: uses only v0 features (`int`, `bool`, functions, `if`/`else if`/`else`, `while`,
  `print`, `@main`).
- `tests/lex/*.mq` + `*.tokens`: every sample copied, with its token dump in the improved format (`line:col`, one
  token per string or char literal, `IDENT` for `print`, `KW_` prefixes, `EOF` at the end). These are the golden
  files your phase 2 lexer has to reproduce. The originals now copied into `tests/` stay frozen while the samples
  keep changing.
- `resources/lex-first-attempt/`: your original hand-written `arith.tokens` and `hello.tokens`, moved here
  unchanged so you keep them for the report. They'd fail as tests now.
- `Writerside/topics/grammar.md`: the lexical grammar (all tokens) and the v0 syntactic grammar.
- `resources/ebnf.md`: what EBNF is, how to read the grammar, and how to grow it. Section 8 is your part.
