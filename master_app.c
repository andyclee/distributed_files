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
	size_t bytes_written = write(write_file, test_buff, file_size);
	fprintf(stderr, "Wrote %zu bytes\n", bytes_written);
	free(full_path);
}

int main(int argc, char** argv) {
	bool debug = false;
	if (argc > 1) {
		if (!strcmp(argv[1], "debug"))
			debug = true;
		else {
			print_invalid_option();
			return 1;
		}
	}
	int fs_stat = launch_fs(debug);
	if (fs_stat == -1) {
		fprintf(stderr, "Unable to launch file system, exiting\n");
		return 1;
	}

	//Network connecting in infinite loop goes here
	if (debug) {
		sleep(1);
		test_fs();
	}

	//TODO: ACCEPT CLIENTS
	

	return 0;
}
