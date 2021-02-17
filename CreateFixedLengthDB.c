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
	fscanf(fp_getlen, "%d", &REC_LEN);
	pclose(fp_getlen);

	if (REC_LEN == 0) {
		printf("fail: Calculated a maximum record length of 0 (improperly formatted file?)\n");
		exit(EXIT_FAILURE);
	}

	// System page size for our buffer
	int PAGESIZE = getpagesize();
	// Read(2) into this incrementally and work on contained data
	char read_buffer[PAGESIZE];

	// Number of records we processed.
	int rec_num = 0;

	while (read(fd_in, read_buffer, PAGESIZE) > 0) {

		// Buffer for our scanf().
		char buff_in[REC_LEN];
		// The record we're going to write out, padded to length (+1 for \n).
		char record_out[REC_LEN+1];
		// Ptr to the current position in the read buffer
		char* buff_ptr = read_buffer;
		// Number of chars after our found str, to advance ptr.
		int n;

		//TODO: need to handle overflow logic here

		while (sscanf(buff_ptr, "%s%n", buff_in, &n) == 1) {
			sprintf(record_out, "%s%*u", buff_in, (REC_LEN -(int)strlen(buff_in)+2), ' ');
			record_out[REC_LEN] = '\n';
			write(fd_out, record_out, sizeof(record_out)/sizeof(record_out[0]));

			// inc. num processed
			rec_num++;
			// adv. ptr
			buff_ptr += n;
		}

		//TODO: check if there's stuff left in array and store it in an overflow if so

	}

	close(fd_in);
	close(fd_out);

	printf("Successful: %d records\n", rec_num);

	return 1;
}	
