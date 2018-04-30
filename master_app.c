/*
 * Application that runs on master server
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include "client.h"

static int endSession;

char* create_full_path(char* path) {
	char* prepend = "./ddfs/mountdir/";
	char* full_path = malloc(strlen(path) + strlen(prepend) + 1);
	strcpy(full_path, prepend);
	strcat(full_path, path);
	return full_path;
}

int launch_fs(bool debug) {
	pid_t fork_stat = fork();
	if (fork_stat == -1) {
		exit(1);
	}
	else if (fork_stat == 0) {
		if (debug)
			system("./dffs -d -o auto_unmount ddfs/rootdir ddfs/mountdir");
		else
			system("./dffs -o auto_unmount ddfs/rootdir ddfs/mountdir");
		exit(2);
	}
	else {
		return fork_stat;
	}
}

void print_invalid_option() {
	fprintf(stderr, "Invalid option!\n");
}

void test_fs() {
	char* test_fp = "test_file.txt";
	FILE* test_file = fopen(test_fp, "r");
	fseek(test_file, 0, SEEK_END);
	size_t file_size = ftell(test_file);
	rewind(test_file);
	char test_buff[file_size + 1];
	fread(test_buff, 1, file_size, test_file);

	test_buff[file_size] = '\0';
	char* full_path = create_full_path(test_fp);

	fprintf(stderr, "Opening file\n");
	int write_file = open(full_path, O_WRONLY | O_CREAT);
	if (write_file == -1) {
		fprintf(stderr, "errno: %d\n", errno);
		return;
	}
	fprintf(stderr, "Writing to file\n");
	ssize_t bytes_written = write(write_file, test_buff, file_size);
	fprintf(stderr, "Wrote %ld bytes\n", bytes_written);
	if (bytes_written == -1) {
		fprintf(stderr, "Error in write, exiting\n");
		exit(1);
	}
	close(write_file);

	int read_file = open(full_path, O_RDONLY);
	char* file_buf = malloc(0);
	int read_stat = read(read_file, file_buf, 0);
	if (read_stat == -1) {
		fprintf(stderr, "Error in read, exiting\n");
		exit(1);
	}
	fprintf(stderr, "%s", file_buf);
	free(file_buf);
	close(read_file);

	free(full_path);
}

void shutdown_server(int sock){
	shutdown(sock, SHUT_RDWR);
	close(sock);
}

void send_error(int client_fd){
	write_to_socket(client_fd, "ERROR", 5);
}

int run_master (const char* port) {
	// start a server
	int s;
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	int reuse = 1;
	setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,(const char*)&reuse, sizeof(reuse));

	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	s = getaddrinfo(NULL, port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(1);
	}

	if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
		perror("bind()");
		exit(1);
	}

	if (listen(sock_fd, 10) != 0) {
		perror("listen()");
		exit(1);
	}

	struct sockaddr_in *result_addr = (struct sockaddr_in *) result->ai_addr;
	printf("Listening on file descriptor %d, port %d\n", sock_fd, ntohs(result_addr->sin_port));

	freeaddrinfo(result);

	endSession = 0;
	while(endSession == 0){

		printf("Waiting for connection...\n");
		int client_fd = accept(sock_fd, NULL, NULL);
		printf("Connection made: client_fd=%d\n", client_fd);

		char head[sizeof(header)];
		int len = read(client_fd, head, sizeof(header));
		header x;
		x = *(header*)head;
		char* filename = x.filename;
		char* full_path = create_full_path(filename);
		uint32_t file_size = x.filesize;

		// if anything went wrong use send_error(int client_fd) function
		// shutdown connection with client after done
		if (x.cmd=='u') { // client requires upload, write to slave
			fprintf(stderr, "Handling upload\n");
			int write_file = open(full_path, O_WRONLY | O_CREAT);
			if (write_file == -1) {
				fprintf(stderr, "errno: %d\n", errno);
				send_error(client_fd);
				close(client_fd);
				free(full_path);
				continue;
			}
			fprintf(stderr, "Writing to file\n");
			char file_buff[file_size];
			read_from_socket(client_fd, file_buff, file_size);
			ssize_t bytes_written = write(write_file, file_buff, file_size);
			if (bytes_written < 0) {
				send_error(client_fd);
				close(client_fd);
				free(full_path);
				continue;
			}
			fprintf(stderr, "Wrote %ld bytes\n", bytes_written);
		}
		else if (x.cmd=='d') { // client requires download, read from slave
			fprintf(stderr, "Handling download\n");
			int read_file = open(full_path, O_RDONLY);
			char* file_buf = malloc(0);
			int read_stat = read(read_file, file_buf, 0);
			if (read_stat < 0) {
				send_error(client_fd);
				close(client_fd);
				free(full_path);
				free(file_buf);
				continue;
			}
			header d_header;
			d_header.filesize = read_stat;
			d_header.cmd = 'd';
			d_header.filename[strlen(filename)] = '\0';
			memcpy(d_header.filename, filename, strlen(filename));
			write_to_socket(client_fd, (char*)&d_header, sizeof(d_header));
			write_to_socket(client_fd, file_buf, read_stat);
			free(file_buf);
		}
		else if (x.cmd == 'l'){ // client requires list, readdir from slave


		}
		else{
			// unknown command

			printf("This is the command: %c\n",x.cmd);
			printf("Command not recognizable.\n");
		}

		free(full_path);

		// need to handle signal interrupt

		// need to close up slaves and shutdown this server
	}

	return 0;
}

int main(int argc, char** argv) {
	bool debug = false;
	bool test = false;
	if (argc > 1) {
		if (!strcmp(argv[1], "debug"))
			debug = true;
		else if (!strcmp(argv[1], "test"))
			test = true;
		else {
			print_invalid_option();
			return 1;
		}
	}
	int fs_stat = launch_fs(debug || test);
	if (fs_stat == -1) {
		fprintf(stderr, "Unable to launch file system, exiting\n");
		return 1;
	}
	sleep(1);

	if (test) {
		test_fs();
	}

	//Network connecting in infinite loop goes here
	//TODO: ACCEPT CLIENTS
        const char* port = "8000";
	run_master(port);

	return 0;
}
