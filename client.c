#include "client.h"

ssize_t write_to_socket(int socket, const char* buffer, size_t count){
  ssize_t ret = 0;
  while (ret < (ssize_t)count) {
      ssize_t len = write(socket, buffer+ret, count - ret);
      if (len == 0) {
        break;
      } else if (len==-1 && errno == EINTR) {
        errno = 0;
        continue;
      } else if (len==-1 && errno!=EINTR) {
        return -1;
      }
      ret += len;
  }
  return ret;
}

ssize_t read_from_socket(int socket, char* buffer, size_t count){
    ssize_t ret = 0;

    while (ret < (ssize_t)count) {
      ssize_t len = read(socket, buffer + ret, count-ret);
      if (len == 0) {
        break;
      } else if (len == -1 && errno == EINTR) {
        errno = 0;
        continue;
      } else if (len == -1 && errno != EINTR) {
        // other error 
        return -1;
      }
      ret += len;
    }

    return ret;
}


/**
 * Send a string
**/
ssize_t send_s(int sock, char* buffer, const char* filename, size_t filelen) {

  char* temp = malloc(sizeof(header)+filelen);
  memset(temp, '\0', sizeof(header)+filelen);
  strncpy(temp+sizeof(header), buffer, filelen);

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
  ssize_t retw =  write_to_socket(sock, data_to_send, sizeof(header));
  ssize_t retw2 = write_to_socket(sock, data_to_send+sizeof(header), x.filesize);

  if (retw<0 || retw2<0) {
    free(temp);
    return -1;
  }

/*  char statBuf[6];
  memset(statBuf, '\0',6);  
  int retr = read(sock,statBuf,6);
  if (retr>0){
    if (strncmp(statBuf, "ERROR",5)==0){
      free(temp);
      return -1;
    }
  }*/

  free(temp);
  return x.filesize;
}


/**
 * Receive a string
**/

char* receive_s(int sock, const char* filename, size_t* filelen){

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
    ssize_t head_size = write_to_socket(sock, data_to_send, sizeof(header));

    if (head_size < 0){
        //  perror("sent wrong\n");
          return NULL;
    }

    // receive
    //receive the string that contain the file

    char head [sizeof(header)];
    ssize_t len = read_from_socket(sock, head, sizeof(header));
    if (len < 0) {
      return NULL;
    }

    header r;
    r = *(header*)head;
    if (r.cmd!='d'){
      //printf("%c,%d\n",r.cmd,len);
      //perror("1\n");
      return NULL;
    }

    char* buffer2 = malloc(r.filesize);
    len = read_from_socket(sock, buffer2, r.filesize); 
    if (len <0){
      free(buffer2);
      return NULL;
    }
    *filelen = r.filesize;

    return buffer2;
}



/**
 * Interface function of the network layer for sending string
 * Need to indicate the string to send, a filename to store as, a port and a server_name
 **/
ssize_t network_send (char* buffer, const char* filename, const char* server_port, const char* server_name, size_t filelen) {
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

  ssize_t stat= -1;
  stat = send_s(sock, buffer, filename, filelen);

  // close the socket
  close(sock);
  freeaddrinfo(result);
  return stat;
}

/**
 * Interface function of the network layer for receiving string
 * Need to indicate the filename to download, a server_port and a server_name
 **/
char* network_receive(const char* filename, const char* server_port, const char* server_name, size_t* filelen){
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

  //printf("before receive_s\n");

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

/**
 * Send from client to master to request a list of filenames. 
 * Need to provide a port and a server name, and a preallocated buffer
 * Stored the list content in buffer
 * Return: return 0 for success -1 for failure.  
 **/
int network_list_request(const char* server_port, const char* server_name, char* buffer){
  // allocate enough memory for a header
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

  char* temp = malloc(sizeof(header));
  memset(temp, '\0', sizeof(header));

  //set header!
  header x;
  x.filesize = 0;
  x.cmd = 'l';
  memcpy(temp, &x, sizeof(header));

  char* mesg = temp;
  // send

  // data that will be sent to the server
  const char* data_to_send = mesg;
  ssize_t retw =  write_to_socket(sock, data_to_send, sizeof(header));

  free(temp);

  // receive the header
  char head [sizeof(header)];
  ssize_t len = read_from_socket(sock, head, sizeof(header));
  if (len < 0) {
    return -1;
  }

  header r;
  r = *(header*)head;
  if (r.cmd!='l'){
    return -1;
  }

  len = read_from_socket(sock, buffer, r.filesize);
  if (len < 0){
    return -1;
  }

  close(sock);
  freeaddrinfo(result);

  if (retw<0){
    return -1;
  }
  return r.filesize;
}
