#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <sys/types.h>

struct Redirections {
    std::string stdout_file;
    std::string stderr_file;
    bool redirect_stdout = false;
    bool redirect_stderr = false;
    bool append_stdout = false;
    bool append_stderr = false;
};

struct Command {
    std::vector<std::string> arguments;
    Redirections redirections;
    bool background = false;
};

struct Job {
    int number;
    pid_t pid;
    std::string command;
};

struct ShellState {
    std::unordered_map<std::string, std::string> variables;
    std::unordered_map<std::string, std::string> completion_specs;
    std::vector<Job> jobs;
    int last_history_append = 0;
    bool multiple_completion_pending = false;
};
