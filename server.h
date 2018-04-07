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
