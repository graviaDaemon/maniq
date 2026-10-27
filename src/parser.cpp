#include "parser.h"

#include <charconv>
#include <format>
#include <stdexcept>

using namespace std;

namespace {
    unique_ptr<Expression> binary(const Token& op, unique_ptr<Expression> left, unique_ptr<Expression> right) {
        auto node = make_unique<Binary>();
        node->line = op.line;
        node->col = op.col;
        node->op = op.text;
        node->left = std::move(left);
        node->right = std::move(right);
        return node;
    }
}

Parser::Parser(vector<Token> tokens) : tokens(std::move(tokens)) {}

Program Parser::parse() {
    Program program;
    while (!check(TokenKind::END_OF_FILE)) {
        program.push_back(function());
    }
    return program;
}

const Token& Parser::peek(const size_t offset) const {
    return tokens[min(pos + offset, tokens.size() - 1)];
}

const Token& Parser::advance() {
    const Token& token = peek();
    if (token.kind != TokenKind::END_OF_FILE) {
        ++pos;
    }
    return token;
}

bool Parser::check(const TokenKind kind) const {
    return peek().kind == kind;
}

bool Parser::match(const TokenKind kind) {
    if (!check(kind)) {
        return false;
    }
    advance();
    return true;
}

const Token& Parser::expect(const TokenKind kind, const string_view what) {
    if (!check(kind)) {
        error(format("expected {}", what));
    }
    return advance();
}

void Parser::error(const string_view message, const string_view code) const {
    const Token& token = peek();
    if (token.kind == TokenKind::ERROR) {
        throw lex_error(token);
    }
    const string found = token.kind == TokenKind::END_OF_FILE ? "end of file" : format("'{}'", token.text);
    throw Diagnostic{string(code), token.line, token.col, format("{}, found {}", message, found)};
}

unique_ptr<Function> Parser::function() {
    auto node = make_unique<Function>();
    expect(TokenKind::AT, "'@'");
    node->line = peek().line;
    node->col = peek().col;
    node->name = expect(TokenKind::IDENT, "a function name").text;
    expect(TokenKind::LEFT_PAREN, "'('");
    if (!check(TokenKind::RIGHT_PAREN)) {
        do {
            node->parameters.push_back(parameter());
        } while (match(TokenKind::COMMA));
    }
    expect(TokenKind::RIGHT_PAREN, "')'");
    expect(TokenKind::LESS_LESS, "'<<'");
    node->returnType = check(TokenKind::KW_VOID) ? string(advance().text) : type();
    node->body = block();
    return node;
}

Parameter Parser::parameter() {
    Parameter node;
    node.line = peek().line;
    node.col = peek().col;
    node.type = type();
    node.name = expect(TokenKind::IDENT, "a parameter name").text;
    return node;
}

string Parser::type() {
    if (!check(TokenKind::KW_INT) && !check(TokenKind::KW_BOOL)) {
        error("expected a type");
    }
    return string(advance().text);
}

unique_ptr<Block> Parser::block() {
    auto node = make_unique<Block>();
    node->line = peek().line;
    node->col = peek().col;
    expect(TokenKind::LEFT_BRACE, "'{'");
    while (!check(TokenKind::RIGHT_BRACE) && !check(TokenKind::END_OF_FILE)) {
        node->body.push_back(statement());
    }
    expect(TokenKind::RIGHT_BRACE, "'}'");
    return node;
}

unique_ptr<Statement> Parser::statement() {
    switch (peek().kind) {
        case TokenKind::KW_INT:
        case TokenKind::KW_BOOL:    return variable();
        case TokenKind::KW_IF:      return if_statement();
        case TokenKind::KW_WHILE:   return while_statement();
        case TokenKind::KW_RETURN:  return return_statement();
        case TokenKind::LEFT_BRACE: return block();
        default: break;
    }
    if (check(TokenKind::IDENT) && peek(1).kind == TokenKind::EQUAL) {
        return assignment();
    }
    auto node = make_unique<ExprStmt>();
    node->line = peek().line;
    node->col = peek().col;
    node->expr = expression();
    expect(TokenKind::SEMICOLON, "';'");
    return node;
}

unique_ptr<Statement> Parser::variable() {
    auto node = make_unique<Variable>();
    node->line = peek().line;
    node->col = peek().col;
    node->type = type();
    node->name = expect(TokenKind::IDENT, "a variable name").text;
    if (match(TokenKind::EQUAL)) {
        node->value = expression();
    }
    expect(TokenKind::SEMICOLON, "';'");
    return node;
}

unique_ptr<Statement> Parser::assignment() {
    auto node = make_unique<Assign>();
    node->line = peek().line;
    node->col = peek().col;
    node->name = advance().text;
    advance();
    node->value = expression();
    expect(TokenKind::SEMICOLON, "';'");
    return node;
}

unique_ptr<If> Parser::if_statement() {
    auto node = make_unique<If>();
    node->line = peek().line;
    node->col = peek().col;
    expect(TokenKind::KW_IF, "'if'");
    expect(TokenKind::LEFT_PAREN, "'('");
    node->condition = expression();
    expect(TokenKind::RIGHT_PAREN, "')'");
    node->body = block();
    if (match(TokenKind::KW_ELSE)) {
        if (check(TokenKind::KW_IF)) {
            node->elseBody = if_statement();
        } else {
            node->elseBody = block();
        }
    }
    return node;
}

unique_ptr<Statement> Parser::while_statement() {
    auto node = make_unique<While>();
    node->line = peek().line;
    node->col = peek().col;
    expect(TokenKind::KW_WHILE, "'while'");
    expect(TokenKind::LEFT_PAREN, "'('");
    node->condition = expression();
    expect(TokenKind::RIGHT_PAREN, "')'");
    node->body = block();
    return node;
}

unique_ptr<Statement> Parser::return_statement() {
    auto node = make_unique<Return>();
    node->line = peek().line;
    node->col = peek().col;
    expect(TokenKind::KW_RETURN, "'return'");
    if (!check(TokenKind::SEMICOLON)) {
        node->value = expression();
    }
    expect(TokenKind::SEMICOLON, "';'");
    return node;
}

unique_ptr<Expression> Parser::expression() {
    return logical_or();
}

unique_ptr<Expression> Parser::logical_or() {
    auto left = logical_and();
    while (check(TokenKind::OR_OR)) {
        const Token& op = advance();
        left = binary(op, std::move(left), logical_and());
    }
    return left;
}

unique_ptr<Expression> Parser::logical_and() {
    auto left = equality();
    while (check(TokenKind::AND_AND)) {
        const Token& op = advance();
        left = binary(op, std::move(left), equality());
    }
    return left;
}

unique_ptr<Expression> Parser::equality() {
    auto left = comparison();
    while (check(TokenKind::EQUAL_EQUAL) || check(TokenKind::BANG_EQUAL)) {
        const Token& op = advance();
        left = binary(op, std::move(left), comparison());
    }
    return left;
}

unique_ptr<Expression> Parser::comparison() {
    auto left = term();
    while (check(TokenKind::LESS) || check(TokenKind::LESS_EQUAL) ||
           check(TokenKind::GREATER) || check(TokenKind::GREATER_EQUAL)) {
        const Token& op = advance();
        left = binary(op, std::move(left), term());
    }
    return left;
}

unique_ptr<Expression> Parser::term() {
    auto left = factor();
    while (check(TokenKind::PLUS) || check(TokenKind::MINUS)) {
        const Token& op = advance();
        left = binary(op, std::move(left), factor());
    }
    return left;
}

unique_ptr<Expression> Parser::factor() {
    auto left = unary();
    while (check(TokenKind::STAR) || check(TokenKind::SLASH) || check(TokenKind::PERCENT)) {
        const Token& op = advance();
        left = binary(op, std::move(left), unary());
    }
    return left;
}

unique_ptr<Expression> Parser::unary() {
    if (check(TokenKind::BANG) || check(TokenKind::MINUS)) {
        auto node = make_unique<Unary>();
        node->line = peek().line;
        node->col = peek().col;
        node->op = advance().text[0];
        node->operand = unary();
        return node;
    }
    return call();
}

unique_ptr<Expression> Parser::call() {
    if (!check(TokenKind::IDENT) || peek(1).kind != TokenKind::LEFT_PAREN) {
        return primary();
    }
    auto node = make_unique<Call>();
    node->line = peek().line;
    node->col = peek().col;
    node->name = advance().text;
    advance();
    if (!check(TokenKind::RIGHT_PAREN)) {
        do {
            node->args.push_back(expression());
        } while (match(TokenKind::COMMA));
    }
    expect(TokenKind::RIGHT_PAREN, "')'");
    return node;
}

unique_ptr<Expression> Parser::primary() {
    switch (peek().kind) {
        case TokenKind::INT_LIT: {
            auto node = make_unique<Int>();
            node->line = peek().line;
            node->col = peek().col;
            const string_view text = peek().text;
            if (from_chars(text.data(), text.data() + text.size(), node->value).ec != errc{}) {
                error("integer out of range", "MQ20002");
            }
            advance();
            return node;
        }
        case TokenKind::KW_TRUE:
        case TokenKind::KW_FALSE: {
            auto node = make_unique<Bool>();
            node->line = peek().line;
            node->col = peek().col;
            node->value = advance().kind == TokenKind::KW_TRUE;
            return node;
        }
        case TokenKind::IDENT: {
            auto node = make_unique<Name>();
            node->line = peek().line;
            node->col = peek().col;
            node->name = advance().text;
            return node;
        }
        case TokenKind::LEFT_PAREN: {
            advance();
            auto node = expression();
            expect(TokenKind::RIGHT_PAREN, "')'");
            return node;
        }
        default:
            error("expected an expression");
    }
}
