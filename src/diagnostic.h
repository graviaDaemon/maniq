#pragma once
#include <string>

struct Diagnostic {
    std::string code;
    int line;
    int col;
    std::string message;
};
