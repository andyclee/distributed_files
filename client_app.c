/*
 * Handles the client side application
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_usage() {
	fprintf(stdout, "USAGE: ./dist_file <COMMAND> <FILENAME>\nValid <COMMAND>s: push, pull, remove\n");
}

int main(int argc, char** argv) {
	if (argc < 3) {
		print_usage();
		return 1;
	}
	char* command = argv[1];
	char* filename = argv[2];

	if (!strcmp(command, "push")) {
		fprintf(stdout, "Pushing file: %s\n", filename);
	}
	else if (!strcmp(command, "pull")) {
		fprintf(stdout, "Pulling file: %s\n", filename);
	}
	else if (!strcmp(command, "remove")) {
		fprintf(stdout, "Removing file: %s\n", filename);
	}
	else {
		fprintf(stdout, "Invalid command!\n");
		print_usage();
		return 1;
	}
	return 0;
}
