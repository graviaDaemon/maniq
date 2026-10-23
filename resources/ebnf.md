# EBNF: what it is and how to use it for Maniq

The grammar itself is in `Writerside/topics/grammar.md`. This file explains how to read it, how it maps to the
parser you'll write, and how to grow it.

## 1. What a grammar is

A grammar is a set of rules that says which sequences of tokens form a valid program, and how they group. Your
samples show *examples* of Maniq. A grammar describes *every* valid Maniq program in a page or two.

Each rule gives a name to a pattern:

```text
whileStmt = "while" "(" expression ")" block ;
```

Read it as: "a `whileStmt` is the word `while`, then `(`, then an `expression`, then `)`, then a `block`."

- **Terminals** are the actual pieces of text, written in quotes (`"while"`, `"("`) or as a token class in capitals
  (`IDENT`, `INT_LIT`). They come straight from the lexer.
- **Nonterminals** are names of other rules (`expression`, `block`). They get replaced by what their rule says.

## 2. BNF and EBNF

**BNF** (Backus–Naur Form, 1960, made for ALGOL) has only rules and alternatives. Repetition has to be written as
recursion:

```text
<args> ::= <expr> | <expr> "," <args>
```

**EBNF** (*Extended* BNF) adds shorthand for repetition and optional parts. It describes the same languages, only
shorter:

```text
args = expr { "," expr } ;
```

There are many EBNF dialects. The one used for Maniq is close to the ISO standard (ISO/IEC 14977):

| Notation        | Meaning                     | Example                        |
|-----------------|-----------------------------|--------------------------------|
| `a = ... ;`     | defines rule `a`            | `type = "int" \| "bool" ;`     |
| `"x"`           | literal text (terminal)     | `"while"`                      |
| `a b`           | `a` followed by `b`         | `"(" expression ")"`           |
| `a \| b`        | `a` or `b`                  | `"true" \| "false"`            |
| `[ a ]`         | `a` is optional (0 or 1)    | `[ "else" block ]`             |
| `{ a }`         | `a` repeated 0 or more times| `{ statement }`                |
| `( a \| b )`    | grouping                    | `( "+" \| "-" ) factor`        |
| `(* ... *)`     | comment                     | `(* not a keyword *)`          |
| `"a" .. "z"`    | character range (shorthand, not ISO) | `digit = "0" .. "9" ;` |

## 3. Two grammars: lexical and syntactic

- The **lexical grammar** works on *characters* and produces tokens: what an `IDENT`, an `INT_LIT` or a
  `STR_LIT` looks like. The lexer (phase 2) implements it.
- The **syntactic grammar** works on *tokens* and describes the program structure. The parser (phase 3) implements it.

Keeping them apart is why whitespace and comments never appear in the syntactic rules: the lexer has already thrown
them away.

## 4. Deriving a program by hand

"Derivable" means you can start at `program` and keep replacing nonterminals until you get exactly the tokens of the
source. Take `count = count - 1;` from `samples/v0-example.mq`:

```text
statement
→ assignment                                  (IDENT followed by "=")
→ IDENT "=" expression ";"
→ count "=" or ";"
→ count "=" and ";"   → ... → count "=" term ";"
→ count "=" factor "-" factor ";"             (term = factor { ("+"|"-") factor })
→ count "=" unary "-" unary ";"
→ count "=" call "-" call ";"
→ count "=" primary "-" primary ";"
→ count "=" IDENT "-" INT_LIT ";"
→ count = count - 1 ;                         ✓
```

The roadmap's "done when" for phase 1 asks you to do this for each v0 sample. Try it on
`return n % 2 == 0;`. You'll find that `%` binds tighter than `==`, which is exactly what you want.

## 5. How the grammar encodes precedence

Why does `1 + 2 * 3` mean `1 + (2 * 3)`? Because of how the rules are layered:

```text
term   = factor { ( "+" | "-" ) factor } ;
factor = unary  { ( "*" | "/" | "%" ) unary } ;
```

A `term` is made of `factor`s joined by `+`. A `factor` is made of `unary`s joined by `*`. So `2 * 3` has to be
complete as one `factor` before `term` ever sees the `+`:

```text
            term
     ┌───────┼────────┐
  factor    "+"     factor
     │          ┌─────┼─────┐
     1        unary  "*"  unary
                │           │
                2           3
```

**Rule of thumb:** the lower a rule sits in the list, the tighter it binds. Adding a new operator means picking the
level it belongs to, or adding a new level between two existing ones.

## 6. From grammar to parser (recursive descent)

Each rule becomes one function in the parser, and each piece of notation becomes a piece of control flow:

| Grammar         | Parser code                                   |
|-----------------|-----------------------------------------------|
| rule `a`        | function `parseA()`                           |
| terminal `"x"`  | `expect(X)`: consume it, or report an error   |
| nonterminal `b` | call `parseB()`                               |
| `{ ... }`       | `while (current token starts it) { ... }`     |
| `[ ... ]`       | `if (current token starts it) { ... }`        |
| `a \| b`        | `if`/`switch` on the current token            |

For example, `term = factor { ( "+" | "-" ) factor } ;` becomes, in pseudo-code:

```text
parseTerm():
    left = parseFactor()
    while current is PLUS or MINUS:
        op = advance()
        right = parseFactor()
        left = Binary(left, op, right)
    return left
```

This is why the grammar is worth writing carefully: a parser for a clean grammar is almost mechanical to write.
*Crafting Interpreters* ch. 6 does exactly this.

## 7. Two traps the grammar avoids

**Left recursion.** You might write `term = term "+" factor | factor ;`. It's valid BNF, but `parseTerm()` would
first call `parseTerm()`, which calls `parseTerm()`, and so on forever. The `{ }` form says the same thing without
that problem.

**Ambiguity.** A grammar is ambiguous when one input can be derived in two ways. The classic case is the "dangling
else" in C: `if (a) if (b) x(); else y();` — which `if` owns the `else`? Maniq avoids it by requiring a block after
`if`, so the braces always decide.

**Lookahead.** Sometimes the current token is not enough to pick an alternative. `count = 1;` and `count(1);` both
start with `IDENT`. The parser peeks at the *next* token (`=` or `(`) to decide. Keep that to one extra token where
you can; the grammar notes in Writerside point out where it's needed.

## 8. Growing the grammar (your part)

The v0 grammar is a starting point. Every feature in "Not in v0 yet" goes through the same steps:

1. Take the sample lines that use the feature.
2. Find where it fits: is it a new statement, a new type, a new expression level, or a new kind of `primary`?
3. Write the rule. Check that the first token of each alternative is different, or note where lookahead is needed.
4. Check it doesn't break anything: does an existing sample now derive in two ways?
5. Derive the sample lines by hand.
6. Record the choice in `Writerside/`.

A good first exercise: add `for (i in 0..n) { ... }`. You'll have to decide whether `0..n` is an *expression* (so
`open r = 0..10;` works) or only allowed inside `for`, and where `..` sits in the precedence list.

## Further reading

- *Crafting Interpreters* ch. 5 "Representing Code" (grammars, derivations) and ch. 6 "Parsing Expressions"
  (precedence, recursive descent): https://craftinginterpreters.com/representing-code.html
- Wikipedia, "Extended Backus–Naur form": https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form
- Real-world example: the Go specification writes its whole grammar in a similar EBNF:
  https://go.dev/ref/spec#Notation
