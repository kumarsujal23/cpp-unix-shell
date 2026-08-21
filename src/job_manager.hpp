#pragma once

#include "shell_types.hpp"

void reap_background_jobs(ShellState& state);
void print_jobs(ShellState& state);
void add_background_job(ShellState& state, pid_t pid, const std::vector<std::string>& arguments);
