#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include "compression.h"
#include "encryption.h"

static char* file_dir = "slave_files/";

typedef struct header_t{
  // u for upload, d for download
  char cmd;
  // size of file
  uint32_t filesize;
  // filename must be less than 30 chars
  char filename[31];
} header;

int upload_f(const char *filename, char* buffer, uint32_t filesize);
int download_f (int sock, const char* filename);
int server_main(const char* port);

void get_path(const char* filename, char* fn_buff) {
	strcpy(fn_buff, file_dir);
	strcat(fn_buff, filename);
}

int main(int argc, char **argv)
{
  if (argc<2) {
    printf("USAGE: ./slave <port>");
  }
  const char* port = argv[1];
  server_main(port);
  return 0;
}

int upload_f(const char *filename, char* buffer, uint32_t filesize){
  FILE* fd;
  fd = fopen(filename, "w+");
  if (fd==NULL){
	  fprintf(stderr, "upload_f had NULL FILE\n");
    return -1;
  }else{
    fprintf(stderr, "upload file %s\n", filename);
    //char* decompress_buf = decompress(buffer, filesize);
    //fprintf(stderr, "size of decompress_buf:%zu\n", strlen(decompress_buf));
    write(fileno(fd), buffer, filesize);
    fclose(fd);
  }
  return 0;
}

int download_f (int sock, const char* filename) {
  char* mesg;

  char fn_buff[strlen(file_dir) + strlen(filename) + 1];
  get_path(filename, fn_buff);
  FILE* fd;
  fd = fopen(fn_buff,"r");
  if (fd == NULL){
    perror("File does not exist\n");
    return -1;
  }
    fseek(fd,0,SEEK_END);
    size_t len = ftell(fd);

    char buffer[sizeof(header)+len+1];
    memset(buffer, '\0',sizeof(header)+len+1);

    fseek(fd,0,SEEK_SET);
    fread(buffer + sizeof(header),1,len, fd);

    buffer[sizeof(header)+len] = '\0';
    fclose(fd);

  //fflush();
  //set header!
  header x;
  x.filesize = len;
  x.cmd = 'd';
  strncpy(x.filename, filename, strlen(filename));
  x.filename[strlen(filename)] = '\0';
  memcpy(buffer, &x, sizeof(header));

  mesg = buffer;

  // send

  // data that will be sent to the server
  const char* data_to_send = mesg;
  fprintf(stderr, "download file %s\n", filename);
  int sentlen = send(sock, data_to_send, sizeof(header)+ x.filesize, 0);
  if (sentlen!=(int)(sizeof(header)+ x.filesize)){
    perror("Sent not successful\n");
    return -1;
  }
  return 0;
}

int server_main(const char* port) {
  DIR* dir = opendir(file_dir);
  if (dir)
    closedir(dir);
  else if (errno == ENOENT) {
    fprintf(stderr, "Error: File directory does not exist, try running make again\n");
    exit(1);
  }
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

  int endSession = 0;
  while(endSession == 0){
    printf("Waiting for connection...\n");
    int client_fd = accept(sock_fd, NULL, NULL);
    printf("Connection made: client_fd=%d\n", client_fd);

    char head[sizeof(header)];
    read(client_fd, head, sizeof(header));
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

    char fn_buff[strlen(file_dir) + 31 + 1];
    get_path(x.filename, fn_buff);
    fprintf(stderr, "Full filepath is: %s\n", fn_buff);
    int stat = 0;
    if (flag==0) {
      char buffer[x.filesize];
      if (x.filesize>130000){
        int len = 0;
        while (len<(int)x.filesize){
          len += read(client_fd, buffer+len, 130000);
        }
      }else{
          read(client_fd, buffer, x.filesize);
      }
      printf("Upload %d size\n", x.filesize);
      stat=upload_f(fn_buff, buffer, x.filesize);
    }else if (flag == 1){
      printf("Download %d size\n", x.filesize);
      stat=download_f(client_fd, x.filename);
    }



    if (stat==0){
      printf("successful command on file %s\n",x.filename);
    }else {
      printf("Unable to process file %s\n",x.filename);
    }

  }

  freeaddrinfo(result);
  close(sock_fd);
  return 0;
}
