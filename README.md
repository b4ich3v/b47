# b47

## Description

**b47** is a high-performance, minimalist Unix shell engineered for advanced users demanding granular command-line control and extensibility. It features a sophisticated parsing engine capable of constructing and interpreting complex syntactic structures, including nested pipelines, conditional execution flows, and comprehensive I/O redirection semantics. Designed with modularity and robustness in mind, b47 implements a rich subset of POSIX-compliant shell behaviors, ensuring precise command evaluation and rigorous process management typically reserved for industrial-grade shell environments:

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

| Operator           | Description                                                                                              |
|--------------------|----------------------------------------------------------------------------------------------------------|
| `\|`               | Pipe: Passes the standard output of one command as the standard input to the next.                        |
| `&&`               | AND: Executes the following command only if the previous command exits successfully (exit status 0).    |
| `\|\|`             | OR: Executes the following command only if the previous command fails (non-zero exit status).             |
| `;`                | Sequence: Executes commands sequentially, regardless of the previous command’s result.                     |
| `&`                | Background execution: Runs the preceding command asynchronously, returning control immediately to the shell. |
| `()`               | Grouping: Groups commands or pipelines into a subshell, enabling combined redirections or logical operations. |
| `<`, `>`, `>>`, `<<` | Redirections:                                                                                         |
|                    | - `<` : Redirects input from a file to the command.                                                     |
|                    | - `>` : Redirects output to a file, overwriting existing content.                                       |
|                    | - `>>`: Redirects output to a file, appending to existing content.                                      |
|                    | - `<<`: Heredoc: Takes multiline input until a delimiter line is found, feeding it to command input.   |
| `$VAR`, `${VAR}`    | Variable expansion: Replaces variables with their current values in the environment.                    |
| `~`                | Tilde expansion: Expands to the current user's home directory or another user’s home when followed by username. |
| `"` and `'`        | Quoting:                                                                                               |
|                    | - `"` (Double quotes): Expands variables and allows command substitution inside.                       |
|                    | - `'` (Single quotes): Treats everything literally, no expansion or substitution.                      |
| `\` (Backslash)    | Escape character: Escapes the next character, preventing its special interpretation.                   |
| `$(...)`           | Command substitution: Executes the command inside and replaces the syntax with its output.             |
| `` `...` ``        | Alternative command substitution syntax (deprecated but supported).                                   |
| `jobs`             | Lists active background and stopped jobs.                                                            |
| `fg`, `bg`         | Foreground/background job control commands to resume jobs in foreground or background.                |

---

### Additional Notes:

- **Subshell Execution:** Using `()` creates a subshell environment, isolating variable changes and redirections inside the group.
- **Command Chaining:** Combining operators allows complex flow control, e.g., `cmd1 && cmd2 || cmd3; cmd4 &`.
- **Signal Handling:** The shell manages job control signals (e.g., SIGINT, SIGTSTP) to properly handle foreground and background tasks.
- **History Expansion:** Integrated with GNU Readline, allowing use of `!` for recalling previous commands (if implemented).

