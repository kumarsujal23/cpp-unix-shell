#pragma once

#include "shell_types.hpp"

bool execute_command(Command command, ShellState& state);
void execute_pipeline(std::vector<std::vector<std::string>> pipeline, ShellState& state);
