#include "command_parser.hpp"
#include "completion.hpp"
#include "executor.hpp"
#include "job_manager.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    ShellState state;
    configure_completion(state);
    if (const char* history_file = std::getenv("HISTFILE"); history_file != nullptr && *history_file != '\0') read_history(history_file);

    while (true) {
        reap_background_jobs(state);
        char* raw_input = readline("$ ");
        if (raw_input == nullptr) break;
        std::string input(raw_input);
        if (!input.empty()) add_history(raw_input);
        free(raw_input);

        // Keep the normal command path independent of pipeline parsing. Besides
        // avoiding unnecessary work, this preserves escaped quotes verbatim.
        if (input.find('|') != std::string::npos) {
            const auto pipeline = parse_pipeline(input);
            if (!pipeline.empty()) execute_pipeline(pipeline, state);
            continue;
        }

        if (!execute_command(parse_command(tokenize(input)), state)) break;
    }
    return 0;
}
