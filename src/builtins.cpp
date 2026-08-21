#include "builtins.hpp"

#include "command_parser.hpp"
#include "command_utils.hpp"
#include "job_manager.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <readline/history.h>
#include <unistd.h>

namespace {
void print_echo(const std::vector<std::string>& arguments) {
    for (std::size_t index = 1; index < arguments.size(); ++index) std::cout << (index == 1 ? "" : " ") << arguments[index];
    std::cout << '\n';
}

int history_count(HIST_ENTRY** entries) {
    int count = 0;
    while (entries != nullptr && entries[count] != nullptr) ++count;
    return count;
}
}

BuiltinResult execute_builtin(const Command& command, ShellState& state) {
    const auto& arguments = command.arguments;
    if (arguments.empty() || !is_builtin(arguments[0])) return BuiltinResult::not_handled;
    const std::string& name = arguments[0];

    if (name == "exit") {
        if (const char* history_file = std::getenv("HISTFILE"); history_file != nullptr && *history_file != '\0') write_history(history_file);
        return BuiltinResult::exit_shell;
    }
    if (name == "echo") { print_echo(arguments); return BuiltinResult::continue_shell; }
    if (name == "pwd") {
        char current_directory[4096];
        if (getcwd(current_directory, sizeof(current_directory)) != nullptr) std::cout << current_directory << '\n';
        return BuiltinResult::continue_shell;
    }
    if (name == "cd") {
        if (arguments.size() < 2) return BuiltinResult::continue_shell;
        std::string directory = arguments[1];
        if (directory == "~") if (const char* home = std::getenv("HOME"); home != nullptr) directory = home;
        if (chdir(directory.c_str()) != 0) std::cout << "cd: " << arguments[1] << ": No such file or directory\n";
        return BuiltinResult::continue_shell;
    }
    if (name == "type") {
        if (arguments.size() < 2) return BuiltinResult::continue_shell;
        if (is_builtin(arguments[1])) std::cout << arguments[1] << " is a shell builtin\n";
        else if (const auto executable = find_executable(arguments[1]); !executable.empty()) std::cout << arguments[1] << " is " << executable << '\n';
        else std::cout << arguments[1] << ": not found\n";
        return BuiltinResult::continue_shell;
    }
    if (name == "declare") {
        if (arguments.size() >= 3 && arguments[1] == "-p") {
            const auto variable = state.variables.find(arguments[2]);
            if (variable == state.variables.end()) std::cout << "declare: " << arguments[2] << ": not found\n";
            else std::cout << "declare -- " << variable->first << "=\"" << variable->second << "\"\n";
        } else {
            for (std::size_t index = 1; index < arguments.size(); ++index) {
                const auto separator = arguments[index].find('=');
                if (separator == std::string::npos) continue;
                const auto variable_name = arguments[index].substr(0, separator);
                if (!is_valid_identifier(variable_name)) std::cout << "declare: `" << arguments[index] << "': not a valid identifier\n";
                else state.variables[variable_name] = arguments[index].substr(separator + 1);
            }
        }
        return BuiltinResult::continue_shell;
    }
    if (name == "history") {
        if (arguments.size() >= 3 && arguments[1] == "-r") read_history(arguments[2].c_str());
        else if (arguments.size() >= 3 && arguments[1] == "-w") write_history(arguments[2].c_str());
        else if (arguments.size() >= 3 && arguments[1] == "-a") {
            if (std::ofstream output(arguments[2], std::ios::app); output) {
                HIST_ENTRY** entries = history_list();
                const int count = history_count(entries);
                for (int index = state.last_history_append; index < count; ++index) output << entries[index]->line << '\n';
                state.last_history_append = count;
            }
        } else {
            HIST_ENTRY** entries = history_list();
            const int count = history_count(entries);
            int start = 0;
            if (arguments.size() >= 2) { try { start = std::max(0, count - std::stoi(arguments[1])); } catch (...) {} }
            for (int index = start; index < count; ++index) std::cout << std::setw(5) << index + 1 << "  " << entries[index]->line << '\n';
        }
        return BuiltinResult::continue_shell;
    }
    if (name == "complete") {
        if (arguments.size() >= 3 && arguments[1] == "-r") state.completion_specs.erase(arguments[2]);
        else if (arguments.size() >= 3 && arguments[1] == "-p") {
            const auto specification = state.completion_specs.find(arguments[2]);
            if (specification == state.completion_specs.end()) std::cout << "complete: " << arguments[2] << ": no completion specification\n";
            else std::cout << "complete -C '" << specification->second << "' " << specification->first << '\n';
        } else if (arguments.size() >= 4 && arguments[1] == "-C") state.completion_specs[arguments[3]] = arguments[2];
        return BuiltinResult::continue_shell;
    }
    print_jobs(state);
    return BuiltinResult::continue_shell;
}
