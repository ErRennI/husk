# HUSK

A bash-like Unix shell written from scratch in C. It has its own tokenizer, recursive descent parser, and a fork/exec-based executor.

## Features

- **Command execution** — external commands via `fork`/`execvp`/`waitpid`
- **Pipes** — `cmd1 | cmd2 | cmd3` (chained pipe support)
- **Logical operators** — `&&`, `||` (short-circuit evaluation)
- **Sequential execution** — `;`
- **Redirection** — `<`, `>`, `>>`
- **Quoting** — single quotes (literal) and double quotes (expandable); operators inside quotes are ignored
- **Environment variable expansion** — `$VAR`, including embedded usage like `hello_$USER`
- **Builtin commands** — `cd`, `pwd`, `exit`
- **Signal handling** — Ctrl+C only interrupts the running command, the shell stays alive
- **Leak-free memory management** — verified with Valgrind

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
make          # produces build/husk
make run      # builds and runs
make clean    # removes the build directory
make valgrind # runs under leak-check, produces build/valgrind.log
make gdb      # runs under gdb
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

## Known limitations

- Heredocs (`<<`) are not supported
- Background execution (`&`) is not supported
- No line editing (arrow keys, command history) — line-based reading via `getline`
- Other builtins such as `export`/`unset` are not implemented yet
