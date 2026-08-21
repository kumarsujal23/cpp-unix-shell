#pragma once

#include "shell_types.hpp"

enum class BuiltinResult { not_handled, continue_shell, exit_shell };

BuiltinResult execute_builtin(const Command& command, ShellState& state);
