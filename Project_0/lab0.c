/*
NAME: Omar Tleimat
EMAIL: otleimat@ucla.edu
ID: 404844706
*/
#include <getopt.h> 
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>



void redirect(char* infile, char* outfile);
void handle_IO();
void signal_handler();
void cause_seg_fault(){
	int* dontTryMe = NULL;
	// Dereference the null pointer
	*dontTryMe = 0;
}

int main(int argc, char** argv){

	static struct option options[] =
  	{
		{"input",	 required_argument, 0, 'i'},
		{"output", 	 required_argument, 0, 'o'},
		{"segfault", no_argument, 		0, 's'},
		{"catch", 	 no_argument, 		0, 'c'}
	};

	int opt = -1;
	int shouldFault = 0;
	char* infile = NULL;
	char* outfile = NULL;
	// Loop through CLI options
	// Required arguments have colons after them
	while((opt = getopt_long(argc, argv, "i:o:sc", options, NULL)) != -1)
	{
		switch (opt) {
			case 'i':
				infile = optarg;
				break;
			case 'o':
				outfile = optarg;
				break;
			case 's':
				shouldFault = 1;
				break;
			case 'c':
				signal(SIGSEGV, signal_handler);
				break;
			// If an unknown argument was passed through, report usage message
			default: 
				printf("usage: lab0 -i file1 -o file2 -s -c\n");
				exit(1);
		}
	}
	redirect(infile, outfile);
	// For the purpose of this assignment
	// nest the segmentation fault to get a 
	// better backtrace
	if (shouldFault){
		cause_seg_fault();
	}
	handle_IO();
	// if it reached this point, execution was succesful 
	exit(0);
}

void redirect(char* infile, char* outfile){
	int ifd = 0;
	int err;
	if(infile) {
		ifd = open(infile, O_RDONLY);
		if (ifd >= 0){
			close(0);
			dup(ifd);
			close(ifd);
		}
		else {
			err = errno;
			fprintf(stderr, "Problem with input file \"%s\": %s\n", infile, strerror(err));
			exit(2);
		}
	}
	int ofd = 1;
	if(outfile) {
		ofd = creat(outfile, S_IRWXU);
		if (ofd >= 0){
			close(1);
			dup(ofd);
			close(ofd);
		}
		else {
			err = errno;
			fprintf(stderr, "Problem with output file \"%s\": %s\n", outfile, strerror(err));
			exit(3);

		}
	}
}

void signal_handler(int signal){
	if (signal == SIGSEGV){
		fprintf(stderr, "Segmentation fault occured: %s\n", strerror(4));
		exit(4);
	}
}

void handle_IO(){
	// Use 0 and 1 even if a redirection occured 
	int ifd = 0, ofd = 1;
	char* buffer;
	buffer = (char*) malloc(sizeof(char));
	int stat;
	int err;
	// Error check while doing read and write
	while(1){
		// Read and write byte by byte
		stat = read(ifd, buffer, 1);
		if (stat < 0){
			err = errno;
			fprintf(stderr, "Error while reading.. %s\n", strerror(err));
			exit(2);
		}
		// If 0 bytes read, exit loop
		if (stat == 0){
			break;
		}
		// read and write byte by byte 
		stat = write(ofd, buffer, 1);
		if (stat < 0){
			err = errno;
			fprintf(stderr, "Error while writing.. %s\n", strerror(err));
			exit(3);
		}
	}
	// free up the space allocated
	free(buffer);
	buffer = NULL;
}