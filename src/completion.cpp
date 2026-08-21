#include "completion.hpp"

#include "command_utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <readline/readline.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

namespace {
ShellState* shell_state = nullptr;

std::vector<std::string> command_matches(const std::string& prefix) {
    std::vector<std::string> matches;
    const std::vector<std::string> builtins = {"echo", "exit", "type", "pwd", "cd", "complete", "jobs", "history", "declare"};
    for (const auto& builtin : builtins) if (builtin.rfind(prefix, 0) == 0) matches.push_back(builtin);
    if (const char* path = std::getenv("PATH"); path != nullptr) {
        std::stringstream paths(path);
        std::string directory;
        while (std::getline(paths, directory, ':')) {
            std::error_code error;
            for (const auto& entry : std::filesystem::directory_iterator(directory, error)) {
                if (error || !entry.is_regular_file() || access(entry.path().c_str(), X_OK) != 0) continue;
                const auto name = entry.path().filename().string();
                if (name.rfind(prefix, 0) == 0 && std::find(matches.begin(), matches.end(), name) == matches.end()) matches.push_back(name);
            }
        }
    }
    std::sort(matches.begin(), matches.end());
    return matches;
}

char* generate_command(const char* text, int state) {
    static std::vector<std::string> matches;
    static std::size_t index;
    if (state == 0) { matches = command_matches(text); index = 0; }
    return index < matches.size() ? strdup(matches[index++].c_str()) : nullptr;
}

std::vector<std::string> run_completer(const std::string& script, const std::string& command, const std::string& word, const std::string& previous) {
    int descriptors[2];
    if (pipe(descriptors) == -1) return {};
    const pid_t pid = fork();
    if (pid == -1) { close(descriptors[0]); close(descriptors[1]); return {}; }
    if (pid == 0) {
        close(descriptors[0]);
        dup2(descriptors[1], STDOUT_FILENO);
        close(descriptors[1]);
        setenv("COMP_LINE", rl_line_buffer, 1);
        const auto point = std::to_string(rl_point);
        setenv("COMP_POINT", point.c_str(), 1);
        char* arguments[] = {const_cast<char*>(script.c_str()), const_cast<char*>(command.c_str()), const_cast<char*>(word.c_str()), const_cast<char*>(previous.c_str()), nullptr};
        execv(script.c_str(), arguments);
        _exit(1);
    }
    close(descriptors[1]);
    FILE* stream = fdopen(descriptors[0], "r");
    std::vector<std::string> matches;
    char buffer[4096];
    while (stream != nullptr && fgets(buffer, sizeof(buffer), stream) != nullptr) {
        std::string result(buffer);
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) result.pop_back();
        if (!result.empty()) matches.push_back(std::move(result));
    }
    if (stream != nullptr) fclose(stream); else close(descriptors[0]);
    waitpid(pid, nullptr, 0);
    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    return matches;
}

char** custom_completion(const char* text, int start, int) {
    if (start == 0) return rl_completion_matches(text, generate_command);
    const std::string line(rl_line_buffer);
    const auto command_end = line.find_first_of(" \t");
    if (command_end == std::string::npos || shell_state == nullptr) return rl_completion_matches(text, rl_filename_completion_function);
    const std::string command = line.substr(0, command_end);
    const auto specification = shell_state->completion_specs.find(command);
    if (specification == shell_state->completion_specs.end()) return rl_completion_matches(text, rl_filename_completion_function);
    std::string before = line.substr(0, start);
    while (!before.empty() && std::isspace(static_cast<unsigned char>(before.back()))) before.pop_back();
    const auto separator = before.find_last_of(" \t");
    const std::string previous = separator == std::string::npos ? before : before.substr(separator + 1);
    auto matches = run_completer(specification->second, command, text, previous);
    if (matches.empty()) { shell_state->multiple_completion_pending = false; std::cout << '\a' << std::flush; return nullptr; }
    if (matches.size() > 1) {
        std::string prefix = matches.front();
        for (const auto& match : matches) {
            std::size_t length = 0;
            while (length < prefix.size() && length < match.size() && prefix[length] == match[length]) ++length;
            prefix.resize(length);
        }
        if (prefix.size() > std::strlen(text)) {
            shell_state->multiple_completion_pending = false;
            rl_completion_append_character = '\0';
            char** result = static_cast<char**>(std::malloc(2 * sizeof(char*)));
            result[0] = strdup(prefix.c_str()); result[1] = nullptr;
            return result;
        }
        if (!shell_state->multiple_completion_pending) { shell_state->multiple_completion_pending = true; std::cout << '\a' << std::flush; return nullptr; }
        shell_state->multiple_completion_pending = false;
        std::cout << '\n';
        for (std::size_t index = 0; index < matches.size(); ++index) std::cout << (index == 0 ? "" : "  ") << matches[index];
        std::cout << '\n'; rl_on_new_line(); rl_redisplay(); return nullptr;
    }
    shell_state->multiple_completion_pending = false;
    rl_completion_append_character = ' ';
    char** result = static_cast<char**>(std::malloc(2 * sizeof(char*)));
    result[0] = strdup(matches.front().c_str()); result[1] = nullptr;
    return result;
}
}

void configure_completion(ShellState& state) {
    shell_state = &state;
    rl_attempted_completion_function = custom_completion;
    rl_completion_append_character = ' ';
}
