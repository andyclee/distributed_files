#include "client.h"

/**
 * Encrypt the file, dummy function
 **/
char* encrypt_f(char* str){
  return str;
}

/**
 * Decrypt the file, dummy function
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



/**
 * Interface function of the network layer for sending string
 * Need to indicate the string to send, a filename to store as, a port and a server_name
 **/
int network_send (char* buffer, const char* filename, const char* server_port, const char* server_name) {
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
  stat = send_s(sock, buffer, filename);

  // print result
  if (stat<0){
    printf("fail to process file %s.\n",filename);
  } else {
    printf("successfully process file %s.\n",filename);
  }

  // close the socket
  close(sock);
  freeaddrinfo(result);
	return 0;
}

/**
 * Interface function of the network layer for receiving string
 * Need to indicate the filename to download, a server_port and a server_name
 **/
char* network_receive(const char* filename, const char* server_port, const char* server_name){
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

  char* buffer = receive_s(sock, filename);

  // close the socket
  close(sock);
  freeaddrinfo(result);
  return buffer;
}
