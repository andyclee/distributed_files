#include "client.h"


/**
 * fliename must be within 30 chars
 **/
char* read_f(const char* filename, int* flen) {
  if (strlen(filename)>30){
    return NULL;
  }
  //get string from file and send if upload
  //receive string and convert to file if download
  char* mesg;
  FILE* fd;
  fd = fopen(filename,"r");
  if (fd == NULL){
    return NULL;
  }

  fseek(fd,0,SEEK_END);
  size_t len = ftell(fd);
  char* buffer = malloc(len+1);
  memset(buffer, '\0', len+1);
  fseek(fd,0,SEEK_SET);
  fread(buffer,1,len, fd);
  buffer[len] = '\0';
  fclose(fd);

  *flen = len;
  return buffer;
}

/**
 * Frees the buffer.
 **/
int receive_f(char* buffer2, const char* filename){

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

/**
 * Test Main function for client.
 **/
int main(int argc, char **argv) {

  // print usage if argv is not in correct format
  if ((strcmp(argv[1],"-upload")!=0 && strcmp(argv[1],"-download")!=0 && strcmp(argv[1],"-close")!=0) || argc > 4){
    printf("Usage: ./client <command name(-upload/-download)> <port> <filename>\nor ./client -close <port> close\n");
    return 2;
  }

  // get the filename from argument
  const char* filename = argv[3];

  // determine is upload or download command
  int flag = strcmp(argv[1],"-upload")==0?0:1; //0 for upload, 1 for download

  // get the test server_port and server_name
	const char* server_port = argv[2];
  const char* server_name = "localhost";

  if (strcmp(argv[1],"-close")==0) {
    int retc = network_close(server_port, server_name);
    return retc;
  }

  if (flag==0) {
    // if upload
    // read the file
    int flen;
    char* buffer = read_f(filename, &flen);
    if (buffer == NULL) {
      printf("fail to process file %s.\n",filename);
      return -1;
    }
    network_send(buffer, filename, server_port, server_name, flen);
    free(buffer);
  } else {
    // if download
    int fsize = 0;
    char* buffer2 = network_receive(filename, server_port, server_name,&fsize);
    int stat = receive_f(buffer2, filename);
    // print result
    if (stat<0){
      printf("fail to receive file %s.\n",filename);
    } else {
      printf("successfully received file %s of size %d.\n", filename, fsize);
    }
  }

  return 0;
}
