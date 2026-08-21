#include "command_parser.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
int failures = 0;

void expect(bool condition, const std::string& description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failures;
    }
}

void expect_tokens(const std::string& input, const std::vector<std::string>& expected) {
    expect(tokenize(input) == expected, "tokenize: " + input);
}
}

int main() {
    expect_tokens("echo hello world", {"echo", "hello", "world"});
    expect_tokens("echo 'two words' \"three words\"", {"echo", "two words", "three words"});
    expect_tokens("echo hello\\ world", {"echo", "hello world"});
    expect_tokens("echo \"mixed\\\"quote'test'\\\\\"", {"echo", "mixed\"quote'test'\\"});

    const Command redirected = parse_command(tokenize("echo hello 1>> out.log 2> errors.log &"));
    expect(redirected.arguments == std::vector<std::string>({"echo", "hello"}), "command arguments survive redirection parsing");
    expect(redirected.background, "background marker is recognized");
    expect(redirected.redirections.redirect_stdout && redirected.redirections.append_stdout &&
               redirected.redirections.stdout_file == "out.log",
           "stdout append redirection is recognized");
    expect(redirected.redirections.redirect_stderr && !redirected.redirections.append_stderr &&
               redirected.redirections.stderr_file == "errors.log",
           "stderr overwrite redirection is recognized");

    const auto pipeline = parse_pipeline("echo 'left|side' | cat");
    expect(pipeline == std::vector<std::vector<std::string>>({{"echo", "left|side"}, {"cat"}}),
           "quoted pipes are not split into pipeline segments");

    ShellState state;
    state.variables["USER"] = "Ada";
    state.variables["HOME_DIR"] = "/tmp/home";
    expect(expand_variables("$USER:${HOME_DIR}", state) == "Ada:/tmp/home", "variable expansion supports both forms");
    expect(expand_variables("$UNKNOWN", state).empty(), "unknown variables expand to an empty string");
    expect(is_valid_identifier("valid_name_42"), "valid identifiers are accepted");
    expect(!is_valid_identifier("42invalid"), "invalid identifiers are rejected");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "All command parser tests passed.\n";
    return EXIT_SUCCESS;
}
