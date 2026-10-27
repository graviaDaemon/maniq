#include "ast.h"

using namespace std;

namespace {
    void line(ostream& out, const int depth, const string_view text) {
        out << string(depth * 4, ' ') << text << "\n";
    }

    string typed(const Expression& node, const string& text) {
        return node.resultType == Types::Unknown ? text : text + " : " + to_string(node.resultType);
    }
}

Types to_type(const string& name) {
    if (name == "int") {
        return Types::Int;
    }
    if (name == "bool") {
        return Types::Bool;
    }
    if (name == "void") {
        return Types::Void;
    }
    return Types::Unknown;
}

string to_string(const Types type) {
    switch (type) {
        case Types::Int:     return "int";
        case Types::Bool:    return "bool";
        case Types::Void:    return "void";
        case Types::Unknown: return "unknown";
    }
    return "unknown";
}

void dump(ostream& out, const Expression& node, const int depth) {
    if (const auto* n = dynamic_cast<const Int*>(&node)) {
        line(out, depth, typed(node, "Int " + to_string(n->value)));
    } else if (const auto* n = dynamic_cast<const Bool*>(&node)) {
        line(out, depth, typed(node, n->value ? "Bool true" : "Bool false"));
    } else if (const auto* n = dynamic_cast<const Name*>(&node)) {
        line(out, depth, typed(node, "Name " + n->name));
    } else if (const auto* n = dynamic_cast<const Unary*>(&node)) {
        line(out, depth, typed(node, string("Unary ") + n->op));
        dump(out, *n->operand, depth + 1);
    } else if (const auto* n = dynamic_cast<const Binary*>(&node)) {
        line(out, depth, typed(node, "Binary " + n->op));
        dump(out, *n->left, depth + 1);
        dump(out, *n->right, depth + 1);
    } else if (const auto* n = dynamic_cast<const Call*>(&node)) {
        line(out, depth, typed(node, "Call"));
        line(out, depth + 1, "Name " + n->name);
        for (const auto& arg : n->args) {
            dump(out, *arg, depth + 1);
        }
    }
}

void dump(ostream& out, const Statement& node, const int depth) {
    if (const auto* n = dynamic_cast<const ExprStmt*>(&node)) {
        dump(out, *n->expr, depth);
    } else if (const auto* n = dynamic_cast<const Variable*>(&node)) {
        line(out, depth, "Variable");
        line(out, depth + 1, "Type " + n->type);
        line(out, depth + 1, "Name " + n->name);
        if (n->value) {
            dump(out, *n->value, depth + 1);
        }
    } else if (const auto* n = dynamic_cast<const Assign*>(&node)) {
        line(out, depth, "Assign");
        line(out, depth + 1, "Name " + n->name);
        dump(out, *n->value, depth + 1);
    } else if (const auto* n = dynamic_cast<const Block*>(&node)) {
        line(out, depth, "Block");
        for (const auto& statement : n->body) {
            dump(out, *statement, depth + 1);
        }
    } else if (const auto* n = dynamic_cast<const If*>(&node)) {
        line(out, depth, "If");
        dump(out, *n->condition, depth + 1);
        dump(out, *n->body, depth + 1);
        if (n->elseBody) {
            dump(out, *n->elseBody, depth + 1);
        }
    } else if (const auto* n = dynamic_cast<const While*>(&node)) {
        line(out, depth, "While");
        dump(out, *n->condition, depth + 1);
        dump(out, *n->body, depth + 1);
    } else if (const auto* n = dynamic_cast<const Return*>(&node)) {
        line(out, depth, "Return");
        if (n->value) {
            dump(out, *n->value, depth + 1);
        }
    }
}

void dump(ostream& out, const Function& node, const int depth) {
    line(out, depth, "Function " + node.name);
    for (const auto& parameter : node.parameters) {
        line(out, depth + 1, "Parameter");
        line(out, depth + 2, "Type " + parameter.type);
        line(out, depth + 2, "Name " + parameter.name);
    }
    line(out, depth + 1, "Returns " + node.returnType);
    dump(out, *node.body, depth + 1);
}

void dump(ostream& out, const Program& program) {
    for (size_t i = 0; i < program.size(); ++i) {
        if (i > 0) {
            out << "\n";
        }
        dump(out, *program[i]);
    }
}
