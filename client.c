#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct header_t{
  // u for upload, d for download
  char cmd;
  // size of file
  uint32_t filesize;
  // filename must be less than 30 chars
  char filename[31];
} header;
/**
 * Encrypt the file
 **/
char* encrypt_f(char* str){
  return str;
}

/**
 * Decrypt the file
 **/
char* decrypt_f(char* str){
  return str;
}

/**
 * Send a string
**/
int send_s(int sock, char* buffer, const char* filename) {
  buffer = encrypt_f(buffer);
  //set header!
  header x;
  x.filesize = strlen(buffer+sizeof(header));
  x.cmd = 'u';
  strncpy(x.filename, filename, strlen(filename));
  x.filename[strlen(filename)] = '\0';
  memcpy(buffer, &x, sizeof(header));

  char* mesg = buffer;

  // send

  // data that will be sent to the server
  const char* data_to_send = mesg;
  send(sock, data_to_send, sizeof(header) + x.filesize, 0);
  return 0;
}

/**
 * fliename must be within 30 chars
 **/
int send_f(int sock, const char* filename) {
  if (strlen(filename)>30){
    return -1;
  }
  //get string from file and send if upload
  //receive string and convert to file if download


  char* mesg;


  FILE* fd;
  fd = fopen(filename,"r");
  if (fd == NULL){
    return -1;
  }
    fseek(fd,0,SEEK_END);
    size_t len = ftell(fd);
    char* buffer = malloc(sizeof(header)+len+1);
    memset(buffer, '\0', sizeof(header)+len+1);
    fseek(fd,0,SEEK_SET);
    fread(buffer+sizeof(header),1,len, fd);
    buffer[sizeof(header)+len] = '\0';
    fclose(fd);

    int ret = send_s(sock, buffer,filename);

    free(buffer);

    return ret;

}

/**
 * Receive a string
**/

char* receive_s(int sock, const char* filename){

    char buffer[sizeof(header)];
    header x;
    x.filesize = 0;
    x.cmd = 'd';
    strncpy(x.filename, filename, strlen(filename));
    x.filename[strlen(filename)] = '\0';

    memcpy(buffer, &x, sizeof(header));

    // send
    // send the header
    const char* data_to_send = (const char*)buffer;
    send(sock, data_to_send, sizeof(header), 0);

    // receive
    //receive the string that contain the file

    char head [sizeof(header)];
    int len = read(sock, head, sizeof(header));

    header r;
    r = *(header*)head;
    if (r.cmd!='d'){
      printf("%c,%d\n",r.cmd,len);
      perror("1\n");
      return NULL;
    }

    char* buffer2 = malloc(r.filesize+1);
    len = read(sock,buffer2, r.filesize);
    buffer2[r.filesize] = '\0';

    buffer2 = decrypt_f(buffer2);
    return buffer2;
}

int receive_f(int sock, const char* filename){

  char* buffer2 = receive_s(sock,filename);
  if (buffer2==NULL) {
    return -1;
  }

 //write the string to filename
  FILE* fd;
  fd = fopen(filename, "w");
  if (fd==NULL){
    perror("2\n");
    free(buffer2);
    return -1;
  }else{
    fputs(buffer2,fd);
    fclose(fd);
    free(buffer2);
    return 0;
  }

}

int main(int argc, char **argv) {

  if ((strcmp(argv[1],"-upload")!=0 && strcmp(argv[1],"-download")!=0) || argc > 4){
    printf("Usage: ./client <command name(-upload/-download)> <port> <filename>\n");
    return 2;
  }

  const char* filename = argv[3];
  int flag = strcmp(argv[1],"-upload")==0?0:1; //0 for upload, 1 for download
  // need change?
	const int server_port = strtol(argv[2], NULL, 10);

  const char* server_name = "localhost";

  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;

  // creates binary representation of server name
  // and stores it as sin_addr
  // http://beej.us/guide/bgnet/output/html/multipage/inet_ntopman.html
  inet_pton(AF_INET, server_name, &server_address.sin_addr);

  // htons: port in network order format
  server_address.sin_port = htons(server_port);

  // open a stream socket
  int sock;
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    printf("could not create socket\n");
    return 1;
  }

  // TCP is connection oriented, a reliable connection
  // **must** be established before any data is exchanged
  if (connect(sock, (struct sockaddr*)&server_address,
              sizeof(server_address)) < 0) {
    printf("could not connect to server\n");
    return 1;
  }

  int stat=0;
  if (flag==0){
    stat = send_f(sock, filename);
  } else if (flag==1) {
    stat = receive_f(sock, filename);
  }

  // print result
  if (stat<0){
    printf("fail to process file %s.\n",filename);
  } else {
    printf("successfully process file %s.\n",filename);
  }
  // close the socket
  close(sock);
	return 0;
}
