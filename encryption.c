/**
 * Distributed Files
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define TOTAL_ENCRYPTION 2;
#define RSA_e 11
#define RSA_N 2257
#define RSA_N_digit 4
#define TRANSPOSE 5

char* transpose_encryption(char* str) {
    char* encrypt = malloc(strlen(str) + 1);
    encrypt[strlen(str)] = '\0';
    for(size_t i = 0; i < strlen(str); i++) {
	int round = (strlen(str)-1)/TRANSPOSE + 1;
	int pos = (i%TRANSPOSE)*round + (i/TRANSPOSE);
	encrypt[pos] = str[i];
    }
    return encrypt;
}

char* RSA_encryption(char* str) {
    char* encrypt = malloc(strlen(str) * 4 + 1);
    encrypt[strlen(str) * 4] = '\0';
    for(size_t i = 0; i < strlen(str); i++) {
	int c = (int)str[i];
	int rs = 1;
	for(size_t j = 0; j < RSA_e; j++) {
	    rs = (rs * c) % RSA_N;
	}
	for(size_t k = 0; k < RSA_N_digit; k++) {
	    int digit = 1;
	    for(size_t m = 0; m < (RSA_N_digit - k -1); m++) {
		digit = digit * 10;
	    } 
	    encrypt[4 * i + k] = ('0' + rs / digit);
	    rs = rs - rs/digit * digit;
	}
    }
    return encrypt;
}

char* encryption(char* str, size_t type) {
    char* result;
    switch(type) {
	case 1:
	    result = transpose_encryption(str);
	    break;
	case 2:
	    result = RSA_encryption(str);
	    break;
	default:
	    result = NULL;
    }
    return result;
}

int main() {
    char* str = "Alice was beginning to get very tired of sitting by her sister on the bank, and of having nothing to do.";
    char* hidden = encryption(str, 2);
    printf("%s\n", hidden);
    char* hidden2 = encryption(str, 1);
    printf("%s\n", hidden2);
    return 0;
}
