#pragma once

#include <string>

bool is_builtin(const std::string& command);
std::string find_executable(const std::string& command);
