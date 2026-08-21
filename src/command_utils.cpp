#include "command_utils.hpp"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <unistd.h>

bool is_builtin(const std::string& command) {
    return command == "echo" || command == "exit" || command == "type" || command == "pwd" ||
           command == "cd" || command == "complete" || command == "jobs" || command == "history" || command == "declare";
}

std::string find_executable(const std::string& command) {
    if (command.find('/') != std::string::npos) {
        return access(command.c_str(), X_OK) == 0 ? command : std::string{};
    }
    const char* path = std::getenv("PATH");
    if (path == nullptr) return {};
    std::stringstream paths(path);
    std::string directory;
    while (std::getline(paths, directory, ':')) {
        const auto candidate = std::filesystem::path(directory) / command;
        if (std::filesystem::is_regular_file(candidate) && access(candidate.c_str(), X_OK) == 0) return candidate.string();
    }
    return {};
}
