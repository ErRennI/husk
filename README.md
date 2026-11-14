# HUSK

[![CI](https://github.com/ErRennI/husk/actions/workflows/ci.yml/badge.svg)](https://github.com/ErRennI/husk/actions/workflows/ci.yml)

A bash-like Unix shell written from scratch in C. It has its own tokenizer, recursive descent parser, and a fork/exec-based executor.

## Features

- **Command execution** — external commands via `fork`/`execvp`/`waitpid`
- **Pipes** — `cmd1 | cmd2 | cmd3` (chained pipe support)
- **Logical operators** — `&&`, `||` (short-circuit evaluation)
- **Sequential execution** — `;`
- **Redirection** — `<`, `>`, `>>`
- **Quoting** — single quotes (literal) and double quotes (expandable); operators inside quotes are ignored
- **Environment variable expansion** — `$VAR`, including embedded usage like `hello_$USER`; an unquoted variable that expands to nothing is dropped, like in bash
- **Builtin commands** — `cd`, `pwd`, `exit`
- **Signal handling** — Ctrl+C only interrupts the running command, the shell stays alive
- **Script input** — the prompt is only shown when stdin is a terminal, so commands can be piped or redirected in (`./build/husk < script.sh`)
- **Leak-free memory management** — checked with Valgrind on every CI run

## Architecture

```
main.c        → REPL loop, line reading
tokenizer.c   → splits a line into tokens (state machine)
parse.c       → builds an AST from tokens (recursive descent)
executor.c    → executes the AST via fork/exec
builtins.c    → cd, pwd, exit
```

### Flow

```
line → tokenize_line() → token array
     → parse_tokens()  → AST (precedence: ; > && || > | > command+redirect)
     → execute_ast()   → fork/exec/wait, result
```

### Precedence (lowest to highest)

```
;
├── && ||
│   ├── |
│   │   ├── command + arguments + redirects
```

## Building

```bash
make                # produces build/husk
make run            # builds and runs
make test           # runs the test suite
make test-valgrind  # runs the test suite under Valgrind
make clean          # removes the build directory
make valgrind       # runs interactively under leak-check, produces build/valgrind.log
make gdb            # runs under gdb
```

## Usage examples

```bash
> ls -la | grep .c | wc -l
> echo hello > out.txt
> cat out.txt >> log.txt
> echo $HOME
> cd $HOME && pwd
> a && b || c
> ls; pwd; whoami
```

Commands can also be read from a file or a pipe:

```bash
./build/husk < script.sh
echo "echo hello | tr a-z A-Z" | ./build/husk
```

## Testing

The tests are end-to-end: they feed commands into husk and compare its output with the expected output.

```
tests/
├── pipes.sh          # commands fed to husk on stdin, one per line
├── pipes.expected    # the exact stdout husk should produce
└── ...
scripts/run_tests.sh  # runs every test and reports PASS / FAIL
```

For each `tests/NAME.sh`, the runner:

- starts husk in a fresh temporary directory, so files created by redirect tests don't end up in the repo
- sets `HUSK_TEST=hello` for the variable-expansion tests
- compares stdout with `tests/NAME.expected` and shows a diff on failure
- stops a test that runs longer than 5 seconds (when `timeout` is available)

`make test-valgrind` runs the same tests under Valgrind; a memory leak or invalid memory access makes the test fail even if the output is correct.

### Adding a test

1. Write the commands in `tests/NAME.sh`.
2. Generate the expected output with bash as a reference, then check it by hand:
   ```bash
   bash < tests/NAME.sh > tests/NAME.expected
   ```
3. Run `make test`.

Keep tests deterministic: avoid output that differs between machines, such as `$HOME`, `$USER`, `ls` of real directories, or dates.

## Continuous integration

Every push and pull request to `main` runs [GitHub Actions](.github/workflows/ci.yml) with two jobs in parallel:

- **build_and_test** — builds with `-Werror`, runs `make test`, then `make test-valgrind`
- **docker** — builds the Docker image (which runs the tests in its build stage) and checks that the final image runs husk

## Docker

The `Dockerfile` uses a multi-stage build:

- **`build` stage** — Ubuntu with `gcc`, `make` and `valgrind`. Compiles husk and runs the test suite; the image build fails if any test fails.
- **final stage** — a plain Ubuntu image containing only the `husk` binary (about 120 MB instead of about 690 MB with the toolchain).

### Run husk

```bash
docker build -t husk .     # build the image (also runs the tests)
docker run --rm -it husk   # start husk interactively
```

`--rm` removes the container when it exits, and `-it` attaches your terminal so you can type into the shell. Leave with `exit` or Ctrl+D.

### Development image

To get the image with the toolchain, build only the `build` stage:

```bash
docker build --target build -t husk-dev .
docker run --rm husk-dev make test            # run the test suite
docker run --rm husk-dev make test-valgrind   # run the tests under Valgrind
```

To work on your current files without rebuilding the image, mount the repository into the container:

```bash
docker run --rm -it -v "$(pwd)":/husk --user "$(id -u):$(id -g)" husk-dev bash
```

Inside the container, `make`, `make test` and `./build/husk` work as usual, and changes you make on the host are visible immediately. `--user` makes files created in `build/` belong to your user instead of root.

Note: the host and the container share the same `build/` directory, so run `make clean` when switching between building on the host and building in the container.

## Known limitations

- Heredocs (`<<`) are not supported
- Background execution (`&`) is not supported
- No line editing (arrow keys, command history) — line-based reading via `getline`
- Other builtins such as `export`/`unset` are not implemented yet; variables can't be set from inside husk (`NAME=value`)
- `$?` (exit status of the last command) is not supported
- A word that mixes quoted and unquoted parts is not kept as one word: `abc"def"` becomes `def`, and `a"b"c` becomes two words, `b` and `c`
