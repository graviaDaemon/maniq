#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "diagnostic.h"

class Checker {
public:
    void check(Program& program);

    [[nodiscard]] const std::vector<Diagnostic>& warnings() const;

private:
    struct Signature {
        std::vector<Types> parameters;
        Types returnType;
    };

    struct Local {
        Types type;
        int line;
        int col;
        bool used = false;
    };

    std::vector<std::unordered_map<std::string, Local>> scopes;
    std::unordered_map<std::string, Signature> functions;
    const Function* current = nullptr;
    std::vector<Diagnostic> warningList;

    void push();
    void pop();
    void declare(const std::string& name, Types type, int line, int col);
    Types lookup(const std::string& name, int line, int col);

    Types check(Expression& node);
    Types check_binary(Binary& node);
    Types check_call(Call& node);
    void check(Statement& node);
    void check_return(const Return& node);
    void expect_type(const Expression& node, Types expected, std::string_view what) const;

    [[noreturn]] static void error(std::string_view code, int line, int col, std::string_view message);
};
