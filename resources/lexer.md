# Lexer: getting `hello.tokens` to pass

Goal of this step: `maniq lex samples/hello.mq` prints exactly what is in `tests/lex/hello.tokens`. Nothing more. You
don't build the whole lexer yet. You build the part `hello.mq` needs, and the other golden files tell you what to add
next.

This guide gives you the shape and the order. The bodies are yours. Each section ends with a check, so you always
know whether you're done with it.

Background reading: Crafting Interpreters ch. 4 "Scanning" (see `README.md`). It uses Java, but the loop is the same.

---

## 1. What the lexer does

The lexer reads characters and produces a list of tokens. That's all. It doesn't know what a statement is. It only
knows that `str` is a keyword, `hello` is a name and `=` is an equals sign.

```text
input:   str hello = "Hello, world!";
output:  KW_STR  IDENT  EQUAL  STR_LIT  SEMICOLON
```

Look at `tests/lex/hello.tokens`. It uses only ten token kinds:

`KW_STR`, `KW_CHAR`, `IDENT`, `EQUAL`, `STR_LIT`, `CHAR_LIT`, `SEMICOLON`, `LEFT_PAREN`, `RIGHT_PAREN`, `EOF`

That's your target for this step.

## 2. The `Token` struct

A token is four facts: *what* it is, *which text* it covers, and *where* it starts.

```cpp
struct Token {
    TokenKind kind;
    ??? text;
    int line;
    int col;
};
```

There are two decisions to make here.

### 2a. `kind`: an `enum class`

```cpp
enum class TokenKind {
    KwStr, KwChar, Ident, Equal, /* ... */ Eof,
};
```

Use `enum class`, not a plain `enum`: its names don't leak into the surrounding scope, and it doesn't silently turn
into an `int`. Start with the ten kinds from §1. You add the rest when a later golden file needs them.

How you spell the names (`KW_STR` or `KwStr`) is up to you. The dump needs the text `KW_STR` regardless, which is §5.

### 2b. `text`: `std::string` or `std::string_view`?

This is the interesting one.

- `std::string` **copies** the characters into the token. It's simple and always safe.
- `std::string_view` is a **pointer + length** into the original source. Nothing is copied. But the source string
  must stay alive as long as the tokens do. If it's freed, every token points at garbage (a *dangling* view).

Compilers almost always use the view: the source file is loaded once and lives for the whole compilation anyway.
Recommended: `std::string_view`, and keep the source in one `std::string` that you own in `main`.

> **Think about it:** if you write `lex(read_file(path))`, where does the file's string live, and when does it die?
> What happens to the tokens afterwards?

### 2c. Where does `Token` go?

In `src/lexer.h`, together with `TokenKind`. Your current `class lexer {}` is empty. Section 3 fills it in.

**Check:** `main.cpp` can `#include "lexer.h"` and create a `Token` by hand. It compiles.

## 3. The lexer's state: a cursor

The lexer walks through the source once, from left to right. At any moment it needs to know:

| Field    | Meaning                                       |
|----------|-----------------------------------------------|
| `source` | the whole file (a `std::string_view`)         |
| `pos`    | index of the next character to read           |
| `line`   | current line, starting at 1                   |
| `col`    | current column, starting at 1                 |

That is the class. Something like:

```cpp
class Lexer {
public:
    explicit Lexer(std::string_view source);
    std::vector<Token> tokenize();

private:
    // the four fields above
    // small helpers (see below)
};
```

A free function `std::vector<Token> lex(std::string_view)` that uses the class internally is fine too. Pick one.

### The helpers

Almost everything in a lexer is built from three tiny functions. Write these first:

- **`peek()`**: return the current character *without* moving. Return `'\0'` at the end of the file, so callers never
  read past the end.
- **`advance()`**: return the current character *and* move forward one. This is the **only** place that changes
  `pos`, `line` and `col`. If the character was `\n`, go to the next line and reset the column.
- **`at_end()`**: `pos >= source.size()`.

Keep line/column tracking in `advance()` only. If you update `col` in five places, one of them will be wrong.

> **Think about it:** `hello.mq` has CRLF line endings (`\r\n`). If `\n` moves to a new line, what should `\r` do?
> (Hint: does it matter what `col` becomes, if the `\n` right after it resets `col` anyway?)

**Check:** a test loop that calls `advance()` until `at_end()` and prints `line:col` for every character ends on line
4 of `hello.mq`.

## 4. The main loop

`tokenize()` is a loop. Each turn produces one token (or skips whitespace):

```text
skip a leading BOM, once
while not at end:
    skip whitespace
    if at end: stop
    remember start = pos, start_line = line, start_col = col
    c = advance()
    decide what token starts with c
    push Token{ kind, source.substr(start, pos - start), start_line, start_col }
push the EOF token
```

Two things to notice:

1. The token's position is where it **starts**, so you save `line`/`col` *before* consuming it.
2. The token's text is just `source.substr(start, pos - start)`. Because `text` is a view, this costs nothing. You
   never build the text character by character.

"Decide what token starts with `c`" is usually a `switch (c)`. Now add the cases one at a time, in this order.

### Step 1: the BOM and whitespace

`hello.mq` starts with three bytes, `EF BB BF` (the UTF-8 BOM). The grammar says to skip it, and the dump says the
first token is at `1:1`, so the BOM doesn't count as a column either. Skip it *before* the loop, without going through
`advance()`.

Whitespace is space, `\t`, `\r`, `\n`: advance past it and produce nothing.

### Step 2: single-character tokens

`(`, `)`, `;` each map straight to a kind. One `case` each.

`=` is special, even though `hello.mq` only uses the single `=`. Later files have `==`. This is the **longest match**
rule from the grammar: after reading `=`, `peek()` at the next character. If it's another `=`, consume it too and make
`EQUAL_EQUAL`. You don't need `EQUAL_EQUAL` for hello, but it's good to see where it will go.

> A helper `match(char expected)` ("if the next char is `expected`, consume it and return true") makes all
> two-character operators one line each. You'll want it soon.

### Step 3: identifiers and keywords

An identifier starts with a letter or `_` and continues with letters, digits and `_`. So in the `default:` branch of
the switch: if `c` is a letter, keep calling `advance()` while `peek()` is a letter or a digit.

Then look at the text you just read. If it's a keyword, the kind is that keyword (`str` → `KW_STR`); otherwise it's
`IDENT`. The grammar calls this *lex an identifier first, then look it up*. This is why `print` comes out as `IDENT`
without any special case.

For the lookup, a `static const std::unordered_map<std::string_view, TokenKind>` works. So does a plain chain of
`if`s for now. Only `str` and `char` matter for hello.

> **Pitfall:** `std::isalpha(c)` with a plain `char` whose value is above 127 (such as the BOM bytes, or `é`) is
> undefined behaviour, because `char` is signed on GCC. Cast first: `std::isalpha(static_cast<unsigned char>(c))`.
> Or write your own `is_letter(char)` that checks `a`-`z`, `A`-`Z` and `_`, which matches the grammar exactly.

### Step 4: string literals

`"` starts a string. Keep advancing until the closing `"`. The token text **includes both quotes**: look at the dump,
`STR_LIT "Hello, world!"`.

Two edge cases to handle now, because they're cheap:

- **Escapes:** a `\` means "the next character is part of the string, whatever it is", so `"a\"b"` doesn't end at
  the middle quote. When you see `\`, advance twice.
- **Unterminated:** if you hit a newline or the end of the file before the closing `"`, produce an `ERROR` token and
  keep going. Don't crash, and don't throw.

### Step 5: char literals

`'W'` works like a string, but with `'`. Here too the text includes the quotes. Per the grammar, a char literal is
exactly one character (or one escape). Something like `'struct'` is an `ERROR`. For hello you only need the happy
path, but think about where the check would go.

### Step 6: EOF

After the loop, push one more token: kind `EOF`, empty text, at the current `line`/`col`.

Look at the last line of `hello.tokens`: `4:10 EOF`. Line 4 is `print(w);`, which is 9 characters, so the cursor
ends at column 10. That only comes out right if your `advance()` bookkeeping is right, which makes it a good free test.

**Check:** print `tokens.size()` for `hello.mq`. It should be 21.

## 5. The dump

The golden file format (from `grammar.md`):

```text
1:1     KW_STR          str
4:10    EOF
```

- `line:col`, left-aligned and padded to **8** characters;
- the kind's name, left-aligned and padded to **16** characters;
- the text.

You need a function that turns a `TokenKind` into its name: `std::string_view to_string(TokenKind)`, a `switch` with
one `return "KW_STR";` per case. Enable `-Wall`: GCC then warns when you add a kind and forget its case.

For the padding, C++20's `std::format` can do it: `{:<8}` means "left-align, width 8". Build the `line:col` string
first, then pad it.

> **Pitfall, look closely at the EOF line.** `4:10    EOF` has **no trailing spaces**. If you always pad the kind to
> 16, the EOF line ends in 13 spaces, and the diff fails on a line that looks identical. Only pad the kind when there's
> text after it.

## 6. `main`

For now, `main` does one thing: `maniq <file>` reads the file, lexes it and prints the dump. (A real command line,
`maniq build`, `maniq lex`, comes later. Don't design it yet.)

Reading the file:

```cpp
std::ifstream in(path, std::ios::binary);
std::string source{std::istreambuf_iterator<char>(in), {}};
```

Open it in **binary** mode. In text mode on Windows, the C++ runtime quietly turns `\r\n` into `\n`. The output
would then look right on your machine and break on Linux, where nobody converts. Binary mode means your lexer sees the
real bytes on every platform, and *your* code handles `\r`.

`source` lives in `main` for the whole run, so every `string_view` in the tokens stays valid (this answers §2b).

## 7. Checking against the golden file

From the repo root, in Git Bash:

```bash
./cmake-build-debug/maniq.exe lex samples/hello.mq > out.tokens
diff --strip-trailing-cr out.tokens tests/lex/hello.tokens && echo PASS
```

All golden files at once: `ctest --test-dir cmake-build-debug --output-on-failure`, or CLion's CTest run
configuration. Each `tests/lex/<name>.tokens` is checked against `samples/<name>.mq`.

No output plus `PASS` means you're done. Any difference is printed line by line, with `<` for yours and `>` for the
expected one.

Why `--strip-trailing-cr`? On Windows, `std::cout` also writes in text mode, so every `\n` you print becomes `\r\n`.
The golden file has LF. That difference doesn't matter; the test decision already says comparison ignores CRLF. The
flag makes `diff` agree. Don't commit `out.tokens`; delete it or add it to `.gitignore`.

## 8. After hello

Once hello passes, the other golden files give you the order. Each one forces something new:

| File                 | Forces you to add                                                                    |
|----------------------|--------------------------------------------------------------------------------------|
| `arith.tokens`       | comments, `INT_LIT`, `FLOAT_LIT`, `+ - * / %`, `,`, keywords `int float decimal open` |
| `types.tokens`       | `{ } [ ]`, `.`, `&`, `#`, `PATH_LIT` after `from`, `$"..."` interpolated strings      |
| `conditional.tokens` | `DECIMAL_LIT` (the `m` suffix), `..` vs `.`, `<` `>`, the loop and `if`/`else` keywords |
| `func.tokens`        | `@`, `<<` (your first two-character operator: write `match()` here)                  |
| `struct.tokens`      | `$` on its own, `class`, `struct`                                                     |
| `v0-example.tokens`  | `==`, `<=`, `&&`, `\|\|`, `!`, `false`                                                |

Every file after hello has comments, so `//` and `/* */` come right after hello. That order is a suggestion. Open each `.mq` next to its `.tokens` and decide for yourself what's new. One file at a
time, and re-run the earlier files each time, because a change for `arith` can break `hello`.

---

## Summary checklist

- [x] `TokenKind` (`enum class`) with the ten kinds from §1
- [x] `Token` with `kind`, `text` (`string_view`), `line`, `col`
- [x] `Lexer` with `source`, `pos`, `line`, `col` and `peek` / `advance` / `at_end`
- [x] BOM skipped; whitespace (including `\r`) skipped
- [x] `( ) ; =` as single-character tokens
- [x] identifiers, plus the keyword lookup for `str` and `char`
- [x] string and char literals, quotes included, escapes skipped, unterminated → `ERROR`
- [x] `EOF` at `4:10`
- [x] `to_string(TokenKind)` and the dump format, with no trailing spaces on `EOF`
- [x] `main` reads the file in binary mode and prints the dump
- [x] `diff` against `hello.tokens` passes
