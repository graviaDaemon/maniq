#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "diagnostic.h"

enum class TokenKind {
    IDENT, INT_LIT, FLOAT_LIT, DECIMAL_LIT, CHAR_LIT, STR_LIT, INTERP_STR_LIT, PATH_LIT,

    KW_INT, KW_FLOAT, KW_DECIMAL, KW_BOOL, KW_CHAR, KW_STR, KW_VOID, KW_OPEN,
    KW_TRUE, KW_FALSE, KW_IF, KW_ELSE, KW_FOR, KW_FOREACH, KW_IN, KW_WHILE,
    KW_DO, KW_RETURN, KW_CLASS, KW_STRUCT, KW_DELETE, KW_RELEASE, KW_FROM,

    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, LEFT_BRACKET, RIGHT_BRACKET,
    SEMICOLON, COMMA, DOT, DOT_DOT, ARROW,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQUAL, EQUAL_EQUAL, BANG, BANG_EQUAL, HASH,
    LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, LESS_LESS,
    AND_AND, OR_OR, AMP, AT, DOLLAR,

    ERROR,
    END_OF_FILE
};

std::string_view to_string(TokenKind kind);

struct Token {
    TokenKind kind;
    std::string_view text;
    int line;
    int col;
};

Diagnostic lex_error(const Token& token);

class Lexer {
public:
    void read(const std::string& path);

    std::vector<Token> lex();

private:
    std::string source;
    std::size_t pos = 0;
    int line = 1;
    int col = 1;

    [[nodiscard]] bool at_end() const;
    [[nodiscard]] char peek(std::size_t offset = 0) const;
    char advance();
    bool match(char expected);

    void skip_whitespace_and_comments(std::vector<Token>& tokens);
    TokenKind quoted(char quote);
    TokenKind number();
    TokenKind identifier();
};
