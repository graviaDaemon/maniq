#include "lexer.h"

#include <fstream>
#include <stdexcept>
#include <unordered_map>

using namespace std;

namespace {
    bool is_letter(const char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
    bool is_digit(const char c) { return c >= '0' && c <= '9'; }
    bool is_line_end(const char c) { return c == '\n' || c == '\r'; }
    bool is_continuation(const char c) { return (static_cast<unsigned char>(c) & 0xC0) == 0x80; }

    const unordered_map<string_view, TokenKind> keywords = {
        {"int", TokenKind::KW_INT}, {"float", TokenKind::KW_FLOAT}, {"decimal", TokenKind::KW_DECIMAL},
        {"bool", TokenKind::KW_BOOL}, {"char", TokenKind::KW_CHAR}, {"str", TokenKind::KW_STR},
        {"void", TokenKind::KW_VOID}, {"open", TokenKind::KW_OPEN}, {"true", TokenKind::KW_TRUE},
        {"false", TokenKind::KW_FALSE}, {"if", TokenKind::KW_IF}, {"else", TokenKind::KW_ELSE},
        {"for", TokenKind::KW_FOR}, {"foreach", TokenKind::KW_FOREACH}, {"in", TokenKind::KW_IN},
        {"while", TokenKind::KW_WHILE}, {"do", TokenKind::KW_DO}, {"return", TokenKind::KW_RETURN},
        {"class", TokenKind::KW_CLASS}, {"struct", TokenKind::KW_STRUCT}, {"delete", TokenKind::KW_DELETE},
        {"release", TokenKind::KW_RELEASE}, {"from", TokenKind::KW_FROM},
    };
}

string_view to_string(const TokenKind kind) {
    switch (kind) {
        case TokenKind::IDENT:          return "IDENT";
        case TokenKind::INT_LIT:        return "INT_LIT";
        case TokenKind::FLOAT_LIT:      return "FLOAT_LIT";
        case TokenKind::DECIMAL_LIT:    return "DECIMAL_LIT";
        case TokenKind::CHAR_LIT:       return "CHAR_LIT";
        case TokenKind::STR_LIT:        return "STR_LIT";
        case TokenKind::INTERP_STR_LIT: return "INTERP_STR_LIT";
        case TokenKind::PATH_LIT:       return "PATH_LIT";
        case TokenKind::KW_INT:         return "KW_INT";
        case TokenKind::KW_FLOAT:       return "KW_FLOAT";
        case TokenKind::KW_DECIMAL:     return "KW_DECIMAL";
        case TokenKind::KW_BOOL:        return "KW_BOOL";
        case TokenKind::KW_CHAR:        return "KW_CHAR";
        case TokenKind::KW_STR:         return "KW_STR";
        case TokenKind::KW_VOID:        return "KW_VOID";
        case TokenKind::KW_OPEN:        return "KW_OPEN";
        case TokenKind::KW_TRUE:        return "KW_TRUE";
        case TokenKind::KW_FALSE:       return "KW_FALSE";
        case TokenKind::KW_IF:          return "KW_IF";
        case TokenKind::KW_ELSE:        return "KW_ELSE";
        case TokenKind::KW_FOR:         return "KW_FOR";
        case TokenKind::KW_FOREACH:     return "KW_FOREACH";
        case TokenKind::KW_IN:          return "KW_IN";
        case TokenKind::KW_WHILE:       return "KW_WHILE";
        case TokenKind::KW_DO:          return "KW_DO";
        case TokenKind::KW_RETURN:      return "KW_RETURN";
        case TokenKind::KW_CLASS:       return "KW_CLASS";
        case TokenKind::KW_STRUCT:      return "KW_STRUCT";
        case TokenKind::KW_DELETE:      return "KW_DELETE";
        case TokenKind::KW_RELEASE:     return "KW_RELEASE";
        case TokenKind::KW_FROM:        return "KW_FROM";
        case TokenKind::LEFT_PAREN:     return "LEFT_PAREN";
        case TokenKind::RIGHT_PAREN:    return "RIGHT_PAREN";
        case TokenKind::LEFT_BRACE:     return "LEFT_BRACE";
        case TokenKind::RIGHT_BRACE:    return "RIGHT_BRACE";
        case TokenKind::LEFT_BRACKET:   return "LEFT_BRACKET";
        case TokenKind::RIGHT_BRACKET:  return "RIGHT_BRACKET";
        case TokenKind::SEMICOLON:      return "SEMICOLON";
        case TokenKind::COMMA:          return "COMMA";
        case TokenKind::DOT:            return "DOT";
        case TokenKind::DOT_DOT:        return "DOT_DOT";
        case TokenKind::ARROW:          return "ARROW";
        case TokenKind::PLUS:           return "PLUS";
        case TokenKind::MINUS:          return "MINUS";
        case TokenKind::STAR:           return "STAR";
        case TokenKind::SLASH:          return "SLASH";
        case TokenKind::PERCENT:        return "PERCENT";
        case TokenKind::EQUAL:          return "EQUAL";
        case TokenKind::EQUAL_EQUAL:    return "EQUAL_EQUAL";
        case TokenKind::BANG:           return "BANG";
        case TokenKind::BANG_EQUAL:     return "BANG_EQUAL";
        case TokenKind::HASH:           return "HASH";
        case TokenKind::LESS:           return "LESS";
        case TokenKind::LESS_EQUAL:     return "LESS_EQUAL";
        case TokenKind::GREATER:        return "GREATER";
        case TokenKind::GREATER_EQUAL:  return "GREATER_EQUAL";
        case TokenKind::LESS_LESS:      return "LESS_LESS";
        case TokenKind::AND_AND:        return "AND_AND";
        case TokenKind::OR_OR:          return "OR_OR";
        case TokenKind::AMP:            return "AMP";
        case TokenKind::AT:             return "AT";
        case TokenKind::DOLLAR:         return "DOLLAR";
        case TokenKind::ERROR:          return "ERROR";
        case TokenKind::END_OF_FILE:    return "EOF";
    }
    return "UNKNOWN";
}

void Lexer::read(const string& path) {
    ifstream inputFile(path, ios::binary);

    if (!inputFile.is_open()) {
        throw runtime_error("Failed to open file: " + path);
    }

    source.assign(istreambuf_iterator<char>(inputFile), istreambuf_iterator<char>());
    pos = source.starts_with("\xEF\xBB\xBF") ? 3 : 0;
    line = 1;
    col = 1;
}

bool Lexer::at_end() const {
    return pos >= source.size();
}

char Lexer::peek(const size_t offset) const {
    return pos + offset < source.size() ? source[pos + offset] : '\0';
}

char Lexer::advance() {
    const char c = source[pos++];

    if (c == '\n') {
        ++line;
        col = 1;
    } else if (!is_continuation(c)) {
        ++col;
    }

    return c;
}

bool Lexer::match(const char expected) {
    if (at_end() || peek() != expected) {
        return false;
    }

    advance();
    return true;
}

void Lexer::skip_whitespace_and_comments(vector<Token>& tokens) {
    while (!at_end()) {
        const char c = peek();

        if (c == ' ' || c == '\t' || is_line_end(c)) {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            while (!at_end() && peek() != '\n') {
                advance();
            }
        } else if (c == '/' && peek(1) == '*') {
            const size_t start = pos;
            const int startLine = line;
            const int startCol = col;

            advance();
            advance();

            while (!at_end() && !(peek() == '*' && peek(1) == '/')) {
                advance();
            }

            if (at_end()) {
                tokens.push_back({TokenKind::ERROR, string_view(source).substr(start, 2), startLine, startCol});
                return;
            }

            advance();
            advance();
        } else {
            return;
        }
    }
}

TokenKind Lexer::quoted(const char quote) {
    int characters = 0;

    while (!at_end() && peek() != quote && !is_line_end(peek())) {
        if (advance() == '\\' && !at_end() && !is_line_end(peek())) {
            advance();
        }

        while (is_continuation(peek())) {
            advance();
        }

        ++characters;
    }

    if (!match(quote)) {
        return TokenKind::ERROR;
    }

    if (quote == '\'') {
        return characters == 1 ? TokenKind::CHAR_LIT : TokenKind::ERROR;
    }

    return TokenKind::STR_LIT;
}

TokenKind Lexer::number() {
    const auto digits = [this] {
        while (is_digit(peek())) {
            advance();
        }
    };

    digits();

    bool fraction = false;

    if (peek() == '.' && is_digit(peek(1))) {
        advance();
        digits();
        fraction = true;
    }

    const bool sign = peek(1) == '+' || peek(1) == '-';

    if ((peek() == 'e' || peek() == 'E') && is_digit(peek(sign ? 2 : 1))) {
        advance();

        if (sign) {
            advance();
        }

        digits();
        return TokenKind::FLOAT_LIT;
    }

    if (match('m') || match('M')) {
        return TokenKind::DECIMAL_LIT;
    }

    return fraction ? TokenKind::FLOAT_LIT : TokenKind::INT_LIT;
}

TokenKind Lexer::identifier() {
    const size_t start = pos - 1;

    while (is_letter(peek()) || is_digit(peek())) {
        advance();
    }

    const auto keyword = keywords.find(string_view(source).substr(start, pos - start));
    return keyword == keywords.end() ? TokenKind::IDENT : keyword->second;
}

vector<Token> Lexer::lex() {
    using enum TokenKind;

    vector<Token> tokens;

    while (true) {
        skip_whitespace_and_comments(tokens);

        if (at_end()) {
            break;
        }

        const size_t start = pos;
        const int startLine = line;
        const int startCol = col;
        const char c = advance();

        TokenKind kind;

        switch (c) {
            case '(': kind = LEFT_PAREN; break;
            case ')': kind = RIGHT_PAREN; break;
            case '{': kind = LEFT_BRACE; break;
            case '}': kind = RIGHT_BRACE; break;
            case '[': kind = LEFT_BRACKET; break;
            case ']': kind = RIGHT_BRACKET; break;
            case ';': kind = SEMICOLON; break;
            case ',': kind = COMMA; break;
            case '+': kind = PLUS; break;
            case '*': kind = STAR; break;
            case '/': kind = SLASH; break;
            case '%': kind = PERCENT; break;
            case '#': kind = HASH; break;
            case '@': kind = AT; break;
            case '.': kind = match('.') ? DOT_DOT : DOT; break;
            case '-': kind = match('>') ? ARROW : MINUS; break;
            case '=': kind = match('=') ? EQUAL_EQUAL : EQUAL; break;
            case '!': kind = match('=') ? BANG_EQUAL : BANG; break;
            case '>': kind = match('=') ? GREATER_EQUAL : GREATER; break;
            case '&': kind = match('&') ? AND_AND : AMP; break;
            case '|': kind = match('|') ? OR_OR : ERROR; break;
            case '<':
                if (!tokens.empty() && tokens.back().kind == KW_FROM) {
                    while (!at_end() && peek() != '>' && !is_line_end(peek())) {
                        advance();
                    }

                    kind = match('>') ? PATH_LIT : ERROR;
                } else {
                    kind = match('=') ? LESS_EQUAL : match('<') ? LESS_LESS : LESS;
                }
                break;
            case '$':
                if (match('"')) {
                    kind = quoted('"') == STR_LIT ? INTERP_STR_LIT : ERROR;
                } else {
                    kind = DOLLAR;
                }
                break;
            case '"':
            case '\'':
                kind = quoted(c);
                break;
            default:
                if (is_digit(c)) {
                    kind = number();
                } else if (is_letter(c)) {
                    kind = identifier();
                } else {
                    while (is_continuation(peek())) {
                        advance();
                    }

                    kind = ERROR;
                }
        }

        tokens.push_back({kind, string_view(source).substr(start, pos - start), startLine, startCol});
    }

    tokens.push_back({END_OF_FILE, {}, line, col});
    return tokens;
}
