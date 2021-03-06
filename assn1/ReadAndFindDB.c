
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "stdcolor.h"
#include "reader.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define CODE_NOTFOUND 0
#define MAXIMUM_INPUT_LENGTH 20

// Handles the main reading logic
int main(const int argc, const char * argv []) {
	if (argc != 4) {
		printf(RED "fail:" reset " " UWHT "Usage:" reset " %s <Input File> <Search Text> <Mode>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/// The text we are to search for
	const char* search_text = argv[2];
	/// Our passed search mode
	const int mode = atoi(argv[3]);

	/// Our file we are buffering and then searching
	const char * filename = argv[1];
	int fd_in = open(filename, O_RDONLY|O_CREAT, 0777);
 	int errnum = errno;
 	if(errnum == -1){
		printf(URED "fail:" reset " Could not open file: %s \n", argv[1]);
		exit(EXIT_FAILURE);
	}

	/// Maximum line length determined from input file
	int REC_LEN = 0;

	char length_call[MAXIMUM_INPUT_LENGTH+7] = "wc -L <";
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

	/// Number of records found
	int rec_num = 0;

	/// Stat struct to get bytesize
	struct stat struct_stat;
	if (stat(argv[1], &struct_stat) == -1) {
		perror(URED "fail:" reset " Could not run sys call: stat\n");
	}
    rec_num = (int) (struct_stat.st_size / REC_LEN);
	printf("Searching " BWHT "%i" reset " records with mode " YEL "%d" reset " in file " BWHT "%s" reset  "...\n", rec_num, mode, argv[1]);

	/// System page size for our buffer
	int PAGESIZE = getpagesize();
	/// Read(2) into this incrementally and work on contained data
	char read_buffer[PAGESIZE];

	/// Number of records we searched through for later display
	int read_num = 0;

	/// Number of pages we went through
	int pages = 0;

	/// Return code from our search function (no default return values in C 😢)
	int return_code = CODE_NOTFOUND;

	// Our main loop to go through different pages and search
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
		if (return_code != CODE_NOTFOUND) break; //We encountered an error or found something
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

/*
 * In Advance note:
 * Sorry that I didn't split up the array allocation and setup from each search.
 * There's massive duplication due to this.
 * I tried, but it ended up wasting a lot of my time trying to debug errors that arose.
 */


/**
 * Searches sequentially through a chunked char array input for a given string.
 *
 * Return:	* negative int error code if failure encountered
 * 					* CODE_NOTFOUND if not found
 * 					* positive int of position if found
 */
int SequentialSearch(char* input, const char* searchstr, int* searchnum, int reclen, int PAGESIZE) {

	if (input == NULL) perror(URED "fail:" reset " bad input\n");
	if (searchnum == NULL || searchstr == NULL) perror(URED "fail:" reset " bad search target\n");

	// Number of records we need to store/calloc
	int max_num_records = (PAGESIZE / (reclen+1)); //+1 for safety
	// Number of records * Length of each record (+1 for \0)
	int arr_size  = (max_num_records * (reclen+1));


	// Setup array
	char** search_arr = calloc(arr_size, sizeof(char*));
	if (search_arr == NULL) perror(URED "fail:" reset " malloc1 failed to allocate\n");

	for (int i = 0; i < max_num_records; i++) // Allocating interior
	{
		search_arr[i] = calloc(reclen + 1, sizeof(char));
		if (search_arr[i] == NULL) perror(URED "fail:" reset " malloc2 failed to allocate\n");
	}

	/// Number of chars in found str, to advance ptr.
	int n_chars = 0;
	/// Current processing position in array, also number of actual records we're processing
	int n_pos = 0;
	/// Ptr to the current position in the passed input slice
	char* input_ptr = input;

	// Scan through our input page and shove it into the array
	char buffer[reclen+1];
	while (sscanf(input_ptr, "%s%n", buffer, &n_chars) == 1)
	{
		if (n_pos >= max_num_records) break; // Break if we're about to go off the arr
		if (strncpy(search_arr[n_pos], buffer, reclen+1) == NULL) {
			perror(URED "fail:" reset "strncpy failed to copy\n");
		}
		n_pos++;
		input_ptr += n_chars;
	}

	// The actual searching part
	for(int i = 0; i < n_pos; ++i)
	{
		(*searchnum)++;
		if(!strcmp(search_arr[i], searchstr))
		{
			// We found it! Cleaning up and returning position.
			cleanup(search_arr, max_num_records);
			return i+1; //pos, not idx
		}
	}

	cleanup(search_arr, max_num_records);
	return CODE_NOTFOUND;
}

/**
 * Searches via binary search through a chunked char array input for a given string.
 *
 * Return:	* negative int error code if failure encountered
 * 					* CODE_NOTFOUND if not found
 * 					* positive int of position if found
 */
int InterpolationSearch(char* input, const char* searchstr, int* searchnum, int reclen, int PAGESIZE) {

	if (input == NULL) perror(URED "fail:" reset " bad input\n");
	if (searchnum == NULL || searchstr == NULL) perror(URED "fail:" reset " bad search target\n");

	// Number of records we need to store/check
	int max_num_records = (PAGESIZE / (reclen+1)+1); //+1 for safety
	// Number of records * Length of each record (+1 for \0)
	int arr_size  = (max_num_records * (reclen+1));


	// Setup array
	char** search_arr = calloc(arr_size, sizeof(char*));
	if (search_arr == NULL) perror(URED "fail:" reset " malloc1 failed to allocate\n");

	for (int i = 0; i < max_num_records; i++) // Allocating interior
	{
		search_arr[i] = calloc(reclen + 1, sizeof(char));
		if (search_arr[i] == NULL) perror(URED "fail:" reset " malloc2 failed to allocate\n");
	}

	/// Number of chars in found str, to advance ptr.
	int n_chars = 0;
	/// Current processing position in array, also number of actual records we're processing
	int n_pos = 0;
	/// Ptr to the current position in the passed input slice
	char* input_ptr = input;
	if (input_ptr == NULL) perror(URED "fail:" reset " bad input\n");

	// Scan through our input page and shove it into the array
	char buffer[reclen+1];
	while (sscanf(input_ptr, "%s%n", buffer, &n_chars) == 1)
	{
		if (n_pos >= max_num_records) break; // Break if we're about to go off the arr
		if (strncpy(search_arr[n_pos], buffer, reclen+1) == NULL) {
			perror(URED "fail:" reset "strncpy failed to copy\n");
		}
		n_pos++;
		input_ptr += n_chars;
	}

	int low = 0;
	int high = n_pos-1;

	// Repeat until the pointers low and high meet each other
	while (low <= high) {
		(*searchnum)++;
		int mid = low + (high - low) / 2;

		int compare = strcmp(search_arr[mid], searchstr);

		if (!compare) { //Found it!
			return mid+1; //pos, not idx
		}
		else if (compare > 0) {
			high = mid - 1;
		}
		else {
			low = mid + 1;
		}
	}

	cleanup(search_arr, max_num_records);
	return CODE_NOTFOUND;
}

/// array cleanup func
void cleanup(char** arr, int num_records) {
	for (int i = 0; i < num_records; i++)
	{
		free(arr[i]);
	}
	free(arr);
}