# MINI-SHELL

## OVERVIEW
this project is a mini-shell written in C++. the base code included structures for the tokenizer, basic command handling and simple shell functionality. it's been extended by adding full functionality for command parsing, redirection, pipes, background processes and error handling.

### FOLDER HIERARCHY
```
.
├── Makefile     
├── README.md
├── command.cc        
├── command.h
├── tokenizer.cc  
└── tokenizer.h
```

### FEATURES IMPLEMENTED
* **Command Execution:** creates child processes using `fork()` and executes commands using `execvp()`.
* **Built-in Commands:** 
  * `cd [dir]`: changes the current working directory. defaults to `$HOME` if no argument is provided.
  * `exit`: gracefully terminates the shell without forking.
* **I/O Redirection:** supports input (`<`), output (`>`, `>>`), and error (`2>`) redirection using `dup2()`.
* **Piping (`|`):** connects the output of one command directly to the input of the next using `pipe()`. supports true concurrent pipeline execution by forking all processes before calling `waitpid()`.
* **Background Processes (`&`):** executes commands in the background without forcing the parent shell to `waitpid()`.
* **Signal Handling:** 
  * **SIGINT (Ctrl-C):** intercepted by the parent shell to prevent accidental termination, printing a custom warning message and fresh prompt instead. child processes correctly restore default termination behavior.
  * **SIGCHLD:** caught by the shell to log terminated background and foreground child processes into `child_log.log`. implemented using asynchronous safe system calls (`open`, `write`) to prevent memory deadlocks during rapid process reaping.
* **Robustness and Edge Cases:** includes strict validation loops to safely handle empty commands and leading/trailing pipes without causing segmentation faults.

### BONUS FEATURE
* **Wildcard Expansion:** expands `*` and `?` wildcards to match directory file names (e.g., `echo *`, `ls *.cc`) similar to `csh` behavior.  
└── *Feature's Implementation: used the POSIX `glob()` function to scan the current working directory and replace wildcard tokens with matching file names before parsing.*


### COMPILATION AND EXECUTION
* to compile the shell, run `make` in the project directory
* to start the shell, execute the compiled binary by typing `./myshell` in the terminal
* to cleanup after using the shell, type `make cleanup` in the terminal