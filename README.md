# Linux Shell

A small interactive POSIX-style shell written in C++23. It uses GNU Readline for
the prompt, history, and tab completion.

## Features

- Executes programs found on `PATH` and executable paths such as `/bin/echo`
- Builtins: `cd`, `complete`, `declare`, `echo`, `exit`, `history`, `jobs`, `pwd`, and `type`
- Single quotes, double quotes, and backslash escaping
- Shell variables with `$NAME` and `${NAME}` expansion
- Standard-output and standard-error redirection (`>`, `>>`, `1>`, `1>>`, `2>`, `2>>`)
- Pipelines with `|`
- Background processes with `&` and job status reporting
- Command, file, and script-provided tab completion

## Requirements

Linux or WSL with CMake 3.20+, a C++23 compiler, `pkg-config`, and GNU Readline.

On Ubuntu or Debian:

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libreadline-dev
```

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/linux-shell
```

Or use the helper script:

```bash
bash run.sh
```

## Tests

The parser test suite covers quoting, escaping, variable expansion, redirection,
background commands, and pipelines.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The included GitHub Actions workflow runs the same build and test commands on
Ubuntu for every push and pull request.

## Source layout

- `src/main.cpp` — shell lifecycle and REPL
- `src/command_parser.*` — tokenization, pipelines, redirection parsing, and variables
- `src/executor.*` — command launching, pipelines, and descriptor redirection
- `src/builtins.*` — builtin command implementations
- `src/completion.*` — Readline completion and completion scripts
- `src/job_manager.*` — background-process bookkeeping
- `src/command_utils.*` — builtin recognition and executable lookup
- `src/shell_types.hpp` — shared shell data types
