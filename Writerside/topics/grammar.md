# Grammar

The formal description of Maniq's syntax, written in EBNF. It has two layers:

- **Lexical grammar**: how characters group into tokens. It covers every token used in the samples, because the lexer
  is built for the whole language at once.
- **Syntactic grammar**: how tokens group into a program. For now it covers **v0 only**: `int`, `bool`, functions,
  `if`/`else`, `while`, `print`. Other features are added one at a time (roadmap phase 7).

Notation: `=` defines a rule, `;` ends it, `|` separates alternatives, `{ x }` means zero or more `x`, `[ x ]` means
optional `x`, `( ... )` groups. `"while"` is literal text. UPPERCASE names (`IDENT`, `INT_LIT`) are token classes from
the lexical grammar. `"a" .. "z"` is shorthand for "any character from a to z".

## Lexical grammar

### Whitespace and comments
Spaces, tabs, `\r` and `\n` separate tokens and are otherwise ignored. Newlines are not tokens; statements end with `;`.
A leading UTF-8 BOM is skipped. `\r\n` counts as one line break.

```ebnf
lineComment  = "//" { any character except newline } ;
blockComment = "/*" { any character } "*/" ;      (* also covers /** doc comments */ *)
```

### Tokens

```ebnf
letter       = "a" .. "z" | "A" .. "Z" | "_" ;
digit        = "0" .. "9" ;

IDENT        = letter { letter | digit } ;        (* unless it is a keyword *)
INT_LIT      = digit { digit } ;
FLOAT_LIT    = digit { digit } ( "." digit { digit } [ exponent ] | exponent ) ;
exponent     = ( "e" | "E" ) [ "+" | "-" ] digit { digit } ;
DECIMAL_LIT  = digit { digit } [ "." digit { digit } ] ( "m" | "M" ) ;

escape       = "\" any character ;
CHAR_LIT     = "'" ( escape | any character except "'" "\" newline ) "'" ;
STR_LIT      = '"' { escape | any character except '"' "\" newline } '"' ;
INTERP_STR_LIT = "$" STR_LIT ;
PATH_LIT     = "<" { any character except ">" newline } ">" ;   (* only directly after "from" *)
```

Rules the lexer applies on top of these:

- **Longest match.** At each position, take the longest token that fits: `<=` is one token, not `<` then `=`. `<<`,
  `==`, `&&`, `..` work the same way.
- **Keywords.** Lex an identifier first, then look it up in the keyword table. `while` becomes `KW_WHILE`, `whilex`
  stays `IDENT`.
- **Numbers and `..`.** A `.` only continues a number when a digit follows it. So `0..10` is `INT_LIT DOT_DOT INT_LIT`
  and `3.5` is one `FLOAT_LIT`.
- **`$`.** `$"` starts an interpolated string. Any other `$` is `DOLLAR` (the constructor marker in `$(...)`).
- **Paths.** `<` directly after `from` starts a `PATH_LIT`, as with C's `#include <stdio.h>`. Anywhere else `<` is
  `LESS`.
- **Errors.** An unknown character or a malformed literal (such as `'struct'`) becomes an `ERROR` token, and lexing
  continues. The parser reports it.

### Keywords

| Token          | Text      | Token          | Text      | Token          | Text      |
|----------------|-----------|----------------|-----------|----------------|-----------|
| `KW_INT`       | `int`     | `KW_TRUE`      | `true`    | `KW_DO`        | `do`      |
| `KW_FLOAT`     | `float`   | `KW_FALSE`     | `false`   | `KW_RETURN`    | `return`  |
| `KW_DECIMAL`   | `decimal` | `KW_IF`        | `if`      | `KW_CLASS`     | `class`   |
| `KW_BOOL`      | `bool`    | `KW_ELSE`      | `else`    | `KW_STRUCT`    | `struct`  |
| `KW_CHAR`      | `char`    | `KW_FOR`       | `for`     | `KW_DELETE`    | `delete`  |
| `KW_STR`       | `str`     | `KW_FOREACH`   | `foreach` | `KW_RELEASE`   | `release` |
| `KW_VOID`      | `void`    | `KW_IN`        | `in`      | `KW_FROM`      | `from`    |
| `KW_OPEN`      | `open`    | `KW_WHILE`     | `while`   |                |           |

`use` and `define` are not keywords. `#use` is the token `HASH` followed by `IDENT use`, so the names stay free for
normal code. `print` is an `IDENT` too.

### Punctuation and operators

| Token           | Text | Token          | Text | Token           | Text |
|-----------------|------|----------------|------|-----------------|------|
| `LEFT_PAREN`    | `(`  | `PLUS`         | `+`  | `LESS`          | `<`  |
| `RIGHT_PAREN`   | `)`  | `MINUS`        | `-`  | `LESS_EQUAL`    | `<=` |
| `LEFT_BRACE`    | `{`  | `STAR`         | `*`  | `GREATER`       | `>`  |
| `RIGHT_BRACE`   | `}`  | `SLASH`        | `/`  | `GREATER_EQUAL` | `>=` |
| `LEFT_BRACKET`  | `[`  | `PERCENT`      | `%`  | `LESS_LESS`     | `<<` |
| `RIGHT_BRACKET` | `]`  | `EQUAL`        | `=`  | `AND_AND`       | `&&` |
| `SEMICOLON`     | `;`  | `EQUAL_EQUAL`  | `==` | `OR_OR`         | `\|\|` |
| `COMMA`         | `,`  | `BANG`         | `!`  | `AMP`           | `&`  |
| `DOT`           | `.`  | `BANG_EQUAL`   | `!=` | `AT`            | `@`  |
| `DOT_DOT`       | `..` | `HASH`         | `#`  | `DOLLAR`        | `$`  |
| `ARROW`         | `->` | | | | |

Every token dump ends with an `EOF` token.

### Token dump format
Golden files in `tests/lex/*.tokens` use one token per line: `line:col`, token kind, then the exact source text.
Columns are 1-based and count characters; the BOM is not counted. Files use LF line endings.

```text
1:1     KW_STR          str
1:5     IDENT           hello
1:11    EQUAL           =
1:13    STR_LIT         "Hello, world!"
1:28    SEMICOLON       ;
```

## Syntactic grammar (v0)

```EBNF
program     = { function } EOF ;

function    = "@" IDENT "(" [ parameters ] ")" "<<" returnType block ;
parameters  = parameter { "," parameter } ;
parameter   = type IDENT ;
returnType  = type | "void" ;
type        = "int" | "bool" ;

block       = "{" { statement } "}" ;

statement   = varDecl
            | assignment
            | ifStmt
            | whileStmt
            | returnStmt
            | exprStmt
            | block ;

varDecl     = type IDENT [ "=" expression ] ";" ;
assignment  = IDENT "=" expression ";" ;
ifStmt      = "if" "(" expression ")" block [ "else" ( ifStmt | block ) ] ;
whileStmt   = "while" "(" expression ")" block ;
returnStmt  = "return" [ expression ] ";" ;
exprStmt    = expression ";" ;

expression  = or ;
or          = and { "||" and } ;
and         = equality { "&&" equality } ;
equality    = comparison { ( "==" | "!=" ) comparison } ;
comparison  = term { ( "<" | "<=" | ">" | ">=" ) term } ;
term        = factor { ( "+" | "-" ) factor } ;
factor      = unary { ( "*" | "/" | "%" ) unary } ;
unary       = ( "!" | "-" ) unary | call ;
call        = primary [ "(" [ arguments ] ")" ] ;
arguments   = expression { "," expression } ;
primary     = INT_LIT | "true" | "false" | IDENT | "(" expression ")" ;
```

## Syntactic grammar (full - in progress)

```EBNF
program     = { declaration } EOF ;

declaration = function | structDecl | classDecl | directive ;
function    = "@" IDENT "(" [ parameters ] ")" "<<" returnType block ;
lambda      = "(" [ parameters ] ")" "->" block ;
(* %params should always be last, since it defines 'any number of additional parameters' *)
parameters  = variadic | parameter { "," parameter } [ "," variadic ] ;
parameter   = type IDENT ;
variadic    = "%" IDENT ;
returnType  = type | "void" ;
type        = [ "&" ] baseType { "[" "]" } ;
baseType    = "float" | "decimal" | "int" | "bool" | "str" | "char" ;

block       = "{" { statement } "}" ;

statement   = varDecl
            | assignment
            | ifStmt
            | whileStmt
            | forStmt
            | foreachStmt
            | doWhileStmt
            | returnStmt
            | exprStmt
            | block ;

varDecl     = ( type | "open" ) IDENT [ "=" expression ] ";" ;
structDecl  = "struct" IDENT "{" [ construct ] { field } "}" ;
classDecl   = "class" IDENT "{" { member } "}" ;
constructor = "$" "(" [ parameters ] ")" block;
field       = [ accessor ] type IDENT [ "=" expression ] ";" ;
method      = [ accessor ] function ;
accessor    = ( "-" | "+" | "*" ) ;
member      = field | method | constructor ;
assignment  = IDENT "=" expression ";" ;
ifStmt      = "if" "(" expression ")" block [ "else" ( ifStmt | block ) ] ;
whileStmt   = "while" "(" expression ")" block ;
forStmt     = "for" "(" IDENT "in" range ")" block ;
foreachStmt = "foreach" "(" IDENT "in" expression ")" block ; (* the expression here must be iterable *)
doWhileStmt = "do" block "while" "(" expression ")" ";" ;
returnStmt  = "return" [ expression ] ";" ;
exprStmt    = expression ";" ;

expression  = or ;
or          = and { "||" and } ;
and         = equality { "&&" equality } ;
equality    = comparison { ( "==" | "!=" ) comparison } ;
comparison  = range { ( "<" | "<=" | ">" | ">=" ) range } ;
range       = term [ ".." term ] ;
term        = factor { ( "+" | "-" ) factor } ;
factor      = unary { ( "*" | "/" | "%" ) unary } ;
unary       = ( "!" | "-" ) unary | call ;
call        = primary { "(" [ arguments ] ")" | "." IDENT | "[" expression "]" } ;  (* index must be an int: checked in phase 4 *)
arguments   = expression { "," expression } ;
primary     = INT_LIT | FLOAT_LIT | DECIMAL_LIT | CHAR_LIT | STR_LIT | INTERP_STR_LIT | "true" | "false" | IDENT | "(" expression ")" | "[" [ arguments ]"]" | lambda ;
```

### Notes on the v0 grammar

- **Entry point.** A program has exactly one `@main() << void`, in the file passed to `maniq build`. Other files only
  hold declarations. v0 has no top-level statements.
- **Precedence** comes from the layering, from loosest to tightest: `||`, `&&`, `==` `!=`, `<` `<=` `>` `>=`, `+` `-`,
  `*` `/` `%`, unary `!` `-`, call. Each level is left-associative: `10 - 2 - 3` is `(10 - 2) - 3`.
- **Assignment is a statement, not an expression.** `if (x = 1)` is a syntax error instead of a silent bug.
- **Blocks are required** after `if`, `else` and `while`. That removes the "dangling else" ambiguity.
- **`else if`** is `else` followed by another `ifStmt`. No separate keyword is needed.
- **Lookahead.** `assignment` and `exprStmt` can both start with `IDENT`. The parser looks one token further: `=`
  means an assignment.
- **The grammar allows more than the language does.** `1 < 2 < 3`, `5()` and `true + 1` all parse. The type checker
  (phase 4) rejects them, which keeps the grammar small.
- **`print`** is a built-in, overloaded for `int` and `bool`. To the parser it's an ordinary call.

### Not in v0 yet

Seen in the samples, to be added to the syntactic grammar one feature at a time:

- the types `float`, `decimal`, `char`, `str`, `open`, and array types `T[]`
- literals for floats, chars, strings, interpolated strings and arrays (`[1, 2, 3]`)
- member access `a.b`, indexing `a[i]`, ranges `a..b`
- `for (i in range)`, `foreach (x in list)`, `do { } while (...);`
- `struct`, `class`, constructors `$(...)`, access markers `-` `+` `*`
- directives `#use ... from <...>;` and `#define name value;` (Arduino-like; constants come later)
- manual memory: `&T`, `delete`, `release`
- variadic parameters `%params`
