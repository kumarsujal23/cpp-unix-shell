#pragma once

#include "shell_types.hpp"

#include <string>
#include <vector>

std::vector<std::string> tokenize(const std::string& input);
std::vector<std::vector<std::string>> parse_pipeline(const std::string& input);
Command parse_command(const std::vector<std::string>& tokens);
std::string expand_variables(const std::string& word, const ShellState& state);
bool is_valid_identifier(const std::string& name);
