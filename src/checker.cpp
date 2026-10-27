#include "checker.h"

#include <algorithm>
#include <format>
#include <ranges>

using namespace std;

namespace {
    bool always_returns(const Statement& node) {
        if (dynamic_cast<const Return*>(&node)) {
            return true;
        }
        if (const auto* n = dynamic_cast<const Block*>(&node)) {
            return ranges::any_of(n->body, [](const auto& statement) { return always_returns(*statement); });
        }
        if (const auto* n = dynamic_cast<const If*>(&node)) {
            return n->elseBody && always_returns(*n->body) && always_returns(*n->elseBody);
        }
        return false;
    }
}

void Checker::check(Program& program) {
    for (const auto& fn : program) {
        if (fn->name == "print" || functions.contains(fn->name)) {
            error("MQ30002", fn->line, fn->col, format("name already defined ('{}')", fn->name));
        }
        Signature signature{{}, to_type(fn->returnType)};
        for (const auto& parameter : fn->parameters) {
            signature.parameters.push_back(to_type(parameter.type));
        }
        functions[fn->name] = signature;
    }

    const auto entry = functions.find("main");
    if (entry == functions.end()) {
        error("MQ30008", 1, 1, "missing entry-point function ('main')");
    }
    if (!entry->second.parameters.empty() || entry->second.returnType != Types::Void) {
        const auto& fn = *ranges::find(program, string("main"), [](const auto& f) { return f->name; });
        error("MQ30008", fn->line, fn->col, "entry-point function must be '@main() << void'");
    }

    for (const auto& fn : program) {
        current = fn.get();
        push();
        for (const auto& parameter : fn->parameters) {
            declare(parameter.name, to_type(parameter.type), parameter.line, parameter.col);
        }
        check(*fn->body);
        pop();
        if (to_type(fn->returnType) != Types::Void && !always_returns(*fn->body)) {
            error("MQ30005", fn->line, fn->col, format("not all paths return a value (function '{}')", fn->name));
        }
    }

    ranges::sort(warningList, {}, [](const Diagnostic& d) { return pair(d.line, d.col); });
}

const vector<Diagnostic>& Checker::warnings() const {
    return warningList;
}

void Checker::push() {
    scopes.emplace_back();
}

void Checker::pop() {
    for (const auto& [name, local] : scopes.back()) {
        if (!local.used) {
            warningList.push_back({"MQ70002", local.line, local.col,
                                   format("unused variable of type '{}' ('{}')", to_string(local.type), name)});
        }
    }
    scopes.pop_back();
}

void Checker::declare(const string& name, const Types type, const int line, const int col) {
    for (const auto& scope : scopes) {
        if (scope.contains(name)) {
            error("MQ30002", line, col, format("name already defined ('{}')", name));
        }
    }
    scopes.back()[name] = {type, line, col};
}

Types Checker::lookup(const string& name, const int line, const int col) {
    for (auto& scope : views::reverse(scopes)) {
        if (const auto found = scope.find(name); found != scope.end()) {
            found->second.used = true;
            return found->second.type;
        }
    }
    error("MQ30001", line, col, format("undeclared variable ('{}')", name));
}

Types Checker::check(Expression& node) {
    Types type = Types::Unknown;
    if (dynamic_cast<Int*>(&node)) {
        type = Types::Int;
    } else if (dynamic_cast<Bool*>(&node)) {
        type = Types::Bool;
    } else if (const auto* n = dynamic_cast<Name*>(&node)) {
        type = lookup(n->name, n->line, n->col);
    } else if (auto* n = dynamic_cast<Unary*>(&node)) {
        type = n->op == '!' ? Types::Bool : Types::Int;
        if (check(*n->operand) != type) {
            error("MQ30003", n->line, n->col, format("cannot use operator ('{}') on type '{}'",
                                                     n->op, to_string(n->operand->resultType)));
        }
    } else if (auto* n = dynamic_cast<Binary*>(&node)) {
        type = check_binary(*n);
    } else if (auto* n = dynamic_cast<Call*>(&node)) {
        type = check_call(*n);
    }
    node.resultType = type;
    return type;
}

Types Checker::check_binary(Binary& node) {
    const Types left = check(*node.left);
    const Types right = check(*node.right);
    const string& op = node.op;

    bool fits;
    Types result = Types::Bool;
    if (op == "&&" || op == "||") {
        fits = left == Types::Bool && right == Types::Bool;
    } else if (op == "==" || op == "!=") {
        fits = left == right && left != Types::Void;
    } else if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        fits = left == Types::Int && right == Types::Int;
    } else {
        fits = left == Types::Int && right == Types::Int;
        result = Types::Int;
    }

    if (!fits) {
        error("MQ30003", node.line, node.col, format("cannot use operator ('{}') on types '{}' and '{}'",
                                                     op, to_string(left), to_string(right)));
    }
    return result;
}

Types Checker::check_call(Call& node) {
    if (node.name == "print") {
        if (node.args.size() != 1) {
            error("MQ30004", node.line, node.col, format("function 'print' expected 1 argument, got {}", node.args.size()));
        }
        const Types type = check(*node.args[0]);
        if (type != Types::Int && type != Types::Bool) {
            error("MQ30006", node.args[0]->line, node.args[0]->col,
                  format("cannot use type '{}' for argument 1 of function 'print', expected type 'int' or 'bool'",
                         to_string(type)));
        }
        return Types::Void;
    }

    const auto found = functions.find(node.name);
    if (found == functions.end()) {
        error("MQ30001", node.line, node.col, format("undeclared function ('{}')", node.name));
    }
    const Signature& signature = found->second;
    if (node.args.size() != signature.parameters.size()) {
        error("MQ30004", node.line, node.col, format("function '{}' expected {} arguments, got {}",
                                                     node.name, signature.parameters.size(), node.args.size()));
    }
    for (size_t i = 0; i < node.args.size(); ++i) {
        check(*node.args[i]);
        expect_type(*node.args[i], signature.parameters[i],
                    format("argument {} of function '{}'", i + 1, node.name));
    }
    return signature.returnType;
}

void Checker::check(Statement& node) {
    if (const auto* n = dynamic_cast<ExprStmt*>(&node)) {
        auto* call = dynamic_cast<Call*>(n->expr.get());
        if (!call) {
            error("MQ30009", n->line, n->col, "only a call can be used as a statement");
        }
        if (const Types type = check(*call); type != Types::Void) {
            warningList.push_back({"MQ70001", call->line, call->col,
                                   format("unused return value of type '{}' ('{}')", to_string(type), call->name)});
        }
    } else if (const auto* n = dynamic_cast<Variable*>(&node)) {
        const Types type = to_type(n->type);
        if (n->value) {
            check(*n->value);
            expect_type(*n->value, type, format("variable '{}'", n->name));
        }
        declare(n->name, type, n->line, n->col);
    } else if (const auto* n = dynamic_cast<Assign*>(&node)) {
        const Types type = lookup(n->name, n->line, n->col);
        check(*n->value);
        expect_type(*n->value, type, format("variable '{}'", n->name));
    } else if (const auto* n = dynamic_cast<Block*>(&node)) {
        push();
        for (const auto& statement : n->body) {
            check(*statement);
        }
        pop();
    } else if (const auto* n = dynamic_cast<If*>(&node)) {
        check(*n->condition);
        expect_type(*n->condition, Types::Bool, "the condition");
        check(*n->body);
        if (n->elseBody) {
            check(*n->elseBody);
        }
    } else if (const auto* n = dynamic_cast<While*>(&node)) {
        check(*n->condition);
        expect_type(*n->condition, Types::Bool, "the condition");
        check(*n->body);
    } else if (const auto* n = dynamic_cast<Return*>(&node)) {
        check_return(*n);
    }
}

void Checker::check_return(const Return& node) {
    const Types expected = to_type(current->returnType);
    const Types type = node.value ? check(*node.value) : Types::Void;
    if (type != expected) {
        error("MQ30007", node.line, node.col, format("cannot return type '{}' for function '{}', expected type '{}'",
                                                     to_string(type), current->name, to_string(expected)));
    }
}

void Checker::expect_type(const Expression& node, const Types expected, const string_view what) const {
    if (node.resultType != expected) {
        error("MQ30006", node.line, node.col, format("cannot use type '{}' for {}, expected type '{}'",
                                                     to_string(node.resultType), what, to_string(expected)));
    }
}

void Checker::error(const string_view code, const int line, const int col, const string_view message) {
    throw Diagnostic{string(code), line, col, string(message)};
}
