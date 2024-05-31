# ECS 150: Shell (sshell)

## Project Overview

This project aims to implement a simple shell (sshell) that accepts and executes user-supplied commands with various features such as output redirection and piping. The goal is to understand important UNIX system calls by implementing a basic shell similar to well-known UNIX shells like bash and zsh.

## Features

### Core Features
1. **Execution of Commands**: Run user-supplied commands with optional arguments.
2. **Builtin Commands**: Implement `exit`, `cd`, and `pwd`.
3. **Output Redirection**: Redirect the standard output of commands to files using `>`.
4. **Piping**: Compose commands via piping using `|`.

### Extra Features
1. **Output Redirection Appending**: Append to files using `>>`.
2. **Simple `ls`-like Builtin Command**: List directory contents with `sls`.

## Constraints

- Written in C and compiled with GCC.
- Only standard functions provided by GNU C Library (libc) can be used.
- Follow a consistent coding style and proper commenting.

## Shell Specifications

### Command Line
- Maximum length: 512 characters.
- Maximum arguments per program: 16 non-null arguments.
- Maximum length of individual tokens: 32 characters.
- Programs are searched according to the `$PATH` environment variable.

### Builtin Commands
- `exit`: Exits the shell.
- `cd`: Changes the current working directory.
- `pwd`: Displays the current working directory.

### Output Redirection
- Use `>` to redirect output to a file.
- Use `>>` to append output to a file.

### Piping
- Use `|` to connect multiple commands.
- Supports up to three pipe signs (`|`) to connect four commands.

## Error Management

### Parsing Errors
- "Error: too many process arguments"
- "Error: missing command"
- "Error: no output file"
- "Error: cannot open output file"
- "Error: mislocated output redirection"

### Launching Errors
- "Error: cannot cd into directory"
- "Error: command not found"

## Reference Program and Testing

A reference program is available on the CSIF at `/home/cs150jp/public/p1/sshell_ref`. Your shell implementation should generate the same output as the reference program.

### Testing Example
```bash
echo -e "echo Hello\nexit\n" | ./sshell >& your_output
echo -e "echo Hello\nexit\n" | ./sshell_ref >& ref_output
diff your_output ref_output
```
