#ifndef MAIN_H
#define MAIN_H

/// Metacharacters which we need to tokenize on
#define METACHAR_TOKS " \r\n"

/// Built-in exit command since I don't want to break it every time.
int zesh_exit();

/// Our main loop: read, tokenize, execute
void zesh_loop(void);

/// Reads a line of input from stdin and returns it
char *zesh_readl(void);

/// Tokenizes our line into an null-term. array
char **zesh_tok(char *line);

/// Tests that we have a valid program, or builtin
int zesh_run(char **args, char **pipe_buff);

/// Main exec.
int zesh_exec(char **args, int background, char *input_file, char *output_file, int pipe_code, char** pipe_data, int killtime);

/// Name of our exit command
char *exit_cmd_name = "exit";

#endif