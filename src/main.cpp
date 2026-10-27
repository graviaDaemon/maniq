#include <cstdio>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "lexer.h"
#include "ast.h"
#include "checker.h"
#include "codegen.h"
#include "parser.h"

using namespace std;

namespace {
    void dump(ostream& out, const vector<Token>& tokens) {
        for (const auto& [kind, text, line, col] : tokens) {
            out << left << setw(8) << to_string(line) + ":" + to_string(col);

            if (text.empty()) {
                out << to_string(kind) << "\n";
            } else {
                out << setw(16) << to_string(kind) << text << "\n";
            }
        }
    }

    void report(const string_view file, const Diagnostic& d, const string& detail = "") {
        const char* label = d.code[2] == '1' ? "Lexer-Error"
                          : d.code[2] == '2' ? "Parser-Error"
                          : d.code[2] == '3' ? "Checker-Error"
                          : d.code[2] == '4' ? "Generate-Error"
                          : d.code[2] == '5' ? "Build-Error"
                          : "Warning";
        if (!file.empty()) {
            cerr << file;
            if (d.line > 0) {
                cerr << ":" << d.line << ":" << d.col;
            }
            cerr << "\n    ==> ";
        }
        cerr << label << "[" << d.code << "]: " << d.message << "\n";

        istringstream lines(detail);
        const char* prefix = "    /=> ";
        for (string text; getline(lines, text); prefix = "        ") {
            if (!text.empty() && text.back() == '\r') {
                text.pop_back();
            }
            cerr << prefix << text << "\n";
        }
    }

    string read_log() {
        ostringstream log;
        log << ifstream("out.log").rdbuf();
        return log.str();
    }

    int compile(const string_view file, const Program& program) {
        const string_view name = file.substr(file.find_last_of("/\\") + 1);
        const string_view stem = name.substr(0, name.rfind('.'));
        {
            ofstream cFile("out.c", ios::binary);
            generate(cFile, program, name);
        }

        if (std::system("gcc --version > out.log 2>&1") != 0) {
            report("", {"MQ50001", 0, 0, "gcc not found; install gcc and add to PATH"}, read_log());
            return 1;
        }
        const string command = format("gcc -std=c23 -O2 -fwrapv out.c -o \"{}\" > out.log 2>&1", stem);
        if (std::system(command.c_str()) != 0) {
            report(file, {"MQ40001", 0, 0, "internal error: gcc rejected the generated C in out.c"}, read_log());
            return 1;
        }
        return 0;
    }

    int build(const string_view file, vector<Token> tokens) {
        for (const char* stale : {"out.tokens", "out.ast", "out.types", "out.c", "out.log"}) {
            std::remove(stale);
        }

        ofstream tokensFile("out.tokens", ios::binary);
        dump(tokensFile, tokens);

        Program program;
        try {
            program = Parser(std::move(tokens)).parse();
        } catch (const Diagnostic& d) {
            report(file, d);
            return 1;
        }
        ofstream astFile("out.ast", ios::binary);
        dump(astFile, program);

        Checker checker;
        try {
            checker.check(program);
        } catch (const Diagnostic& d) {
            report(file, d);
            return 1;
        }
        for (const auto& warning : checker.warnings()) {
            report(file, warning);
        }
        ofstream typesFile("out.types", ios::binary);
        dump(typesFile, program);
        return compile(file, program);
    }
}

int main(const int argc, char* argv[]) {
    if (argc == 1) {
        cout << "maniq " MANIQ_VERSION "\n";
        return 0;
    }

    const string_view command = argv[1];

    if (argc != 3 || (command != "lex" && command != "parse" && command != "build")) {
        cerr << "usage: maniq lex <file> | maniq parse <file> | maniq build <file>\n";
        return 1;
    }

    Lexer lexer;

    try {
        lexer.read(argv[2]);
    } catch (const runtime_error& e) {
        cerr << e.what() << "\n";
        return 1;
    }

    auto tokens = lexer.lex();

    if (command == "build") {
        return build(argv[2], std::move(tokens));
    }

    if (command == "parse") {
        ostringstream tree;
        try {
            dump(tree, Parser(std::move(tokens)).parse());
        } catch (const Diagnostic& d) {
            report(argv[2], d);
            return 1;
        }
        cout << tree.str();
        ofstream("out.ast", ios::binary) << tree.str();
        return 0;
    }

    dump(cout, tokens);
    return 0;
}
