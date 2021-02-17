
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "reader.h"

int main(const int argc, const char * argv []) {
	if (argc != 4) {
		printf("Usage: %s <Input File> <Search Text> <Mode>\n", argv[0]);
		exit(EXIT_FAILURE); 
	}

	const char* search_text = argv[2];
	const int mode = atoi(argv[3]);

	const char * filename = argv[1];
	int fd_in = open(filename, O_RDONLY|O_CREAT, 0777);
 	int errnum = errno;	
 	if(errnum == -1){
		printf("Could not open file: %s \n", argv[1]);
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
		printf("fail: Didn't find any records (improperly formatted file?)\n");
		exit(EXIT_FAILURE);
	}

	// Number of records found
	int rec_num = 0;
	// Stat struct to get bytesize
	struct stat struct_stat;	
	if (stat(argv[1], &struct_stat) == -1) {
		perror("Could not run sys call: stat\n");
	}
    rec_num = (int) (struct_stat.st_size / REC_LEN);
	printf("Number of Records: %i\n", rec_num);

	// System page size for our buffer
	int PAGESIZE = getpagesize();
	// Read(2) into this incrementally and work on contained data
	char read_buffer[PAGESIZE];

	// Number of records we searched through
	int searched = 0;

	while (read(fd_in, read_buffer, PAGESIZE) > 0) {
		int return_code = EXIT_SUCCESS;
		switch(mode) {
			case 0: //sequential
				return_code = SequentialSearch(read_buffer, search_text);
			case 1: //interpolation
				return_code = InterpolationSearch(read_buffer, search_text);
			default:
				return_code = SequentialSearch(read_buffer, search_text);
		}
		if (return_code != EXIT_SUCCESS) {
			printf("fail: Error %d encountered while searching.", return_code);
			exit(EXIT_FAILURE);
		}
	}

	close(fd_in);

	return EXIT_SUCCESS;
}	

int SequentialSearch(char* input, const char* searchstr) {
	return EXIT_SUCCESS;
}

int InterpolationSearch(char* input, const char* searchstr) {
	return EXIT_SUCCESS;
}