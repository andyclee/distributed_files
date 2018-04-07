/**
 * Distributed File
 * Client header on the network layer
 */

 #include <arpa/inet.h>
 #include <stdio.h>
 #include <string.h>
 #include <sys/socket.h>
 #include <unistd.h>
 #include <string.h>
 #include <stdlib.h>
 #include <stdint.h>
 #include <sys/types.h>
 #include <netdb.h>

 typedef struct header_t{
   // u for upload, d for download
   char cmd;
   // size of file
   uint32_t filesize;
   // filename must be less than 30 chars
   char filename[31];
 } header;

 /**
  * Interface function of the network layer for sending string
  * Need to indicate the string to send, a filename to store as, a port and a server_name
  **/
 int network_send (char* buffer, const char* filename, const char* server_port, const char* server_name);

 /**
  * Interface function of the network layer for receiving string
  * Need to indicate the filename to download, a server_port and a server_name
  **/
 char* network_receive(const char* filename, const char* server_port, const char* server_name);
