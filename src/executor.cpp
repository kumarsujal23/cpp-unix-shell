#include "executor.hpp"

#include "builtins.hpp"
#include "command_parser.hpp"
#include "command_utils.hpp"
#include "job_manager.hpp"

#include <algorithm>
#include <array>
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

namespace {
class RedirectionGuard {
public:
    explicit RedirectionGuard(const Redirections& redirects) {
        apply(redirects.redirect_stdout, redirects.stdout_file, redirects.append_stdout, STDOUT_FILENO, saved_stdout_);
        apply(redirects.redirect_stderr, redirects.stderr_file, redirects.append_stderr, STDERR_FILENO, saved_stderr_);
    }
    ~RedirectionGuard() { restore(saved_stdout_, STDOUT_FILENO); restore(saved_stderr_, STDERR_FILENO); }
    bool valid() const { return valid_; }
private:
    void apply(bool enabled, const std::string& file, bool append, int target, int& saved) {
        if (!enabled) return;
        saved = dup(target);
        const int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        const int descriptor = open(file.c_str(), flags, 0644);
        if (descriptor == -1) { std::cerr << "Failed to open " << file << '\n'; valid_ = false; return; }
        dup2(descriptor, target); close(descriptor);
    }
    static void restore(int saved, int target) { if (saved != -1) { std::cout.flush(); std::cerr.flush(); dup2(saved, target); close(saved); } }
    int saved_stdout_ = -1;
    int saved_stderr_ = -1;
    bool valid_ = true;
};

void child_execute(const Command& command) {
    RedirectionGuard redirects(command.redirections);
    if (!redirects.valid()) _exit(1);
    std::vector<char*> arguments;
    for (const auto& argument : command.arguments) arguments.push_back(const_cast<char*>(argument.c_str()));
    arguments.push_back(nullptr);
    execvp(arguments[0], arguments.data());
    std::cerr << command.arguments[0] << ": command not found\n";
    _exit(127);
}
}

bool execute_command(Command command, ShellState& state) {
    for (auto& argument : command.arguments) argument = expand_variables(argument, state);
    if (command.redirections.redirect_stdout) command.redirections.stdout_file = expand_variables(command.redirections.stdout_file, state);
    if (command.redirections.redirect_stderr) command.redirections.stderr_file = expand_variables(command.redirections.stderr_file, state);
    command.arguments.erase(std::remove(command.arguments.begin(), command.arguments.end(), ""), command.arguments.end());
    if (command.arguments.empty()) return true;
    if (is_builtin(command.arguments[0])) {
        RedirectionGuard redirects(command.redirections);
        if (!redirects.valid()) return true;
        return execute_builtin(command, state) != BuiltinResult::exit_shell;
    }
    if (find_executable(command.arguments[0]).empty()) { std::cout << command.arguments[0] << ": command not found\n"; return true; }
    const pid_t pid = fork();
    if (pid == -1) { perror("fork"); return true; }
    if (pid == 0) child_execute(command);
    if (command.background) add_background_job(state, pid, command.arguments);
    else waitpid(pid, nullptr, 0);
    return true;
}

void execute_pipeline(std::vector<std::vector<std::string>> pipeline, ShellState& state) {
    for (auto& segment : pipeline) for (auto& word : segment) word = expand_variables(word, state);
    std::vector<std::array<int, 2>> pipes(pipeline.empty() ? 0 : pipeline.size() - 1);
    for (auto& pipe_pair : pipes) if (pipe(pipe_pair.data()) == -1) { perror("pipe"); return; }
    std::vector<pid_t> pids;
    for (std::size_t index = 0; index < pipeline.size(); ++index) {
        const pid_t pid = fork();
        if (pid == -1) { perror("fork"); return; }
        if (pid == 0) {
            if (index > 0) dup2(pipes[index - 1][0], STDIN_FILENO);
            if (index + 1 < pipeline.size()) dup2(pipes[index][1], STDOUT_FILENO);
            for (auto& pair : pipes) { close(pair[0]); close(pair[1]); }
            Command command = parse_command(pipeline[index]);
            if (command.arguments.empty()) _exit(0);
            if (is_builtin(command.arguments[0])) _exit(execute_builtin(command, state) == BuiltinResult::exit_shell ? 0 : 0);
            child_execute(command);
        }
        pids.push_back(pid);
    }
    for (auto& pair : pipes) { close(pair[0]); close(pair[1]); }
    for (const auto pid : pids) waitpid(pid, nullptr, 0);
}
