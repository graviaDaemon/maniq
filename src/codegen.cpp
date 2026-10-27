#include "codegen.h"

#include <format>

using namespace std;

namespace {
    constexpr string_view runtime = R"c([[noreturn]] void maniq_runtime_error(const char* code, const char* message, int line, int col) {
    fflush(stdout);
    fprintf(stderr, "%s:%d:%d\n    ==> Runtime-Error[%s]: %s\n", maniq_source, line, col, code, message);
    exit(1);
}

void maniq_print_int(int32_t value) {
    printf("%" PRId32 "\n", value);
}

void maniq_print_bool(bool value) {
    puts(value ? "true" : "false");
}

int32_t maniq_div(int32_t left, int32_t right, int line, int col) {
    if (right == 0) {
        maniq_runtime_error("MQ60001", "division by zero", line, col);
    }
    if (right == -1) {
        return (int32_t)(0u - (uint32_t)left);
    }
    return left / right;
}

int32_t maniq_mod(int32_t left, int32_t right, int line, int col) {
    if (right == 0) {
        maniq_runtime_error("MQ60001", "division by zero", line, col);
    }
    if (right == -1) {
        return 0;
    }
    return left % right;
}

)c";

    void line(ostream& out, const int depth, const string_view text) {
        out << string(depth * 4, ' ') << text << "\n";
    }

    string c_type(const string& type) {
        return type == "int" ? "int32_t" : type;
    }

    bool is_operator(const Expression& node) {
        const auto* n = dynamic_cast<const Binary*>(&node);
        return n && n->op != "/" && n->op != "%";
    }

    string expression(const Expression& node);

    string grouped(const Expression& node, const bool parens) {
        return parens ? "(" + expression(node) + ")" : expression(node);
    }

    string expression(const Expression& node) {
        if (const auto* n = dynamic_cast<const Int*>(&node)) {
            return to_string(n->value);
        }
        if (const auto* n = dynamic_cast<const Bool*>(&node)) {
            return n->value ? "true" : "false";
        }
        if (const auto* n = dynamic_cast<const Name*>(&node)) {
            return "mq_" + n->name;
        }
        if (const auto* n = dynamic_cast<const Unary*>(&node)) {
            const bool parens = is_operator(*n->operand) || dynamic_cast<const Unary*>(n->operand.get());
            return n->op + grouped(*n->operand, parens);
        }
        if (const auto* n = dynamic_cast<const Binary*>(&node)) {
            if (n->op == "/" || n->op == "%") {
                return format("maniq_{}({}, {}, {}, {})", n->op == "/" ? "div" : "mod",
                              expression(*n->left), expression(*n->right), n->line, n->col);
            }
            return grouped(*n->left, is_operator(*n->left)) + " " + n->op + " " +
                   grouped(*n->right, is_operator(*n->right));
        }
        if (const auto* n = dynamic_cast<const Call*>(&node)) {
            if (n->name == "print") {
                return format("maniq_print_{}({})", to_string(n->args[0]->resultType), expression(*n->args[0]));
            }
            string args;
            for (const auto& arg : n->args) {
                args += (args.empty() ? "" : ", ") + expression(*arg);
            }
            return format("mq_{}({})", n->name, args);
        }
        return "";
    }

    void statement(ostream& out, const Statement& node, int depth);

    void body(ostream& out, const Block& node, const int depth) {
        for (const auto& s : node.body) {
            statement(out, *s, depth);
        }
    }

    void if_chain(ostream& out, const If& node, const int depth, const string& prefix) {
        line(out, depth, format("{}if ({}) {{", prefix, expression(*node.condition)));
        body(out, *node.body, depth + 1);
        if (!node.elseBody) {
            line(out, depth, "}");
        } else if (const auto* next = dynamic_cast<const If*>(node.elseBody.get())) {
            if_chain(out, *next, depth, "} else ");
        } else {
            line(out, depth, "} else {");
            body(out, dynamic_cast<const Block&>(*node.elseBody), depth + 1);
            line(out, depth, "}");
        }
    }

    void statement(ostream& out, const Statement& node, const int depth) {
        if (const auto* n = dynamic_cast<const ExprStmt*>(&node)) {
            line(out, depth, expression(*n->expr) + ";");
        } else if (const auto* n = dynamic_cast<const Variable*>(&node)) {
            const string value = n->value ? expression(*n->value) : n->type == "bool" ? "false" : "0";
            line(out, depth, format("{} mq_{} = {};", c_type(n->type), n->name, value));
        } else if (const auto* n = dynamic_cast<const Assign*>(&node)) {
            line(out, depth, format("mq_{} = {};", n->name, expression(*n->value)));
        } else if (const auto* n = dynamic_cast<const Block*>(&node)) {
            line(out, depth, "{");
            body(out, *n, depth + 1);
            line(out, depth, "}");
        } else if (const auto* n = dynamic_cast<const If*>(&node)) {
            if_chain(out, *n, depth, "");
        } else if (const auto* n = dynamic_cast<const While*>(&node)) {
            line(out, depth, format("while ({}) {{", expression(*n->condition)));
            body(out, *n->body, depth + 1);
            line(out, depth, "}");
        } else if (const auto* n = dynamic_cast<const Return*>(&node)) {
            line(out, depth, n->value ? format("return {};", expression(*n->value)) : "return;");
        }
    }

    string signature(const Function& fn) {
        string parameters;
        for (const auto& parameter : fn.parameters) {
            parameters += format("{}{} mq_{}", parameters.empty() ? "" : ", ", c_type(parameter.type), parameter.name);
        }
        return format("{} mq_{}({})", c_type(fn.returnType), fn.name, parameters);
    }
}

void generate(ostream& out, const Program& program, const string_view source) {
    out << "#include <inttypes.h>\n"
           "#include <stdint.h>\n"
           "#include <stdio.h>\n"
           "#include <stdlib.h>\n\n";
    out << format("static const char maniq_source[] = \"{}\";\n\n", source);
    out << runtime;

    for (const auto& fn : program) {
        out << signature(*fn) << ";\n";
    }
    out << "\n";

    for (const auto& fn : program) {
        out << signature(*fn) << " {\n";
        body(out, *fn->body, 1);
        out << "}\n\n";
    }

    out << "int main() {\n"
           "    mq_main();\n"
           "    return 0;\n"
           "}\n";
}
