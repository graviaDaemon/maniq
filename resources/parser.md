# Parser: getting `v0-example.ast` to pass

Goal of this step: `maniq parse samples/v0-example.mq` prints exactly what is in `tests/parse/v0-example.ast`. You
already wrote that file by hand, so you know what every node looks like. This step turns that tree into C++ types and
writes the code that builds it from tokens.

Like the lexer guide, this gives you the shape and the order. The bodies are yours, and each section ends with a check.

Background reading: Crafting Interpreters ch. 5 "Representing Code" (the tree) and ch. 6 "Parsing Expressions" (the
ladder). Ch. 8 and 9 cover statements, `if` and `while`. Skip ch. 7 (it evaluates, which an AOT compiler doesn't do).

---

## 1. What the parser does

The lexer turned characters into words. The parser turns words into sentences: it reads the token list from left to
right and builds the tree you drew.

```text
input:   KW_RETURN  IDENT n  PERCENT  INT_LIT 2  EQUAL_EQUAL  INT_LIT 0  SEMICOLON
output:  Return
             Binary ==
                 Binary %
                     Name n
                     Int 2
                 Int 0
```

The parser only checks **form**. `return x;` in a `void` function, an undeclared name, or `true + 1`: all of these
parse fine. Catching them is phase 4's job. If the parser starts checking meaning, stop.

## 2. The node types

Open your `.ast` file and list every node name you used. You'll find three families:

| Family          | Nodes                                                   | What they are             |
|-----------------|---------------------------------------------------------|---------------------------|
| **Expressions** | `Int`, `Bool`, `Name`, `Unary`, `Binary`, `Call`        | produce a value           |
| **Statements**  | `Variable`, `Assign`, `If`, `While`, `Return`, `Block`  | do something              |
| **Declaration** | `Function`                                              | the only top-level thing  |

`Parameter`, `Type` and `Returns` aren't nodes of their own. They're **fields** of `Function` and `Variable` that the
dump prints as children. That's the first design lesson: the dump is a view of the tree, not the tree itself.

There's one statement your `.ast` file doesn't show: the **expression statement** (`countDown(3);`). The grammar has
`exprStmt`, so the C++ needs a type for it. But you didn't write an `ExprStmt` line in the dump, and that's fine. The
dump prints only its expression. (Same lesson.)

### 2a. How to represent "one of several kinds"

An `Expr` is an `Int` *or* a `Binary` *or* a `Call`... C++ gives you two ways to say "or":

- **A class hierarchy:** a base `struct Expr` with `virtual ~Expr()`, and `struct Binary : Expr`. To do something per
  kind you write a virtual function, or a visitor. This is what Crafting Interpreters does (in Java).
- **`std::variant`:** each kind is a plain struct, and `Expr` holds a `std::variant<Int, Bool, Name, ...>`. To do
  something per kind, you call `std::visit`.

Recommended: **`std::variant`**. The structs stay plain data, and `std::visit` refuses to compile if you forget a kind,
just as `-Wall` warned about a missing `case` in `to_string(TokenKind)`. Either works though. Pick one and use it for
both families.

### 2b. Recursion needs pointers

`Binary` contains two expressions, and an expression can be a `Binary`. A struct can't contain itself by value (it would
be infinitely big), so children are held through a pointer:

```cpp
struct Expr;
using ExprPtr = std::unique_ptr<Expr>;
```

`std::unique_ptr` means "I own this, and it's deleted when I am." The tree owns its children, the children own theirs,
and deleting the root frees everything. No `delete` anywhere in your code.

Lists of children (a block's statements, a call's arguments, a function's parameters) are a `std::vector`. A
`std::vector<StmtPtr>` is fine. For things that aren't recursive, like `Parameter`, a plain `std::vector<Parameter>`
is enough.

> **Think about it:** `Return` has an *optional* expression (`return;` is legal in v0). `If` has an optional `else`. How
> do you store "maybe a child"? (Hint: what is a `unique_ptr` that owns nothing?)

### 2c. What each node stores

Go through the grammar rule by rule. Each rule's parts become the node's fields, in the same order as in your dump:

```text
ifStmt = "if" "(" expression ")" block [ "else" ( ifStmt | block ) ] ;
         ──── ─── ────────── ─── ───── ─────────────────────────────
         gone gone condition gone then  else (optional, If or Block)
```

Keywords and punctuation disappear; the meaningful parts stay. Do this for every node before writing any parser code.

For names, operators and literals, store a `std::string_view` (the token's text) or the whole `Token`. Storing the
`Token` keeps `line`/`col` around, which phase 4 needs for its error messages ("`count` is not declared at 3:9").

> **Think about it:** every `string_view` in the tree points into `Lexer::source`. What does that mean for how long the
> `Lexer` object must live? Where in `main` does it live today?

### 2d. Where it goes

`src/ast.h`, for the node types only. No `.cpp` is needed for plain structs. Add it to `add_executable` in
`CMakeLists.txt`.

**Check:** in `main`, build the tree for `return n % 2 == 0;` by hand, with `std::make_unique`. It compiles.

## 3. The dump

Write the dump **before** the parser. Then you can test it on the hand-built tree from §2's check, and when the parser
comes, any difference from the golden file is a parser bug and not a printing bug.

The format, from your `.ast` file:

- one node per line: the node name, then its text if it has any (`Name count`, `Binary >`, `Function max`);
- each level of depth indents **4 spaces**;
- a **blank line between functions**, but not after the last one;
- the file ends with a newline (your golden file currently doesn't: add one at the end).

The natural shape is a recursive function that takes a node and a depth, prints its own line, and calls itself on each
child with `depth + 1`. You need one for expressions and one for statements, plus one for a function.

With `std::variant`, each one is a `std::visit` with one branch per kind. Look up the "overloaded" pattern
(on cppreference, the example on the `std::visit` page): it lets you write the branches as lambdas side by side.

> **Pitfall:** in `else if`, the else branch is an `If`, not a `Block`. If your dump only handles "else is a block",
> `@main` will come out wrong. Print whatever statement the else holds.

**Check:** dumping your hand-built `return n % 2 == 0;` matches lines 26–31 of the golden file, apart from the indent.

## 4. The parser's state: another cursor

The parser walks the token list once, left to right, exactly as the lexer walked the characters. Same idea, one level
up:

| Field    | Meaning                                  |
|----------|------------------------------------------|
| `tokens` | the whole token list                     |
| `pos`    | index of the next token to read          |

And the same small helpers, now over tokens:

- **`peek()`**: the current token, without moving. The list always ends in `END_OF_FILE`, so `peek()` never runs off
  the end, as long as `advance()` never steps *past* the `EOF` token.
- **`advance()`**: return the current token and move on.
- **`check(kind)`**: is the current token of this kind? It doesn't move.
- **`match(kind)`**: if `check(kind)`, advance and return true. It's the same helper as the lexer's `match(char)`.
- **`expect(kind, message)`**: the current token *must* be this kind. Advance and return it, or report an error
  (§7). Most of the parser is calls to this one.

**Check:** a loop over `samples/v0-example.mq` that calls `advance()` until `END_OF_FILE` and counts tokens gets one less than the
number of lines in `tests/lex/v0-example.tokens` (that file also lists `EOF`).

## 5. One function per grammar rule

This is the whole trick, and the reason the grammar was worth writing. **Each grammar rule becomes one function**, and
the function's body reads like the rule:

| In the grammar          | In the function                                      |
|-------------------------|------------------------------------------------------|
| `"("` (a literal token) | `expect(LEFT_PAREN, ...)`                            |
| `block` (another rule)  | call `parse_block()`                                 |
| `[ ... ]` (optional)    | `if (match(...))` or `if (check(...))`               |
| `{ ... }` (repeat)      | `while (check(...))`                                 |
| `a \| b` (choice)       | `if`/`switch` on `peek().kind` to decide which       |

Take `function` from the v0 grammar:

```text
function = "@" IDENT "(" [ parameters ] ")" "<<" returnType block ;
```

That's: expect `@`, expect an `IDENT` (keep it, it's the name), expect `(`, if the next token *isn't* `)` then parse
parameters, expect `)`, expect `<<`, parse the return type, parse a block. Build a `Function` from the pieces you kept
and return it. The code is as long as the rule, and no longer.

This style is called **recursive descent**: `parse_block` calls `parse_statement`, which can call `parse_block` again.
The C++ call stack mirrors the shape of the tree.

### Choosing a statement

`statement` is a choice of seven. Most start with a token that gives them away:

| Next token         | Statement    |
|--------------------|--------------|
| `int`, `bool`      | `varDecl`    |
| `if`               | `ifStmt`     |
| `while`            | `whileStmt`  |
| `return`           | `returnStmt` |
| `{`                | `block`      |

Two are left: `assignment` (`count = count - 1;`) and `exprStmt` (`countDown(3);`). Both start with an `IDENT`. One
token isn't enough to tell them apart.

> **Think about it:** what does the token *after* the `IDENT` tell you? Your `peek()` may need an offset, like the
> lexer's `peek(std::size_t offset)`.

## 6. The expression ladder

This part matched the hardest bit of your hand-drawn tree. The rules again:

```text
expression = or ;
or         = and { "||" and } ;
...
term       = factor { ( "+" | "-" ) factor } ;
factor     = unary { ( "*" | "/" | "%" ) unary } ;
unary      = ( "!" | "-" ) unary | call ;
call       = primary [ "(" [ arguments ] ")" ] ;
primary    = INT_LIT | "true" | "false" | IDENT | "(" expression ")" ;
```

Same rule as §5: one function per level. Each level calls the one below it. That's where precedence comes from:
`term` calls `factor` first, so `*` has already grabbed its operands before `+` gets a look.

### Left associativity is in the loop

Look at `term = factor { ( "+" | "-" ) factor }`. In words:

```text
left = parse a factor
while the next token is + or -:
    op = that token
    right = parse a factor
    left = a new Binary(left, op, right)     ← the old left becomes a child
return left
```

The marked line is the whole answer to your `6 / 2 + negative` puzzle. Each time round the loop, the tree built so far
is pushed *down* into the left child of a new node, so the **last** operator ends up on top. Trace
`(2 + 3) * 4 - 6 / 2 + negative` through this loop on paper, and you'll get the `Binary +` at the top that you drew.

> **Pitfall:** `left` is a `unique_ptr`. Putting it into the new node is a **move** (`std::move(left)`), and after
> that `left` is empty until you assign the new node to it. The compiler will tell you if you try to copy it.

### The bottom levels

- `unary` calls **itself** for the operand (`!!x`, `--x`). It's the one level that recurses into itself rather than
  looping, and that makes it right-associative.
- `primary` ends the climb. `"(" expression ")"` goes back to the top of the ladder, which is why parentheses need no
  node of their own: they just restart the climb.
- `call`: after a primary, if the next token is `(`, it's a call. The primary you just parsed is the callee.

> The roadmap mentions **Pratt parsing**. It does the same job as this ladder but with a precedence table instead of
> one function per level. Write the ladder first: it's a 1:1 copy of the grammar you already understand. Switch to Pratt
> later, when the ladder has ten levels and gets tedious. That will happen in the full grammar, not in v0.

## 7. Errors

The roadmap's format is `file:line:col: message`, e.g. `v0-example.mq:3:7: expected ';'`. The position is the token
where the parser got stuck: `peek()`'s `line` and `col`.

For v0, **stop at the first error**. When `expect` fails, throw an exception that carries the token and the message.
Catch it in `main`, print it to `std::cerr`, and return `1`. (The lexer has a `runtime_error` catch there already.)

Real compilers try to recover and report more errors in one run. That's called *synchronization* (CI ch. 6.3.3). Skip
it for now.

Also handle `ERROR` tokens from the lexer. An unterminated string reaches the parser as an `ERROR` token, and the parser
should report it as an error rather than "expected `;`".

> **Think about it:** the error message should say what was found, too. `expected ';'` is fine; `expected ';', found
> 'print'` is better. You have `peek().text`.

**Check:** remove a `;` from a copy of the sample. You get one line on stderr pointing at the right line and column,
and exit code 1.

## 8. `main` and the test

`maniq parse <file>`: read, lex, parse, dump. It's the same `argv[1]` check as `lex`.

For the CTest side, copy the `lex` block in `CMakeLists.txt`: glob `tests/parse/*.ast`, and run `maniq parse` against
`samples/<name>.mq`. `tests/lex.cmake` hard-codes the `lex` command. Either copy it to `tests/parse.cmake` or pass the
command in as one more `-D` variable. The second option is one script for both.

To check by hand:

```bash
./cmake-build-debug/maniq.exe parse samples/v0-example.mq > out.ast
diff --strip-trailing-cr out.ast tests/parse/v0-example.ast && echo PASS
```

Add `out.ast` to `.gitignore`, like `out.tokens`.

## 9. Order of work

Don't try to parse `v0-example.mq` in one go. Grow the parser with tiny scratch files (not committed) that only use
what you've built:

| Step | Scratch input                           | You add                                      |
|------|-----------------------------------------|----------------------------------------------|
| 1    | `@main() << void {}`                    | `program`, `function`, empty `block`, dump   |
| 2    | `@f(int a, bool b) << int {}`           | parameters, `returnType`                     |
| 3    | `@f() << int { return 1; }`             | `returnStmt`, `primary` (just `INT_LIT`)     |
| 4    | `... return 1 + 2 * 3; ...`             | the ladder from `term` down                  |
| 5    | `... int x = 1; x = x - 1; ...`         | `varDecl`, `assignment`, the `IDENT` lookahead |
| 6    | `... f(1, 2); ...`                      | `call`, `arguments`, `exprStmt`              |
| 7    | `... if (...) {} else if (...) {} ...`  | `ifStmt`, `whileStmt`, the rest of the ladder |
| 8    | `samples/v0-example.mq`                 | nothing new, hopefully: the golden test      |

After step 1 your dump already prints `Function main`, `Returns void`, `Block`. Every step after that adds one rule and
is visible right away in the output.

---

## Summary checklist

- [ ] node types in `src/ast.h`: expressions, statements, `Function`, using `unique_ptr` for children
- [ ] dump works on a hand-built tree, with 4-space indent and a blank line between functions
- [ ] `Parser` with `tokens`, `pos` and `peek` / `advance` / `check` / `match` / `expect`
- [ ] `function`, `parameters`, `block`
- [ ] all seven statements, including the assignment-vs-call lookahead
- [ ] the ladder, one function per level, left-associative loops
- [ ] errors as `file:line:col: message`, first error stops, exit code 1
- [ ] `maniq parse <file>` and a CTest per `tests/parse/*.ast`
- [ ] `diff` against `v0-example.ast` passes
