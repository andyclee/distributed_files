#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>

typedef struct header_t{
  // u for upload, d for download
  char cmd;
  // size of file
  uint32_t filesize;
  // filename must be less than 30 chars
  char filename[31];
} header;

int upload_f(const char *filename, const char* buffer){
  FILE* fd;
  fd = fopen(filename, "w");
  if (fd==NULL){
    return -1;
  }else{
    fputs(buffer,fd);
    fclose(fd);
  }
  return 0;
}

int download_f (int sock, const char* filename) {
  char* mesg;

  FILE* fd;
  fd = fopen(filename,"r");
  if (fd == NULL){
    return -1;
  }
    fseek(fd,0,SEEK_END);
    size_t len = ftell(fd);

    char buffer[len+1];
    memset(buffer, '\0', len+1);

    fseek(fd,0,SEEK_SET);
    fread(buffer+sizeof(header),1,len, fd);
    buffer[sizeof(header)+len] = '\0';
    fclose(fd);

  //fflush();
  //set header!
  header x;
  x.filesize = strlen(buffer+sizeof(header));
  x.cmd = 'd';
  strncpy(x.filename, filename, strlen(filename));
  x.filename[strlen(filename)] = '\0';
  memcpy(buffer, &x, sizeof(header));

  mesg = buffer;
  printf("%s\n",mesg);

  // send

  // data that will be sent to the server
  const char* data_to_send = mesg;
  send(sock, data_to_send, sizeof(header) + x.filesize, 0);
  return 0;
}

int main(int argc, char **argv)
{
    int s;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,(const char*)&reuse, sizeof(reuse));

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    s = getaddrinfo(NULL, "1234", &hints, &result);
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

    printf("Waiting for connection...\n");
    int client_fd = accept(sock_fd, NULL, NULL);
    printf("Connection made: client_fd=%d\n", client_fd);

    char head[sizeof(header)];
    int len = read(client_fd, head, sizeof(header));
    header x;
    x = *(header*)head;

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
      len = read(client_fd,buffer, x.filesize);
      buffer[x.filesize] = '\0';
      stat=upload_f(x.filename, buffer);
    }else if (flag == 1){
      stat=download_f(sock_fd, x.filename);
    }



    if (stat==0){
      printf("successful command on file %s\n",x.filename);
    }else {
      printf("Unable to process file %s\n",x.filename);
    }

    close(sock_fd);

    return 0;
}
