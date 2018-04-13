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

int launch_fs(bool debug) {
	pid_t fork_stat = fork();
	if (fork_stat == -1) {
		exit(1);
	}
	else if (fork_stat == 0) {
		if (debug)
			system("./dffs -d -o auto_unmount ddfs/rootdir ddfs/mountdir");
		else
			system("./dffs -d -o auto_unmount ddfs/rootdir ddfs/mountdir");
		exit(2);
	}
	else {
		return waitpid(fork_stat, NULL, 0);
	}
}

void print_invalid_option() {
	fprintf(stderr, "Invalid option!\n");
}

void test_fs() {
	char* test_fp = "test_file.txt";
}

int main(int argc, char** argv) {
	bool debug = false;
	if (argc < 1) {
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
	//TODO: Remove test code
	test_fs();
}
