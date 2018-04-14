/*
 * Handles the client side application
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define WRITE_BUFF_SIZE 4096
#define SERVER_PORT "8000"
#define SERVER_NAME "localhost"

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
		FILE* push_file = fopen(filename, "r");
		fseek(push_file, 0, SEEK_END);
		size_t total_bytes = ftell(push_file);
		rewind(push_file);
		size_t bytes_sent = 0;
		while (bytes_sent < push_file) {
			char write_buff[WRITE_BUFF_SIZE];
			size_t read_size = fread(write_buff, 1, WRITE_BUFF_SIZE, filename);
			int sent_batch_size = network_send(write_buff, read_size, SERVER_PORT, SERVER_NAME, read_size);
			bytes_sent = bytes_sent + sent_batch_size;
			fseek(local, sent_batch_size, SEEK_SET);
		}
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
