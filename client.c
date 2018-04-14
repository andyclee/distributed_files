#include "client.h"



/**
 * Send a string
**/
int send_s(int sock, char* buffer, const char* filename, int filelen) {

  char* temp = malloc(sizeof(header)+filelen+1);
  memset(temp, '\0', sizeof(header)+filelen+1);
  strncpy(temp+sizeof(header), buffer, filelen+1);

  //set header!
  header x;
  x.filesize = filelen;
  x.cmd = 'u';
  strncpy(x.filename, filename, strlen(filename));
  x.filename[strlen(filename)] = '\0';
  memcpy(temp, &x, sizeof(header));

  char* mesg = temp;

  // send

  // data that will be sent to the server
  const char* data_to_send = mesg;
  int bytes_sent = send(sock, data_to_send, sizeof(header), 0);
  bytes_sent = send(sock, data_to_send+sizeof(header), x.filesize, 0);

  if (bytes_sent != x.filesize) {
    // in case sent fails
    perror("send bytes fail");
  }

  free(temp);
  return x.filesize;
}


/**
 * Receive a string
**/

char* receive_s(int sock, const char* filename, int* filelen){

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
    int head_size = send(sock, data_to_send, sizeof(header), 0);

    if (head_size!=sizeof(header)){
          perror("sent wrong\n");
    }

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
    len = 0;
    while (len < r.filesize){
      len += read(sock, buffer2+len, r.filesize);
    }
    buffer2[r.filesize] = '\0';
    *filelen = len;

    return buffer2;
}



/**
 * Interface function of the network layer for sending string
 * Need to indicate the string to send, a filename to store as, a port and a server_name
 **/
int network_send (char* buffer, const char* filename, const char* server_port, const char* server_name, int filelen) {
  // connection
  int s;
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM; //TCP

  s = getaddrinfo(server_name, server_port, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(1);
  }

  // TCP is connection oriented, a reliable connection
  // **must** be established before any data is exchanged
  if (connect(sock, result->ai_addr, result->ai_addrlen) < 0) {
    perror("connect");
    freeaddrinfo(result);
    exit(1);
  }

  int stat=-1;
  stat = send_s(sock, buffer, filename, filelen);

  // print result
  if (stat<0){
    printf("fail to process file %s.\n",filename);
  } else {
    printf("successfully process file %s.\n",filename);
  }

  // close the socket
  close(sock);
  freeaddrinfo(result);
	return stat;
}

/**
 * Interface function of the network layer for receiving string
 * Need to indicate the filename to download, a server_port and a server_name
 **/
char* network_receive(const char* filename, const char* server_port, const char* server_name, int* filelen){
  // connection
  int s;
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM; //TCP

  s = getaddrinfo(server_name, server_port, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(1);
  }

  // TCP is connection oriented, a reliable connection
  // **must** be established before any data is exchanged
  if (connect(sock, result->ai_addr, result->ai_addrlen) < 0) {
    perror("connect");
    freeaddrinfo(result);
    exit(1);
  }

  printf("before receive_s\n");

  char* buffer = receive_s(sock, filename,filelen);

  // close the socket
  close(sock);
  freeaddrinfo(result);
  return buffer;
}

int network_close(const char* server_port, const char* server_name){
  // connection
  int s;
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM; //TCP

  s = getaddrinfo(server_name, server_port, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(1);
  }

  // TCP is connection oriented, a reliable connection
  // **must** be established before any data is exchanged
  if (connect(sock, result->ai_addr, result->ai_addrlen) < 0) {
    perror("connect");
    freeaddrinfo(result);
    exit(1);
  }

  int stat=0;
  char* bufferc = malloc(sizeof(header));
  memset(bufferc, '\0', sizeof(header));
  header x;
  memset(&x, '\0',sizeof(header));
  x.filesize = 0;
  x.cmd = 'c';
  memcpy(bufferc, &x, sizeof(header));
  char* mesg = bufferc;
  // send
  // data that will be sent to the server
  const char* data_to_send = mesg;
  stat = send(sock, data_to_send, sizeof(header), 0);

  // print result
  if (stat<0){
    printf("fail to send close message.\n");
  } else {
    printf("successfully sent close message.\n");
  }

  // close the socket
  close(sock);
  freeaddrinfo(result);
  free(bufferc);
  return 0;
}
