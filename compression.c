/**
  distributed files
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "encryption.h"

struct h_node {
    char data;
    unsigned int frequency;
    struct h_node* left;
    struct h_node* right;
};
typedef struct h_node h_node;

char* compress(char* str) {
    if(str == NULL) {
	return NULL;
    }
    char* encrypt = encryption(str);




    return encrypt;
}

char* decompress(char* str) {
    if(str == NULL) {
	return NULL;
    }




    char* decrypt = decryption(str);
    return decrypt;
}
