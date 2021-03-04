
#ifndef HELPERS_H
#define HELPERS_H

/// Exit our main loop
#define EXIT 0
/// Continue our main loop
#define CONT 1

// Return codes from helpers

/// Indicates an error
#define ERROR -1
/// duh
#define TRUE 1
/// duh
#define FALSE 0

/// Size of our latter arg string size
#define STRINGSIZE 100

// Special symbols

/// Input redirection
#define INPUTC '<'
/// Output redirection
#define OUTPUTC '>'
/// Background process
#define BACKGC '&'
/// Pipe
#define PIPEC '|'
/// Timed execution
#define TIMEC '@'

/// Sees if we redirect input, set the vars
int test_redirect(char **args, char **input_file, char symbol);

/// Tests to see if we have a pipe
int test_pipe(char **args, char **pipe_dest);

/// See if our last argument is an amp
int has_backgc(char **args);

/// See if we have timed execution, returns value in seconds
int has_timed(char **args);

#endif