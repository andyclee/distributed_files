/*
 * Handles the client side application
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "client.h"
#include "compression.h"

#define WRITE_BUFF_SIZE 4096
#define SERVER_PORT "8000"
#define SERVER_NAME "localhost"

void print_usage() {
	fprintf(stdout, "USAGE: ./dist_file <COMMAND> <FILENAME>\nValid <COMMAND>s: upload, download, list\n");
}

int main(int argc, char** argv) {
	if (argc < 2 || argc > 3) {
		print_usage();
		return 1;
	}
	if(argc == 2 && strcmp(argv[1], "list") != 0) {
		print_usage();
		return 1;
	}
	char* command = argv[1];
	char* filename = argv[2];
	
	if (!strcmp(command, "upload")) {
		fprintf(stdout, "Uploading file %s...\n", filename);
		FILE* push_file = fopen(filename, "r");
		if(push_file == NULL) {
			fprintf(stdout, "Failed to find file %s\n", filename);
			print_usage();
			return 1;
		}

		fseek(push_file, 0, SEEK_END);
		size_t total_bytes = ftell(push_file);
		char* buffer = malloc(total_bytes);
		
		fseek(push_file, 0, SEEK_SET);
		fread(buffer, 1, total_bytes, push_file);

		fprintf(stderr, "Total_bytes: %zu\n", total_bytes);
		
		int compress_size = 0;
		char* compress_buf = compress(buffer, &compress_size);
		int result = network_send(compress_buf, filename, SERVER_PORT, SERVER_NAME, compress_size);
		free(buffer);
		free(compress_buf);
		if(result < 0) {
		    	fprintf(stdout, "Failed to upload file %s, please try again.\n", filename);
			return 1;
		} else {
		    	fprintf(stdout, "Successfully upload file %s\n", filename);
		}	
		fclose(push_file);	
	}
	else if (!strcmp(command, "download")) {
		fprintf(stdout, "Downloading file %s...\n", filename);
		size_t file_size = 0;
		char* buffer = network_receive(filename, SERVER_PORT, SERVER_NAME, &file_size);
		if(buffer == NULL) {
			fprintf(stdout, "Failed to download file %s, please try again.\n", filename);
			return 1;
		} 
		char* decompress_buf = decompress(buffer, (int)file_size);
		FILE* pull_file = fopen(filename, "w");
		if(pull_file == NULL) {
			fprintf(stdout, "Succeed in downloading, failed to write to file %s\n", filename);
			free(buffer);
			free(decompress_buf);
			return 1;
		} else {
			write(fileno(pull_file), decompress_buf, file_size);
			fclose(pull_file);
			free(buffer);
			free(decompress_buf);
			fprintf(stdout, "Successfully download file %s\n", filename);
		}
	}
	else if (!strcmp(command, "list")) {
		char* buffer = malloc(WRITE_BUFF_SIZE);
		int buf_size = network_list_request(SERVER_PORT, SERVER_NAME, buffer);
		if(buf_size < 0) {
			fprintf(stdout, "Failed to get list, please try again.\n");
			return 1;
		} 
		buffer[buf_size] = '\0';
		fprintf(stdout, "List of file:\n");
		fprintf(stdout, "%s", buffer);
		free(buffer);
	}
	else {
		fprintf(stdout, "Invalid command!\n");
		print_usage();
		return 1;
	}
	return 0;
}
