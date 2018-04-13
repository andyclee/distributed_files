/**
  distributed files
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "encryption.h"


/**
   compress the string
   str - char array to be compressed
   compress_size - int pointer to store the size of the compressed char array
   @return a compressed char array
*/
char* compress(char* str, int* compress_size);

/**
   decompress the string
   str - char array to be decompress
   @return a decompressed char array
*/
char* decompress(char* str);
