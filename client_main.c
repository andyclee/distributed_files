#include "client.h"


/**
 * fliename must be within 30 chars
 **/
char* read_f(const char* filename) {
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
  char* buffer = malloc(sizeof(header)+len+1);
  memset(buffer, '\0', sizeof(header)+len+1);
  fseek(fd,0,SEEK_SET);
  fread(buffer+sizeof(header),1,len, fd);
  buffer[sizeof(header)+len] = '\0';
  fclose(fd);

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
  if ((strcmp(argv[1],"-upload")!=0 && strcmp(argv[1],"-download")!=0) || argc > 4){
    printf("Usage: ./client <command name(-upload/-download)> <port> <filename>\n");
    return 2;
  }

  // get the filename from argument
  const char* filename = argv[3];

  // determine is upload or download command
  int flag = strcmp(argv[1],"-upload")==0?0:1; //0 for upload, 1 for download

  // get the test server_port and server_name
	const char* server_port = argv[2];
  const char* server_name = "localhost";

  if (flag==0) {
    // if upload
    // read the file
    char* buffer = read_f(filename);
    if (buffer == NULL) {
      printf("fail to process file %s.\n",filename);
      return -1;
    }
    network_send(buffer, filename, server_port, server_name);
    free(buffer);
  } else {
    // if download
    char* buffer2 = network_receive(filename, server_port, server_name);
    int stat = receive_f(buffer2, filename);
    // print result
    if (stat<0){
      printf("fail to process file %s.\n",filename);
    } else {
      printf("successfully process file %s.\n",filename);
    }
  }

  return 0;
}
