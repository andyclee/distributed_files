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

int run_master() {
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

  int endSession = 0;
  while(endSession == 0){
    printf("Waiting for connection...\n");
    int client_fd = accept(sock_fd, NULL, NULL);
    printf("Connection made: client_fd=%d\n", client_fd);

    char head[sizeof(header)];
    int len = read(client_fd, head, sizeof(header));
    header x;
    x = *(header*)head;

    //if cmd is c, close Connection
    if (x.cmd=='c') {
      endSession = 1;
      break;
    }

    int flag = 0; //0 for upload, 1 for download
    if (x.cmd=='u') {
      flag = 0;
    } else if (x.cmd=='d') {
      flag = 1;
    } else {
      printf("This is the command: %c\n",x.cmd);
      printf("Command not recognizable.\n");
      return 2;
    }

    int stat = 0;
    if (flag==0) {
      char buffer[x.filesize+1];
      if (x.filesize>130000){
        int len = 0;
        while (len<x.filesize){
          len += read(client_fd, buffer+len, 130000);
        }
      }else{
        int len = read(client_fd, buffer, x.filesize);
      }

      buffer[x.filesize] = '\0';
      stat=upload_f(x.filename, buffer);
    }else if (flag == 1){
      stat=download_f(client_fd, x.filename);
    }



    if (stat==0){
      printf("successful command on file %s\n",x.filename);
    }else {
      printf("Unable to process file %s\n",x.filename);
    }

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
