#pragma once
#include <string_view>
#include <vector>

#include "ast.h"
#include "lexer.h"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    Program parse();

private:
    std::vector<Token> tokens;
    std::size_t pos = 0;

    [[nodiscard]] const Token& peek(std::size_t offset = 0) const;
    const Token& advance();
    [[nodiscard]] bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    const Token& expect(TokenKind kind, std::string_view what);
    [[noreturn]] void error(std::string_view message, std::string_view code = "MQ20001") const;

    unique_ptr<Function> function();
    Parameter parameter();
    std::string type();
    unique_ptr<Block> block();
    unique_ptr<Statement> statement();
    unique_ptr<Statement> variable();
    unique_ptr<Statement> assignment();
    unique_ptr<If> if_statement();
    unique_ptr<Statement> while_statement();
    unique_ptr<Statement> return_statement();

    unique_ptr<Expression> expression();
    unique_ptr<Expression> logical_or();
    unique_ptr<Expression> logical_and();
    unique_ptr<Expression> equality();
    unique_ptr<Expression> comparison();
    unique_ptr<Expression> term();
    unique_ptr<Expression> factor();
    unique_ptr<Expression> unary();
    unique_ptr<Expression> call();
    unique_ptr<Expression> primary();
};
