#include "job_manager.hpp"

#include <algorithm>
#include <iostream>
#include <sys/wait.h>

namespace {
std::string display_command(const std::string& command) {
    return command.size() >= 2 && command.ends_with(" &") ? command.substr(0, command.size() - 2) : command;
}

char marker(std::size_t index, std::size_t count) {
    return index + 1 == count ? '+' : index + 2 == count ? '-' : ' ';
}

void print_job(const Job& job, char job_marker, const char* status) {
    std::cout << '[' << job.number << ']' << job_marker << "  " << status;
    for (std::size_t index = std::char_traits<char>::length(status); index < 24; ++index) std::cout << ' ';
    std::cout << display_command(job.command) << '\n';
}
}

void reap_background_jobs(ShellState& state) {
    for (std::size_t index = 0; index < state.jobs.size();) {
        int status = 0;
        if (waitpid(state.jobs[index].pid, &status, WNOHANG) == state.jobs[index].pid && WIFEXITED(status)) {
            print_job(state.jobs[index], marker(index, state.jobs.size()), "Done");
            state.jobs.erase(state.jobs.begin() + static_cast<std::ptrdiff_t>(index));
        } else {
            ++index;
        }
    }
}

void print_jobs(ShellState& state) {
    std::vector<bool> completed(state.jobs.size());
    for (std::size_t index = 0; index < state.jobs.size(); ++index) {
        int status = 0;
        completed[index] = waitpid(state.jobs[index].pid, &status, WNOHANG) == state.jobs[index].pid && WIFEXITED(status);
    }
    for (std::size_t index = 0; index < state.jobs.size(); ++index) {
        print_job(state.jobs[index], marker(index, state.jobs.size()), completed[index] ? "Done" : "Running");
    }
    for (std::size_t index = state.jobs.size(); index-- > 0;) {
        if (completed[index]) state.jobs.erase(state.jobs.begin() + static_cast<std::ptrdiff_t>(index));
    }
}

void add_background_job(ShellState& state, pid_t pid, const std::vector<std::string>& arguments) {
    int number = 1;
    for (const auto& job : state.jobs) number = std::max(number, job.number + 1);
    std::string command;
    for (const auto& argument : arguments) command += (command.empty() ? "" : " ") + argument;
    command += " &";
    state.jobs.push_back({number, pid, command});
    std::cout << '[' << number << "] " << pid << '\n';
}
