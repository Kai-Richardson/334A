#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>

#ifndef MAIN_H
#include "cs334.h"
#endif

#ifndef HELPERS_H
#include "helpers.h"
#endif

// CS334 Shell
// By Kai Richardson
//
// Note: I internally refer to the shell as `zesh`, because having nums in fn names is cursed.

/// Basically just runs the loop, it's easier to deal with a void main loop.
int main(int argc, char **argv) {

	// Run main loop
	zesh_loop();

	return EXIT_SUCCESS;
}


/// Our main loop: read, tokenize, execute
void zesh_loop(void) {
	/// Line we read in
	char *line;
	/// Our arguments that we then operate on
	char **args;
	/// Status code: 0 to exit, 1 to continue
	int stat;
	/// Our arguments for a possible piped command
	char **pipe_args = NULL;

	do {
		printf("cs334:) "); // prompt

		line = zesh_readl();
		args = zesh_tok(line);


		// Allocate pipe argument array ahead of time
		pipe_args = calloc(STRINGSIZE, sizeof(char*));
		if (!pipe_args) {
			perror("Pipe Memory allocation failed");
			exit(EXIT_FAILURE);
		}

		stat = zesh_run(args, pipe_args);

		free(line);
		free(args);
		free(pipe_args);
	}
	while (stat == CONT); // we perform a do-while so we guarentee execute once
}


/// Reads a line of input from stdin and returns it
char *zesh_readl(void) {
	/// Line ptr
	char *line = NULL;
	// Buffer size that getline allocates
	size_t bufsize = 0;

	if (getline(&line, &bufsize, stdin) == -1){
		if (feof(stdin)) { //Did we get an eof (EOL/interrupt)?
			exit(EXIT_SUCCESS);
		} else  {
			perror("Unable to read input");
			exit(EXIT_FAILURE);
		}
	}

	return line;
}


#define TOK_BUFF_SIZE 32
char **zesh_tok(char *line) {
	/// Size of our token buffer
	int bufsize = TOK_BUFF_SIZE;
	/// Array of our tokens
	char **tokens = malloc(bufsize * sizeof(char*));
	/// Current position in the arr
	int pos = 0;
	/// Current processing token we get from strtok
	char *token;

	if (!tokens) {
		perror("Memory allocation failed");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, METACHAR_TOKS);
	while (token != NULL) { // Iterate while we have tokens from strtok
		tokens[pos] = token;
		pos++;

		// Are we going to go over the buffer? If so, expand by buffer size
		if (pos >= bufsize) {
			bufsize += TOK_BUFF_SIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));

			if (!tokens) {
				perror("Memory reallocation failed");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, METACHAR_TOKS); // get next token
	}

	tokens[pos] = NULL; // append null terminator

	return tokens;
}

/// Tests that we have a valid program, or builtin
int zesh_run(char **args, char **pipe_buff)
{
	int background = FALSE;

	int output_code;
	char *output_file = NULL;

	int input_code;
	char *input_file = NULL;

	int pipe_code;

	int time_seconds;

	// Empty command?
	if (args[0] == NULL) {
		return CONT;
	}

	// Is this the exit command?
	if (strcmp(args[0], exit_cmd_name) == 0) {
		return zesh_exit();
	}

	background = (has_backgc(args) == TRUE);

	// Check for redirected input
	input_code = test_redirect(args, &input_file, INPUTC);
	if (input_code == ERROR) {
		printf("Filename not detected after %c character.\n", INPUTC);
		return CONT;
	}

	// Check for redirected input
	output_code = test_redirect(args, &output_file, OUTPUTC);
	if (output_code == ERROR) {
		printf("Filename not detected after %c character.\n", OUTPUTC);
		return CONT;
	}

	pipe_code = test_pipe(args, pipe_buff);
	if (pipe_code == ERROR) {
		printf("Command not detected after %c character.\n", PIPEC);
		return CONT;
	}

	time_seconds = has_timed(args);

	// Executes
	return zesh_exec(args, background, input_file, output_file, pipe_code, pipe_buff, time_seconds);
}


/**
 * Main exec.
 *
 * Params:
 * Arguments
 * Background/blocking process or not
 * Input file for <
 * Output file for >
 * Pipe code to determine if we have a pipe or not
 * Pipe data to run if pipe code is true
 * Seconds we wait to kill the process if true
 */
int zesh_exec(char **args, int background, char *input_file, char *output_file, int pipe_code, char** pipe_data, int killtime) {
	/// Our pid from fork
	pid_t pid;
	/// Status code
	int stat;
	/// Our pipe that we can use for piped command
	int pipefd[2];

	// Various masks and struct for our timeout functionality required by sigtimedwait
	sigset_t mask;
	sigset_t orig_mask;
	struct timespec timeout;

	// set our timeout struct to the time we obtained from args
	timeout.tv_sec = killtime;

	if (killtime) {
		// set signal to be the child term. signal
		sigemptyset(&mask);
		sigaddset(&mask, SIGCHLD);

		// check our created signal mask
		if (sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
				perror ("Singal error: sigprocmask");
				return CONT;
		}
	}

	// Create our pipe so we can do stuff with it
  if(pipe(pipefd) == -1){
    perror("Error creating pipe");
    exit(EXIT_FAILURE);
  }

	// Hansel & Gretel
	if ((pid = fork()) < 0) {
		perror("Error creating process");
	}
	else if (pid == 0) {
		// Child 1

		// handle child pipe
		if (pipe_data[0] != NULL) {
			// Close reading
			close(pipefd[0]);

			// Set stdout to be the writing end of the pipe
			dup2(pipefd[1], STDOUT_FILENO);

			// Close writing
			close(pipefd[1]);
		}

		// handle possible input or output files
		if (input_file != NULL) {
			freopen(input_file, "r", stdin);
		}
		if (output_file != NULL) {
			freopen(output_file, "w+", stdout);
		}

		// actually exec
		if (execvp(args[0], args) == -1) {
			printf("%s: command not found\n", args[0]);
			exit(EXIT_FAILURE);
		}

		exit(EXIT_SUCCESS);
	}
	else {
		// Handle parent pipe
		if (pipe_data[0] != NULL) {

			// Fork off child 2 to execute
			if ((pid = fork()) < 0) {
				perror("Error creating process");
			}
			if (pid == 0) {
				// Child 2

				// Close writing
				close(pipefd[1]);

				// Set stdout to be the read end of the pipe
				dup2(pipefd[0], STDIN_FILENO);

				// Close reading
				close(pipefd[0]);

				// Executes our command after pipe
				if (execvp(pipe_data[0], pipe_data) == -1) {
					perror("Error executing piped command");
					exit(EXIT_FAILURE);
				}

				exit(EXIT_SUCCESS);
			}
		}

		// Parent parent logic
		if((close(pipefd[0]) < 0 ) || (close(pipefd[1]) < 0 )){
			perror("PPipe close error");
			exit(EXIT_FAILURE);
		}

		if (killtime) {
			do {
				if (sigtimedwait(&mask, NULL, &timeout) < 0) {
						if (errno == EINTR) {
								// Interrupted by a signal other than SIGCHLD. Therefore, no timeout statement
								continue;
						}
						else if (errno == EAGAIN) {
								printf("Timeout reached: %ld seconds. Killing process...\n", timeout.tv_sec);
								kill(pid, SIGKILL);
						}
						else {
								perror("Timing error: sigtimedwait");
								return CONT;
						}
				}
				break;
			}
			while(TRUE);
		}

		// do-while for same reason as master loop (guar. exec.)
		if (!background) {
			do {
				waitpid(pid, &stat, WUNTRACED);
			}
			while (!WIFSIGNALED(stat) && !WIFEXITED(stat));
		}
	}

	return CONT;
}


/// Built-in exit command since I don't want to CTRL+C it every time.
int zesh_exit() {
	return EXIT;
}