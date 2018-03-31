/**
 * Distributed Files
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define RSA_e 7
#define RSA_d 103
#define RSA_N 143
#define RSA_N_digit 2
#define TRANSPOSE 5

/**
 Use transpose and RSA encrytion and decrytion method
*/

char* encryption(char* str) { 
    if(str == NULL) {
	return NULL;
    }   
    size_t size = strlen(str);
    char* encrypt = malloc(size + 1);
    encrypt[size] = '\0';
    /*
    for(size_t i = 0; i < size; i++) {
        int round = (size-1)/TRANSPOSE + 1;
        int pos = (i%TRANSPOSE)*round + (i/TRANSPOSE);
        encrypt[pos] = str[i];
    }    
    */
    for(size_t i = 0; i < size; i++) {
	int c = (int)str[i];
	
	int rs = 1;
	for(size_t j = 0; j < RSA_e; j++) {
	    rs = (rs * c) % RSA_N;
	}
	encrypt[i] = (char) rs;
    }
    return encrypt;
}

char* decryption(char* str) {
    if(str == NULL) {
	return NULL;
    }
    size_t size = strlen(str);
    char* decrypt = malloc(size + 1);
    decrypt[size] = '\0';    
    for(size_t i = 0; i < size; i++) {
	int num = (int)str[i];
	if(num < 0) {
	    num = num + 256;
	}
        int rs = 1;
        for(int j = 0; j < RSA_d; j++) {
            rs = (rs * num) % RSA_N;
        }
        decrypt[i] = (char)rs;
	//decrypt[i] = str[i];
    }
    /*
    char* decrypt2 = malloc(size + 1);
    decrypt2[size] = '\0';
    for(size_t i = 0; i < size; i++) {
        int round = (size-1)/TRANSPOSE + 1;
        int pos = (i/round) + TRANSPOSE*(i%round);
        decrypt2[pos] = str[i];
    }
    free(decrypt);*/
    return decrypt;
}
