#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef HELPERS_H
#include "helpers.h"
#endif

/// Tests if the given redirection symbol is in the arguments. Sets the given file ptr if true.
/// Returns an error code of -1 if there's a syntax error (no filename after symbol)
int test_redirect(char **args, char **input_file, char symbol) {

	// Scan input for our input char
	for(int i = 0; args[i] != NULL; i++) {

		if(args[i][0] == symbol) {
			args[i] = NULL;

			// Get the filename after the symbol
			if(args[i+1] == NULL) { // nothing specified after
				return ERROR;
			}
			else {
				*input_file = args[i+1];
			}

			// Remove it so it doesn't get executed, shift rest
			for(int j = i; args[j-1] != NULL; j++) {
				args[j] = args[j+2];
			}
			return TRUE; // yes we redirect
		}
	}

	return FALSE;
}

/// Tests to see if we have a pipe, sets pipe args if so
int test_pipe(char **args, char **pipe_args) {

	// Scan input for our input char
	for(int i = 0; args[i] != NULL; i++) {

		if(args[i][0] == PIPEC) {
			args[i] = NULL;

			// Get everything after
			if(args[i+1] == NULL) { // nothing specified after
				return ERROR;
			}
			else {
				// Current position in the pipe args
				int idx = 0;

				// Move the rest of the arguments into our pipe land
				for(int j = i; args[j+1] != NULL; j++) {
					pipe_args[idx] = args[j+1];
					args[j+1] = NULL;
					idx++;
				}
				pipe_args[idx+1] = NULL;
			}
			return TRUE; // yes we pipe
		}
	}

	return FALSE;
}

/// See if our last argument is an amp
int has_backgc(char **args) {
	int i;

	// Get to the end of our arguments excluding null entry
	for(i = 1; args[i] != NULL; i++);

	if(args[i-1][0] == BACKGC) {
		args[i-1] = NULL;
		return TRUE;
	}
	else {
		return FALSE;
	}

	return FALSE;
}

/// See if we have timed execution, returns value in seconds
int has_timed(char **args) {

	// Time to wait for process (seconds)
	int ret = 0;

	// Scan input for our timed char
	for(int i = 0; args[i] != NULL; i++) {

		if(args[i][0] == TIMEC) {
			args[i] = NULL;

			// Get the filename after the symbol
			if(args[i+1] == NULL) { // nothing specified after
				return ERROR;
			}
			else {
				ret = atoi(args[i+1]);
			}

			// Remove it so it doesn't get executed, shift rest
			for(int j = i; args[j-1] != NULL; j++) {
				args[j] = args[j+2];
			}
		}
	}
	return ret;
}