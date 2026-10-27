# errorcodes

| Range | Stage | Example |
|-------|-------|---------|
| MQ1xxxx | lexer | `MQ10001`: Unterminated string |
| MQ2xxxx | parser | `MQ20001`: expected ';', found '}' |
| MQ3xxxx | checker | `MQ30001`: undeclared variable ('step') |
| MQ4xxxx | code generation | comes later |
| MQ5xxxx | build | comes later |
| MQ6xxxx | runtime | comes later |
| MQ7xxxx | warnings | `MQ70002`: unused variable of type 'int' ('a') |

`maniq build` prints errors and warnings to stderr, location first. The position is the node at fault, at its first
token (an operator for a binary expression). The first error stops the build with exit code 1. Warnings don't stop the
build, and a clean build prints nothing.

```text
bad.mq:3:13
    ==> Checker-Error[MQ30001]: undeclared variable ('b')
bad.mq:2:5
    ==> Warning[MQ70002]: unused variable of type 'int' ('a')
```

## Lexer (MQ1xxxx)

The lexer never stops: it emits an `ERROR` token, and the error is reported when the parser reaches that token.

| Code      | Message (example)                                    | Cause                                                   |
|-----------|------------------------------------------------------|---------------------------------------------------------|
| `MQ10001` | `unterminated string literal`                        | A string, char or path literal isn't closed on its line. |
| `MQ10002` | `unterminated block comment`                         | A `/*` without a matching `*/`.                         |
| `MQ10003` | `invalid character ('\|')`                           | A character that isn't part of any token.               |
| `MQ10004` | `a char literal holds exactly one character ('ab')`  | A char literal that is empty or has more than one character. |

## Parser (MQ2xxxx)

| Code      | Message (example)                                    | Cause                                                   |
|-----------|------------------------------------------------------|---------------------------------------------------------|
| `MQ20001` | `expected ';', found 'print'`                        | The next token doesn't fit the grammar (also: expected a type, an expression). |
| `MQ20002` | `integer out of range, found '99999999999'`          | An integer literal doesn't fit in an `int`.             |

## Checker (MQ3xxxx)

| Code      | Message (example)                                            | Cause                                                        |
|-----------|--------------------------------------------------------------|--------------------------------------------------------------|
| `MQ30001` | `undeclared variable ('step')` or `undeclared function ('step')` | A variable or function is used but never declared, or used outside its scope. |
| `MQ30002` | `name already defined ('count')`                             | A name is declared twice where both are visible: a variable, a parameter, a function, or the built-in `print`. |
| `MQ30003` | `cannot use operator ('+') on types 'int' and 'bool'`        | An operator is used on types it doesn't accept.              |
| `MQ30004` | `function 'max' expected 2 arguments, got 1`                 | A call has the wrong number of arguments.                    |
| `MQ30005` | `not all paths return a value (function 'max')`              | A non-`void` function can end without a `return`.            |
| `MQ30006` | `cannot use type 'bool' for variable 'a', expected type 'int'` | A value of the wrong type in an initializer, assignment, argument or condition. |
| `MQ30007` | `cannot return type 'bool' for function 'f', expected type 'int'` | A `return` doesn't match the function's return type.    |
| `MQ30008` | `missing entry-point function ('main')`                      | There is no `@main`, or it has the wrong shape.              |
| `MQ30009` | `only a call can be used as a statement`                     | An expression like `1 + 2;` stands alone and does nothing.   |

## Warnings (MQ7xxxx)

| Code      | Message (example)                                            | Cause                                                        |
|-----------|--------------------------------------------------------------|--------------------------------------------------------------|
| `MQ70001` | `unused return value of type 'int' ('max')`                  | A function's return value is ignored.                        |
| `MQ70002` | `unused variable of type 'int' ('a')`                        | A variable or parameter is declared but never used.          |
