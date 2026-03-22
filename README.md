smallsh is a custom Unix-like shell implemented in C, designed to replicate core features of standard shells like Bash. This project demonstrates process management, signal handling, and low-level system programming using the Unix API.

Features
- Custom command prompt (:) for user input
- Execution of commands using fork() and execvp()
- Built-in commands:
  - cd – change directory
  - exit – terminate the shell
  - status – display exit status of last foreground process
- Support for input (<) and output (>) redirection
- Foreground and background process execution (&)
- Process management using waitpid()
- Shell variable expansion ($$ -> process ID)
- Custom signal handling:
  - SIGINT (Ctrl+C) handling
  - SIGTSTP (Ctrl+Z) toggles foreground-only mode
- Handles blank lines and comment lines (#)

- 
