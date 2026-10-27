#pragma once
#include <ostream>
#include <string_view>

#include "ast.h"

void generate(std::ostream& out, const Program& program, std::string_view source);
