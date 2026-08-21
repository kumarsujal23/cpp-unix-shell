#include "command_parser.hpp"

#include <cctype>

std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool single_quoted = false;
    bool double_quoted = false;

    const auto flush = [&] {
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    };

    for (std::size_t index = 0; index < input.size(); ++index) {
        const char character = input[index];
        if (single_quoted) {
            if (character == '\'') single_quoted = false;
            else current += character;
            continue;
        }
        if (double_quoted) {
            if (character == '"') double_quoted = false;
            else if (character == '\\' && index + 1 < input.size() &&
                     (input[index + 1] == '"' || input[index + 1] == '\\')) {
                current += input[++index];
            } else {
                current += character;
            }
            continue;
        }
        if (character == '\\' && index + 1 < input.size()) {
            current += input[++index];
        } else if (character == '\'') {
            single_quoted = true;
        } else if (character == '"') {
            double_quoted = true;
        } else if (character == '>') {
            flush();
            if (index + 1 < input.size() && input[index + 1] == '>') {
                tokens.emplace_back(">>");
                ++index;
            } else {
                tokens.emplace_back(">");
            }
        } else if (std::isspace(static_cast<unsigned char>(character))) {
            flush();
        } else {
            current += character;
        }
    }
    flush();
    return tokens;
}

std::vector<std::vector<std::string>> parse_pipeline(const std::string& input) {
    std::vector<std::vector<std::string>> pipeline;
    std::string segment;
    bool single_quoted = false;
    bool double_quoted = false;
    for (std::size_t index = 0; index <= input.size(); ++index) {
        const char character = index == input.size() ? '|' : input[index];
        if (character == '\'' && !double_quoted) single_quoted = !single_quoted;
        if (character == '"' && !single_quoted) double_quoted = !double_quoted;
        if (character == '|' && !single_quoted && !double_quoted) {
            auto tokens = tokenize(segment);
            if (!tokens.empty()) pipeline.push_back(std::move(tokens));
            segment.clear();
        } else {
            segment += character;
        }
    }
    return pipeline;
}

Command parse_command(const std::vector<std::string>& tokens) {
    Command command;
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        const auto& token = tokens[index];
        const bool stderr_redirect = token == "2>" || token == "2>>" ||
            (token == "2" && index + 1 < tokens.size() &&
             (tokens[index + 1] == ">" || tokens[index + 1] == ">>"));
        const bool stdout_redirect = token == ">" || token == ">>" || token == "1>" || token == "1>>" ||
            (token == "1" && index + 1 < tokens.size() &&
             (tokens[index + 1] == ">" || tokens[index + 1] == ">>"));
        if (stderr_redirect || stdout_redirect) {
            const bool append = token == ">>" || token == "1>>" || token == "2>>" ||
                (index + 1 < tokens.size() && tokens[index + 1] == ">>");
            if (token == "1" || token == "2") ++index;
            if (index + 1 >= tokens.size()) break;
            const std::string& file = tokens[++index];
            if (stderr_redirect) {
                command.redirections.redirect_stderr = true;
                command.redirections.append_stderr = append;
                command.redirections.stderr_file = file;
            } else {
                command.redirections.redirect_stdout = true;
                command.redirections.append_stdout = append;
                command.redirections.stdout_file = file;
            }
        } else {
            command.arguments.push_back(token);
        }
    }
    if (!command.arguments.empty() && command.arguments.back() == "&") {
        command.background = true;
        command.arguments.pop_back();
    }
    return command;
}

std::string expand_variables(const std::string& word, const ShellState& state) {
    std::string result;
    for (std::size_t index = 0; index < word.size();) {
        if (word[index] != '$') { result += word[index++]; continue; }
        std::size_t name_start = index + 1;
        std::size_t name_end = name_start;
        if (name_start < word.size() && word[name_start] == '{') {
            name_start++;
            name_end = word.find('}', name_start);
            if (name_end == std::string::npos) { result += word[index++]; continue; }
            index = name_end + 1;
        } else if (name_start < word.size() &&
                   (std::isalpha(static_cast<unsigned char>(word[name_start])) || word[name_start] == '_')) {
            while (name_end < word.size() &&
                   (std::isalnum(static_cast<unsigned char>(word[name_end])) || word[name_end] == '_')) ++name_end;
            index = name_end;
        } else { result += word[index++]; continue; }
        const auto found = state.variables.find(word.substr(name_start, name_end - name_start));
        if (found != state.variables.end()) result += found->second;
    }
    return result;
}

bool is_valid_identifier(const std::string& name) {
    if (name.empty() || (!std::isalpha(static_cast<unsigned char>(name.front())) && name.front() != '_')) return false;
    for (std::size_t index = 1; index < name.size(); ++index) {
        if (!std::isalnum(static_cast<unsigned char>(name[index])) && name[index] != '_') return false;
    }
    return true;
}
