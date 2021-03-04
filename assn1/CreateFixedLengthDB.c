/*
* Reads a dictionary of words and converts the file into a fixed record database file.
* Author: Kai Richardson
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "stdcolor.h"

#define MAXIMUM_INPUT_LENGTH 20

/// Handles all of our creation logic really
int main(const int argc, const char * argv []) {
	// Tests for our arguments
	if (argc != 3) {
		printf(RED "fail:" reset " " UWHT "Usage:" reset " %s <Input File> <Output File>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// File descriptor pointing to input file
	const char * fn_in = argv[1];
	int fd_in = open(fn_in, O_RDONLY|O_CREAT, 0777);
	int errnum_in = errno;
	if(errnum_in == -1){
		printf(URED "fail:" reset " Could not open ifile: %s \n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// File descriptor pointing to output file
	const char * fn_out = argv[2];
	int fd_out = open(fn_out, O_WRONLY|O_CREAT|O_TRUNC, 0777);
	int errnum_out = errno;
	if(errnum_out == -1){
		printf(URED "fail:" reset " Could not open ofile: %s \n", argv[2]);
		exit(EXIT_FAILURE);
	}

	/// Maximum line length determined from input file
	int REC_LEN = 0;

	char length_call[MAXIMUM_INPUT_LENGTH+7] = "wc -L <";
	strcat(length_call, argv[1]);

	FILE *fp_getlen = popen(&length_call[0], "r");
	if (fscanf(fp_getlen, "%d", &REC_LEN) == EOF) {
		printf(URED "fail:" reset " Could not find record to determine length of (improperly formatted file?)\n");
		exit(EXIT_FAILURE);
	}
	pclose(fp_getlen);

	if (REC_LEN == 0) {
		printf(URED "fail:" reset " Calculated a maximum record length of 0 (improperly formatted file?)\n");
		exit(EXIT_FAILURE);
	}

	/// System page size for our buffer
	const int PAGESIZE = getpagesize();

	/// Read(2) into this incrementally and work on contained data
	char* read_buffer = calloc(PAGESIZE, sizeof(char));

	/// Number of records we processed for later display.
	int rec_num = 0;

	/// A place to shove any overflow between pages to use it next iter.
	char overflow_str[PAGESIZE];

	/// Is this our first inner scanf pass? Used for overflow prepending.
	int first_after_over = 0;

	/// The record we're going to write out, padded to length (+1 for \n).
	char record_out[REC_LEN+1];

	// Main reading loop
	while (read(fd_in, read_buffer, PAGESIZE) > 0) {

		/// Buffer for our scanf().
		char buff_in[REC_LEN];

		/// Ptr to the current position in the read buffer
		char* buff_ptr = read_buffer;

		/// Number of chars in our found str (+\n if there), to advance ptr.
		int n;

		//string, whitespace, newline
		while (sscanf(buff_ptr, "%s%*c%n", buff_in, &n) == 1) {

			// How many bytes we've processed so far
			int buffpos = -(read_buffer - buff_ptr);

			// Weird thing to handle scanf ignoring newlines,
			// we look at this to conditionally append \n.
			char likely_newline = buff_ptr[n-1];

			//printf("scanned:[%s][%d]\n", buff_in, buffpos);

			// If we're going to overflow on this string
			if ((buffpos + n) > PAGESIZE) {
				//printf("Overflow: %s(%d)\n", buff_in, buffpos + REC_LEN);

				strcpy(overflow_str, buff_in); //Copy to overflow and prepend on next iter

				// need to handle potential missed newline from the sscanf
				if (likely_newline == '\n') {
					overflow_str[n-1] = '\n';
					overflow_str[n] = '\0';
				}

				first_after_over = 1;
				break;
			}

			// Was the previous iteration an overflow?
			if (first_after_over) {
				//printf("Prepending %s to %s\n", overflow_str, buff_in);

				if (buff_ptr[buffpos] == '\n') {
					// We need another record to manually write for our overflow we've just done
					char over_record_out[REC_LEN+1];

					sprintf(over_record_out, "%s%*c\n", overflow_str, (REC_LEN -(int)strlen(overflow_str)), ' ');

					if (write(fd_out, over_record_out, sizeof(over_record_out)/sizeof(over_record_out[0])) == -1) {
						printf(URED "fail:" reset " Error writing database record %s to file %s", record_out, argv[2]);
						exit(EXIT_FAILURE);
					}

					sprintf(record_out, "%s%*c\n", buff_in, (REC_LEN -(int)strlen(buff_in)), ' ');
					record_out[REC_LEN] = '\n';

					rec_num++; //We dealt with an additional record
				}
				else {
					sprintf(record_out, "%s%s%*c\n", overflow_str, buff_in, (REC_LEN -(int)strlen(buff_in)-(int)strlen(overflow_str)), ' ');
				}
				first_after_over = 0;
				buff_ptr += n;
			}
			else {
				//If we're not at the end of a line, don't pad with spaces
				if (likely_newline != '\n') {
					//Copy to overflow and prepend on next iter
					strcpy(overflow_str, buff_in);
					first_after_over = 1;
					break;
				}
				else {
					sprintf(record_out, "%s%*c", buff_in, (REC_LEN -(int)strlen(buff_in)), ' ');
					record_out[REC_LEN] = '\n';
				}

				// adv. ptr
				buff_ptr += n;
			}

			if (write(fd_out, record_out, sizeof(record_out)/sizeof(record_out[0])) == -1) {
				printf(URED "fail:" reset " Error writing database record %s to file %s", record_out, argv[2]);
				exit(EXIT_FAILURE);
			}
			// inc. num processed
			rec_num++;
		}

		// Create new memory block for next iteration so we have a clean working space
		free(read_buffer);
		read_buffer = calloc(PAGESIZE, sizeof(char));

		//printf("--------PAGE--------\n");
	}

	if (first_after_over) //We still have leftover overflow (only last block)
	{
		sprintf(record_out, "%s%*u", overflow_str, (REC_LEN -(int)strlen(overflow_str)+2), ' ');
		record_out[REC_LEN] = '\n';
		if (write(fd_out, record_out, sizeof(record_out)/sizeof(record_out[0])) == -1) {
			printf(URED "fail:" reset " Error writing database record %s to file %s", record_out, argv[2]);
			exit(EXIT_FAILURE);
		}

		// inc. num processed
		rec_num++;
	}

	// Free our last used read buffer block
	free(read_buffer);

	// Close our file descriptors
	close(fd_in);
	close(fd_out);

	printf(GRN "Creation Successful: " BWHT "%d" reset " records\n", rec_num);

	return EXIT_SUCCESS;
}
