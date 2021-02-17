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

int main(const int argc, const char * argv []) {
	// Tests for our arguments
	if (argc != 3) {
		printf("fail: Usage: %s <Input File> <Output File>\n", argv[0]);
		exit(EXIT_FAILURE); 
	}

	// File descriptor pointing to input file
	const char * fn_in = argv[1];
	int fd_in = open(fn_in, O_RDONLY|O_CREAT, 0777);
	int errnum_in = errno;
	if(errnum_in == -1){
		printf("fail: Could not open ifile: %s \n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// File descriptor pointing to output file
	const char * fn_out = argv[2];
	int fd_out = open(fn_out, O_WRONLY|O_CREAT|O_TRUNC, 0777);
	int errnum_out = errno;
	if(errnum_out == -1){
		printf("fail: Could not open ofile: %s \n", argv[2]);
		exit(EXIT_FAILURE);
	}

	// Getting maximum line length from input file
	int REC_LEN = 0;

	// Maximum of 20 digits of length for our popen() call (unknown #of digits).
	char length_call[27] = "wc -L <";
	strcat(length_call, argv[1]);

	FILE *fp_getlen = popen(&length_call[0], "r");
	if (fscanf(fp_getlen, "%d", &REC_LEN) == EOF) {
		printf("fail: Could not find record to determine length of (improperly formatted file?)\n");
		exit(EXIT_FAILURE);
	}
	pclose(fp_getlen);

	if (REC_LEN == 0) {
		printf("fail: Calculated a maximum record length of 0 (improperly formatted file?)\n");
		exit(EXIT_FAILURE);
	}

	// System page size for our buffer
	const int PAGESIZE = getpagesize();

	// Read(2) into this incrementally and work on contained data
	char* read_buffer = calloc(PAGESIZE, sizeof(char));

	// Number of records we processed for later display.
	int rec_num = 0;

	// A place to shove any overflow between pages to use it next iter.
	char overflow_str[PAGESIZE];

	// Is this our first inner scanf pass? Used for overflow prepending.
	int first_pass = 0;

	while (read(fd_in, read_buffer, PAGESIZE) > 0) {

		// Buffer for our scanf().
		char buff_in[REC_LEN];

		// The record we're going to write out, padded to length (+1 for \n).
		char record_out[REC_LEN+1];

		// Ptr to the current position in the read buffer
		char* buff_ptr = read_buffer;

		// Number of chars after our found str, to advance ptr.
		int n;

		//printf("ReadPass\n");

		while (sscanf(buff_ptr, "%s%n", buff_in, &n) == 1) {

			// How many bytes we've processed so far
			int buffpos = -(read_buffer - buff_ptr);

			//printf("scanned:[%s][%d]\n", buff_in, buffpos);

			// If we're going to overflow on this string
			if (buffpos + REC_LEN > PAGESIZE) {
				//printf("Overflow: %s(%d)\n", buff_in, buffpos + REC_LEN);
				strcpy(overflow_str, buff_in); //Copy to overflow and prepend on next iter
				first_pass = 1;
				break;
			}

			if (first_pass) {
				//printf("Prepending %s to %s\n", overflow_str, buff_in);
				sprintf(record_out, "%s\n%s%*u", overflow_str, buff_in, (REC_LEN -(int)strlen(buff_in)+2), ' ');
				first_pass = 0;
				buff_ptr += n;
			}
			else {
				sprintf(record_out, "%s%*u", buff_in, (REC_LEN -(int)strlen(buff_in)+2), ' ');
				// adv. ptr
				buff_ptr += n;				
			}
			
			record_out[REC_LEN] = '\n';
			if (write(fd_out, record_out, sizeof(record_out)/sizeof(record_out[0])) == -1) {
				printf("fail: Error writing database record %s to file %s", record_out, argv[2]);
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

	if (first_pass) //We still have leftover overflow
	{
		char record_out[REC_LEN+1];
		printf("writing %s\n", overflow_str);
		sprintf(record_out, "%s%*u", overflow_str, (REC_LEN -(int)strlen(overflow_str)+2), ' ');
		record_out[REC_LEN] = '\n';
		if (write(fd_out, record_out, sizeof(record_out)/sizeof(record_out[0])) == -1) {
			printf("fail: Error writing database record %s to file %s", record_out, argv[2]);
			exit(EXIT_FAILURE);
		}

		// inc. num processed
		rec_num++;
	}

	// Free our last used read buffer block
	free(read_buffer);

	close(fd_in);
	close(fd_out);

	printf("Successful: %d records\n", rec_num);

	return 1;
}	
