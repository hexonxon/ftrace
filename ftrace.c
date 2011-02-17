#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "task.h"


static void usage() 
{
	printf(	"ftrace - Trace process file i/o:\n"
		"usage: ftrace command [args...]\n"
		" command [args...]	- run command with optional arguments.\n"
	);
}


int main(int argc, char** argv) 
{
	if(argc < 2) {
		usage();
		return EXIT_FAILURE;
	}

	const char* cmd = argv[1];
	char** args = &argv[1];

	return ftrace_create(cmd, args);
}


