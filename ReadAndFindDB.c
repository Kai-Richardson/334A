
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "stdkai.h"
#include "reader.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define CODE_NOTFOUND 0

int main(const int argc, const char * argv []) {
	if (argc != 4) {
		printf(RED "fail:" reset " " UWHT "Usage:" reset " %s <Input File> <Search Text> <Mode>\n", argv[0]);
		exit(EXIT_FAILURE); 
	}

	const char* search_text = argv[2];
	const int mode = atoi(argv[3]);

	const char * filename = argv[1];
	int fd_in = open(filename, O_RDONLY|O_CREAT, 0777);
 	int errnum = errno;	
 	if(errnum == -1){
		printf(URED "fail:" reset " Could not open file: %s \n", argv[1]);
		exit(EXIT_FAILURE);
	}


	// Getting maximum line length from input file
	int REC_LEN = 0;

	// Maximum of 20 digits of length for our popen() call (unknown #of digits).
	char length_call[27] = "wc -L <";
	strcat(length_call, argv[1]);

	FILE *fp_getlen = popen(&length_call[0], "r");
	if (fscanf(fp_getlen, "%d", &REC_LEN) == EOF) {
		printf(URED "fail:" reset " Could not find record to determine length (improperly formatted file?)\n");
		exit(EXIT_FAILURE);
	}
	pclose(fp_getlen);

	if (REC_LEN == 0) {
		printf(URED "fail:" reset " Didn't find any records (improperly formatted file?)\n");
		exit(EXIT_FAILURE);
	}

	// Number of records found
	int rec_num = 0;
	// Stat struct to get bytesize
	struct stat struct_stat;	
	if (stat(argv[1], &struct_stat) == -1) {
		perror(URED "fail:" reset " Could not run sys call: stat\n");
	}
    rec_num = (int) (struct_stat.st_size / REC_LEN);
	printf("Searching " BWHT "%i" reset " records with mode " YEL "%d" reset "...\n", rec_num, mode);

	// System page size for our buffer
	int PAGESIZE = getpagesize();
	// Read(2) into this incrementally and work on contained data
	char read_buffer[PAGESIZE];

	// Number of records we searched through for later display
	int read_num = 0;

	// Number of pages we went through
	int pages = 1;

	// Return code from our search function
	int return_code = CODE_NOTFOUND;
	while (read(fd_in, read_buffer, PAGESIZE) > 0) {
		pages++;
		switch(mode) {
			case 0: //sequential
				return_code = MAX(return_code, SequentialSearch(read_buffer, search_text, &read_num, REC_LEN, PAGESIZE));
				break;
			case 1: //interpolation
				return_code = MAX(return_code, InterpolationSearch(read_buffer, search_text, &read_num, REC_LEN, PAGESIZE));
				break;
		}
		if (return_code != CODE_NOTFOUND) break;
	}

	close(fd_in);

	if (return_code < CODE_NOTFOUND){
		printf(URED "fail:" reset " Error %d encountered while searching.\n", return_code);
		return EXIT_FAILURE;
	}
	else if (return_code == CODE_NOTFOUND) {
		printf(URED "fail:" reset " String " URED "%s" HRED " not found " reset "in given input. %d records read.\n", search_text, read_num);
		return EXIT_FAILURE;
	}
	else {
		printf(GRN "Success:" reset " String " UWHT "%s" reset " found in given input. \n", search_text);
		printf("%d records read, found at position %d.\n", read_num, return_code*pages);
	}

	return EXIT_SUCCESS;
}	

/**
 * Searches sequentially through a chunked char array input for a given string.
 * 
 * Return:	* negative int error code if failure encountered
 * 			* CODE_NOTFOUND if not found
 * 			* positive int of position if found
 */
int SequentialSearch(char* input, const char* searchstr, int* searchnum, int reclen, int PAGESIZE) {

	if (input == NULL) perror(URED "fail:" reset " bad input\n");
	if (searchnum == NULL || searchstr == NULL) perror(URED "fail:" reset " bad search target\n");

	// Number of records we need to store
	int num_records = (PAGESIZE / (reclen+1))+4; //TODO: +4 due to overflow garbage data
	// Number of records * Length of each record (+1 for \0)
	int arr_size  = (num_records * (reclen+1));
	

	// Setup array
	char** search_arr = calloc(arr_size, sizeof(char*));
	if (search_arr == NULL) perror(URED "fail:" reset " malloc1 failed to allocate\n");

	for (int i = 0; i < num_records; i++) // Allocating interior
	{
		search_arr[i] = calloc(reclen + 1, sizeof(char));
		if (search_arr[i] == NULL) perror(URED "fail:" reset " malloc2 failed to allocate\n");
	}

	// Number of chars after our found str, to advance ptr.
	int n_chars = 0;
	// Current processing position in array
	int n_pos = 0;
	// Ptr to the current position in the passed input slice
	char* input_ptr = input;

	// Scan through our input page and shove it into the array
	char buffer[reclen+1];
	while (sscanf(input_ptr, "%s%n", buffer, &n_chars) == 1)
	{
		if (n_pos >= num_records) break;
		//printf("Copying %s to %d.\n", buffer, n_pos);
		if (strncpy(search_arr[n_pos], buffer, reclen+1) == NULL) {
			perror(URED "fail:" reset "strncpy failed to copy\n");
		}
		n_pos++;
		input_ptr += n_chars;
	}
	
	// The actual searching part
	for(int i = 0; i < num_records; ++i)
	{
		(*searchnum)++;
		if(!strcmp(search_arr[i], searchstr))
		{
			// We found it! Cleaning up and returning position.
			cleanup(search_arr, num_records);
			return i+1; //pos, not idx
		}
	}

	cleanup(search_arr, num_records);
	return CODE_NOTFOUND;
}

/**
 * Searches via binary search through a chunked char array input for a given string.
 * 
 * Return:	* negative int error code if failure encountered
 * 			* CODE_NOTFOUND if not found
 * 			* positive int of position if found
 */
int InterpolationSearch(char* input, const char* searchstr, int* searchnum, int reclen, int PAGESIZE) {

	if (input == NULL) perror(URED "fail:" reset " bad input\n");
	if (searchnum == NULL || searchstr == NULL) perror(URED "fail:" reset " bad search target\n");

	// Number of records we need to store
	int num_records = (PAGESIZE / (reclen+1)+2); //TODO: +4 due to overflow garbage data
	// Number of records * Length of each record (+1 for \0)
	int arr_size  = (num_records * (reclen+1));
	

	// Setup array
	char** search_arr = calloc(arr_size, sizeof(char*));
	if (search_arr == NULL) perror(URED "fail:" reset " malloc1 failed to allocate\n");
	
	for (int i = 0; i < num_records; i++) // Allocating interior
	{
		search_arr[i] = calloc(reclen+1, sizeof(char));
		if (search_arr[i] == NULL) perror(URED "fail:" reset " malloc2 failed to allocate\n");
	}

	// Number of chars after our found str, to advance ptr.
	int n_chars = 0;
	// Current processing position in array
	int n_pos = 0;
	// Ptr to the current position in the passed input slice
	char* input_ptr = input;
	if (input_ptr == NULL) perror(URED "fail:" reset " bad input\n");

	// Scan through our input page and shove it into the array
	char buffer[reclen+1];
	while (sscanf(input_ptr, "%s%n", buffer, &n_chars) == 1)
	{
		if (n_pos >= num_records) break;
		//printf("Copying %s to %d.\n", buffer, n_pos);
		if (strncpy(search_arr[n_pos], buffer, reclen+1) == NULL) {
			perror(URED "fail:" reset "strncpy failed to copy\n");
		}
		n_pos++;
		input_ptr += n_chars;
	}
	
	int low = 0;
	int high = num_records-1;

	// Repeat until the pointers low and high meet each other
	while (low <= high) {
		(*searchnum)++;
		int mid = low + (high - low) / 2;

		//printf("Testing %s\n", search_arr[mid]);

		if (!strcmp(search_arr[mid], searchstr)) {
			return mid+1; //pos, not idx
		}

		if (strcmp(search_arr[mid], searchstr) < 0) {
			high = mid - 1;
		}
		else {
			low = mid + 1;
		}
	}

	cleanup(search_arr, num_records);
	return CODE_NOTFOUND;
}


void cleanup(char** arr, int num_records) {
	for (int i = 0; i < num_records; i++)
	{
		free(arr[i]);
	}
	free(arr);
}