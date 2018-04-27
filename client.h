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
 * Write to a socket.
**/
ssize_t write_to_socket(int socket, const char* buffer, size_t count);

/**
 * Read from a socket
**/
ssize_t read_from_socket(int socket, char* buffer, size_t count);

 /**
  * Interface function of the network layer for sending string
  * Need to indicate the string to send, a filename to store as, a port and a server_name
  * filelen is the count of chars. (No need to count in the '\0')
  * Return: The number of bytes read (number of chars)
  **/
ssize_t network_send (char* buffer, const char* filename, const char* server_port, const char* server_name, size_t filelen);

 /**
  * Interface function of the network layer for receiving string
  * Need to indicate the filename to download, a server_port and a server_name
  * filelen is the count of chars. (No need to count in the '\0')
  * Return: The string of content read from slaves on heap. (Needs to be clean by caller)
  **/
 char* network_receive(const char* filename, const char* server_port, const char* server_name, size_t* filelen);
 /**
  * Interface function of the network layer for closing slave
  * Need to indicate the filename to download, a server_port and a server_name
  * Return: success or failure
  **/
  int network_close(const char* server_port, const char* server_name);
