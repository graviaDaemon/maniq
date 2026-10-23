#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include "lexer.h"

using namespace std;

int main(const int argc, char* argv[]) {
    if (argc == 1) {
        cout << "maniq " MANIQ_VERSION "\n";
        return 0;
    }

    if (argc != 3 || string_view(argv[1]) != "lex") {
        cerr << "usage: maniq lex <file>\n";
        return 1;
    }

    Lexer lexer;

    try {
        lexer.read(argv[2]);
    } catch (const runtime_error& e) {
        cerr << e.what() << "\n";
        return 1;
    }

    for (const auto& [kind, text, line, col] : lexer.lex()) {
        cout << left << setw(8) << to_string(line) + ":" + to_string(col);

        if (text.empty()) {
            cout << to_string(kind) << "\n";
        } else {
            cout << setw(16) << to_string(kind) << text << "\n";
        }
    }

    return 0;
}
