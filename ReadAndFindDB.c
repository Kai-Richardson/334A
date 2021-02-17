
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

int main(const int argc, const char * argv []) {
	int REC_LEN = 10;
	int RECORDS = 0;
	char buff[REC_LEN];
	int len = 0;
	srand((unsigned)time(NULL));
	struct stat struct_stat;

	if (argc != 2) {
		printf("Usage: %s <INPUT) FILE NAME>\n", argv[0]);
		exit(EXIT_FAILURE); 
	}

	const char * filename = argv[1];
	int fd_in = open(filename, O_RDONLY);
 	int errnum = errno;	
 	if(errnum == -1){
		printf("Could not open file: %s \n", argv[1]);
		exit(EXIT_FAILURE);
	}

	if (stat(argv[1], &struct_stat) == -1) {
		perror("Could not run sys call: stat\n");
	} else {
    	printf("File is: %lld bytes long\n",
           (long long) struct_stat.st_size);
    }
    RECORDS = (int) (struct_stat.st_size / REC_LEN);
	for(int i=0; i<5; i++){
		int record = rand() % RECORDS;
		lseek(fd_in, (record * REC_LEN), SEEK_SET);
		int rc = read(fd_in, buff, REC_LEN);
		printf ("Record: %d rc: %d \n", record, rc);
		if (rc < 0 ){
			perror("Could not read from file\n");
			exit(EXIT_FAILURE);
		}				
		printf("Index is %d, and name is: %.*sX\n", record,REC_LEN, buff);
	}

	close(fd_in);

	return 1;
}	
