# Semantic analysis: checking what the tree means

Goal of this step: `maniq build <file>` accepts every correct v0 program and rejects every wrong one with one clear,
located error. For a correct program it writes the tree with a type on every expression.

The parser only checked **form**. All of these parse fine today, and all of them are wrong:

```text
@main() << void {
    print(x);              // x doesn't exist
    int a = true;          // a bool in an int
    bool b = 1 + false;    // + on a bool
    max(1);                // max takes two arguments
    return 5;              // main returns void
}
```

Catching these is this phase. Like the other guides, this gives you the shape and the order; the bodies are yours, and
each section ends with a check.

Background reading: Crafting Interpreters ch. 11 "Resolving and Binding" for scopes (skip the parts about closures
and the interpreter). CI doesn't cover type checking, because Lox is dynamic: §6 below covers it.

---

## 0. Decide first: the rules

A checker enforces rules, so the rules have to exist first. v0 leaves a few open. Answer these before writing code;
each answer is one `if` somewhere in the checker. Recommendations are in brackets.

| #  | Question                                                                 | Recommended                         |
|----|--------------------------------------------------------------------------|-------------------------------------|
| 1  | Can a function call a function that is defined **below** it?             | yes (like C#, unlike C)             |
| 2  | May an inner block declare a name that an outer one already has (*shadowing*)? | no (like C#)                  |
| 3  | Two functions with the same name?                                        | error in v0; overloading comes later |
| 4  | `int x;` without a value: allowed? What's in it?                         | allowed, starts at `0` / `false`    |
| 5  | Must a non-`void` function return on every path?                         | yes, error otherwise                |
| 6  | `1 + 2;` as a statement (a value nobody uses)?                           | error: only calls can be statements |
| 7  | What does `print` accept?                                                | one `int` or one `bool`             |
| 8  | Rules for `@main`?                                                       | exactly one, `@main() << void`      |

Write your answers down (a `requests/` file is fine) so they end up in `plan/00-decisions.md`. The rest of this guide
assumes the recommendations; where yours differ, the change is small.

## 1. What the checker does

Two jobs, done in the same walk over the tree:

- **Names:** for every `Name` and `Call`, find the declaration it refers to. Not found → error.
- **Types:** work out the type of every expression, and check that it fits where it's used.

The checker doesn't build a new tree. It reads the parser's tree, writes a type onto each expression, and either
returns normally or reports an error.

```text
input:   Binary ==                   output:  Binary == : bool
             Binary %                             Binary % : int
                 Name n                               Name n : int
                 Int 2                                Int 2 : int
             Int 0                                Int 0 : int
```

That's the whole picture: same tree, plus answers.

## 2. Before you start: positions in the tree

An error needs a `line:col`, and right now the tree has none. The parser threw the tokens away after reading them.

Add `int line = 0;` and `int col = 0;` to the bases `Expression` and `Statement`, and to `Function` and `Parameter`.
Then, in the parser, set them from the node's **first** token: for `Binary`, that's the operator token (`a > b` is
"at the `>`"); for everything else, the token the rule starts with.

> **Think about it:** in `Parser::term()` the operator token is consumed by `advance()` inside the loop. Where do you
> grab its `line` and `col` before moving on? (The `binary(...)` helper in `parser.cpp` will need two more arguments.)

**Check:** `maniq parse` output and the parse test are unchanged. The dump doesn't print positions; nothing else
should notice.

## 3. Types as values

The tree stores types as text (`"int"`, `"bool"`, `"void"`). The checker compares types all the time, so give them a
real C++ type:

```cpp
enum class Type { Int, Bool, Void };
```

Write two small helpers: one turns the parser's `string` into a `Type`, one turns a `Type` back into text for error
messages and the dump.

Each `Expression` gets a field for its result type. It needs a value for "not checked yet": add an `Unknown` member to
the enum, or use `std::optional<Type>`. Put the enum in `ast.h` so both the checker and the dump can see it.

## 4. Scopes: where names live

A **scope** is the set of names visible at one point. A block opens a new one, and the closing `}` throws it away:

```text
@countDown(int start) << void {      scope 1: start
    int count = start;               scope 1: start, count
    while (count > 0) {              scope 2 opens (empty)
        int step = 1;                scope 2: step
    }                                scope 2 closes: step is gone
    print(step);                     error: step doesn't exist here
}
```

The data structure is a **stack of maps**: `vector<unordered_map<string, Type>>`. Four operations:

| Operation      | What it does                                                                 |
|----------------|------------------------------------------------------------------------------|
| `push()`       | `emplace_back()` an empty map. At the start of every `Block`.                |
| `pop()`        | `pop_back()`. At the end of every `Block`.                                   |
| `declare(n,t)` | Add `n` to the **innermost** map. Error if it's already visible (rule 2).    |
| `lookup(n)`    | Search from the **innermost** map outwards. Not found → error.               |

Parameters go in a scope of their own, opened before the function's body block.

> **Pitfall:** `int count = count + 1;` must say "count doesn't exist". Check the value **before** you declare the
> name, so the value can't see it.

**Check:** on paper, run the four operations over `@countDown` from `samples/v0-example.mq`. How many maps are on the
stack when `count = count - 1;` is checked? (Answer: three, the parameter scope, the body block and the while block.)

## 5. Functions: two passes

Rule 1 says `@main` may call a function defined below it. When the checker reaches that call, it hasn't seen the
function yet. The fix is two passes over `Program`:

1. **Collect:** for every `Function`, store its *signature*, the parameter types and the return type, in a map from
   name to signature. A duplicate name is an error (rule 3). No bodies are looked at.
2. **Check:** walk each body, with the full map available.

Functions live in their own map, not in the scope stack: in v0 a name is either a function or a variable, and you
always know which one you need (`Call` → function, `Name` → variable).

`print` isn't declared anywhere. It's **built in**: handle it inside the `Call` check, before the map lookup. It
takes one argument of type `int` or `bool` and returns `void`.

`@main` (rule 8): after pass 1, check that the map has a `main`, with no parameters and `void` as its return type.
Report it at `1:1` if it's missing.

**Check:** a file with two `@f` gets an error at the second one; a file without `@main` gets an error.

## 6. Checking expressions

One function, `Type check(Expression&)`, that returns the expression's type **and** writes it onto the node. It has
the same shape as `dump`: one branch per node kind, children first. A node's type comes from its children's types,
so the work goes bottom-up, just like building the tree by hand did.

| Node                | Rule                                                                          | Type   |
|---------------------|-------------------------------------------------------------------------------|--------|
| `Int`               | always fine                                                                   | `int`  |
| `Bool`              | always fine                                                                   | `bool` |
| `Name`              | `lookup` its name                                                             | the variable's type |
| `Unary -`           | operand is `int`                                                              | `int`  |
| `Unary !`           | operand is `bool`                                                             | `bool` |
| `Binary + - * / %`  | both `int`                                                                    | `int`  |
| `Binary < <= > >=`  | both `int`                                                                    | `bool` |
| `Binary == !=`      | both the **same** type, and not `void`                                        | `bool` |
| `Binary && \|\|`    | both `bool`                                                                   | `bool` |
| `Call`              | function exists, argument count matches, each argument's type matches its parameter | the return type |

A `void` call is the one way an expression can be `void`, and a `void` value can't be used for anything:
`int x = countDown(3);` and `print(countDown(3))` are both errors. The table catches most of these by itself (`void`
isn't `int`); the `==` row needs the explicit "not `void`".

> **Think about it:** `Binary` stores `op` as a `string`, and `Unary` as a `char`. A `switch` won't work on the
> `string`. What's the shortest way to sort the ten binary operators into the four rows of the table?

**Check:** hand-check `(2 + 3) * 4 - 6 / 2 + negative` from `@main`: every node is `int`. Then `even && biggest > 5`:
`Name even` is `bool`, the `>` is `bool`, the `&&` is `bool`.

## 7. Checking statements

A second function, `void check(Statement&)`, again one branch per kind. It returns nothing: statements have no value.

| Statement  | Rule                                                                                         |
|------------|----------------------------------------------------------------------------------------------|
| `Variable` | if it has a value: check it, and its type equals the declared type. Then `declare` the name.  |
| `Assign`   | `lookup` the name; the value's type equals the variable's type.                              |
| `If`       | condition is `bool`; check the body, and the else (an `If` or a `Block`) if there is one.     |
| `While`    | condition is `bool`; check the body.                                                         |
| `Return`   | the value's type equals the function's return type. `return;` only in `void`; `return x;` never in `void`. |
| `Block`    | `push`, check each statement, `pop`.                                                         |
| `ExprStmt` | the expression must be a `Call` (rule 6). Check it; its type can be anything, including `void`. |

`Return` needs to know which function it's in. Keep the current function's return type in a member of the checker,
set before each body is checked.

**Check:** the whole of `samples/v0-example.mq` checks without an error.

## 8. Does every path return?

Rule 5. `@max` is fine: the `if` returns `a`, and if it doesn't run, `return b;` does. Remove `return b;` and there is
a path (`a <= b`) that falls off the end of the function, returning nothing.

This is a small recursive question, separate from the type check: *does this statement always return?*

| Statement  | Always returns when...                                     |
|------------|------------------------------------------------------------|
| `Return`   | always                                                     |
| `Block`    | any of its statements always returns                       |
| `If`       | it has an else, **and** both the body and the else always return |
| everything else | never (a `while` might run zero times)                |

For every non-`void` function: if its body doesn't always return, report an error at the function.

> **Think about it:** why is "any statement in the block" enough, and not "the last one"? What does that say about
> statements written after a `return`? (Those are *unreachable*. Some languages warn about it; v0 can ignore it.)

**Check:** `@f(int a) << int { if (a > 0) { return 1; } }` is an error; adding `else { return 0; }` fixes it.

## 9. Errors

Every stage reports the same way: the checker throws a `Diagnostic{code, line, col, message}` (`src/diagnostic.h`),
`main` prints it location first, and the first error stops with exit code 1. The position is the node at fault, at its
first token (a binary expression at its operator). The codes and the format are in `Writerside/topics/errorcodes.md`.

Good messages say what was expected and what was found:

```text
bad.mq:13:12
    ==> Checker-Error[MQ30003]: cannot use operator ('+') on types 'int' and 'bool'
bad.mq:27:19
    ==> Checker-Error[MQ30004]: function 'max' expected 2 arguments, got 1
```

The parser had to stop at the first error, because after a syntax error the tokens no longer line up. A checker
*could* keep going, and real compilers do. The catch is cascades: one undeclared `x` makes every expression that uses
`x` fail too. Stopping at the first error avoids that for now.

## 10. `main` and the tests

`maniq build <file>`: read, lex, parse, check. It's the same `argv[1]` pattern as `parse`, and the name is the one the
command keeps once it produces an executable. It writes `out.tokens`, `out.ast` and, if checking succeeds,
`out.types`; errors and warnings go to stderr, and a clean build prints nothing.

The typed tree: `dump` ends an expression line in ` : int` once its type is known, and prints it as before while it's
unknown, so `maniq parse` stays the same. Only expressions get a type (`Int`, `Bool`, `Name` as a use, `Unary`,
`Binary`, `Call`); the `Type`/`Name` lines under `Parameter`, `Variable`, `Assign` and `Call` are labels. This is the
output from §1.

Every test reads `samples/<name>.mq`:

- **Valid programs:** `tests/types/<name>.types` holds the expected typed tree. Make it by copying
  `tests/parse/<name>.ast` and adding the types by hand: the same exercise as writing the `.ast` file, one level up.
  A program with an error has no `.types` file, because the checker stops before the tree is fully typed.
- **Errors and warnings:** `tests/err/<name>.err` holds what `maniq build` prints to stderr (empty for a clean
  build). The samples are named `v0-mq<code>.mq` or `v0-mq<code>-<case>.mq`, with one mistake per file.

`tests/golden.cmake` has a mode per kind. For `.err` it captures stderr and expects exit code 1 exactly when the
output contains `Error[`; for `.types` it runs `build` in its own folder and compares `out.types`.

> **Pitfall:** the error line contains the path you passed on the command line. CTest passes an absolute path, which
> differs per machine, so the test script strips the directory before comparing.

## 11. Order of work

Grow it with scratch files again, one rule at a time:

| Step | Scratch input                                          | You add                                   |
|------|--------------------------------------------------------|-------------------------------------------|
| 1    | (any file)                                             | positions in the tree (§2), `Type` enum   |
| 2    | `@main() << void {}`                                   | pass 1, the `@main` rule, `maniq build`   |
| 3    | `... int x = 1; print(x); ...`                         | scopes, `Variable`, `Name`, `Int`, `print` |
| 4    | `... int x = true; ...`                                | the first type error                      |
| 5    | `... x = x + 2 * 3; ...`                               | `Assign`, the `Binary` table, `Unary`     |
| 6    | `@f(int a) << int { return a; }` + a call from main    | parameters, `Call`, `Return`              |
| 7    | `if` / `while` with conditions                         | `If`, `While`, the `bool` checks          |
| 8    | `@f(int a) << int { if (a > 0) { return 1; } }`        | every-path-returns (§8)                   |
| 9    | `samples/v0-example.mq`                                | typed dump, golden tests                  |

---

## Summary checklist

- [x] the eight rules from §0 decided and written down
- [x] `line`/`col` on every node, set by the parser
- [x] `enum class Type`, with a type field on `Expression`
- [x] scope stack: `push` / `pop` / `declare` / `lookup`
- [x] pass 1 collects signatures; duplicate names and `@main` checked
- [x] `check(Expression&)` following the table in §6, `print` built in
- [x] `check(Statement&)` following the table in §7
- [x] every-path-returns for non-`void` functions
- [x] errors as coded diagnostics, location first, exit code 1
- [x] `maniq build <file>`, typed dump, `.types` and `.err` golden tests
