# b47

## Description

**b47** is a rock-solid, minimalistic Unix shell designed for power users who want both flexibility and control. It parses and executes complex command lines with surgical precision, supporting advanced shell features usually found only in mature shells:

- **Robust Command Parsing:** Supports pipelines (`|`), command grouping (`()`), logical operators (`&&`, `||`), sequential execution (`;`), and background processes (`&`).
- **Masterful Redirection Handling:** Fully implements input/output redirections (`<`, `>`, `>>`) and heredocs (`<<`), letting you manipulate data streams like a pro.
- **Environment Variable Engine:** Seamlessly manages environment variables with export/unset capabilities, ensuring your shell session is both dynamic and scriptable.
- **Foreground/Background Job Control:** Tracks, updates, and controls processes with detailed job management, allowing you to juggle multiple tasks effortlessly.
- **Signal Savvy:** Captures and handles UNIX signals to keep the shell responsive and stable, ignoring interruptions when needed but gracefully managing child processes.
- **User-Friendly Command History:** Leverages GNU Readline for smart history navigation and line editing.
- **Advanced Expansion:** Handles variable expansions and command preprocessing to let you build complex commands intuitively.

---

## Project Structure

| File           | Purpose                                                      |
|----------------|--------------------------------------------------------------|
| `main.c`       | The heart of b47: command input, shell loop, and initialization. |
| `parser.c/h`   | Syntax analyzer: tokenizes input and builds the command AST.  |
| `process.c/h`  | Job control and process lifecycle management.                 |
| `variable.c/h` | Environment variables: set, unset, export, and manage vars.   |
| `shell.h`      | Core globals (like shell process group ID) shared across modules. |
| `executor.c/h` | Command executor handling built-in and external commands.     |
| `expand.c/h`   | Handles variable and tilde expansions inside commands.        |
| `builtins.c/h` | Implements built-in commands like `cd`, `exit`, `jobs`, etc.  |
| `monitor.h`    | (Optional) System resource monitoring utilities.              |

---

## Core Data Structures

| Type         | Description                                                                                  |
|--------------|----------------------------------------------------------------------------------------------|
| `node_t`     | Abstract syntax tree node for commands, pipelines, logical connectors, and redirections.    |
| `redir_t`    | Represents a command’s input/output redirections, including heredoc data.                   |
| `process`    | Tracks running process metadata: PID, command string, and execution state.                   |
| `var_entry`  | Linked list node for environment variables, including export flags.                         |

---

## Supported Syntax and Operators

| Operator    | Description                                                             |
|-------------|-------------------------------------------------------------------------|
| `|`         | Pipe: Passes output of one command as input to the next.                |
| `&&`        | AND: Execute next command only if the previous succeeds.                |
| `||`        | OR: Execute next command only if the previous fails.                    |
| `;`         | Sequence: Run commands sequentially, regardless of success.             |
| `&`         | Background: Run command asynchronously without blocking the shell.      |
| `()`        | Grouping: Combine multiple commands or pipelines into a single unit.    |
| `<`, `>`, `>>`, `<<` | Redirection: Input from file, overwrite or append output, heredoc input. |

---

## Installation & Usage

Compile the project with:

```bash
gcc -o b47 main.c parser.c process.c variable.c executor.c expand.c builtins.c -lreadline
```

Run your new shell:

```bash
./b47
```

Examples of badass commands to try:

```bash
(ls -l | grep ".c") && echo "C files found!" > output.log &
export PATH=$PATH:/custom/bin
jobs
fg %1
```

---

## Features that Make b47 a Beast

- **Precision Variable Handling:** Define and export variables that persist across the shell and subprocesses.
- **Comprehensive Job Control:** Spawn and manage foreground and background jobs with real-time status updates.
- **Powerful Heredoc Support:** Multiline input blocks (`<<`) for scripting complex commands.
- **Signal Mastery:** Ignores unwanted interrupts on the shell itself but handles child process signals cleanly.
- **Readline Integration:** Smart command line editing and history for lightning-fast interaction.
- **Expandable Architecture:** Modular components make it easy to extend with new features.

---

## Example Complex Command Breakdown

```
(cat file.txt | grep hello) && echo "Found" > output.txt &
```

- **Grouping** with `()` runs the pipeline as a unit.
- **Pipeline** connects `cat` output to `grep`.
- **Logical AND (`&&`)** runs `echo` only if the pipeline succeeds.
- **Redirection (`>`)** writes output to `output.txt`.
- **Background Execution (`&`)** detaches the command, freeing the terminal immediately.

