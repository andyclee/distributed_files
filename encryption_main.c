#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "encryption.h"

int main() {
    char* str = "Alice was beginning to get very tired of sitting by her sister on the bank, and of having nothing to do.";
    char* encrypt = encryption(str);
    printf("%s\n", encrypt);
    printf("done encripting\n");
    char* decrypt = decryption(encrypt);
    printf("%s\n", decrypt);

    free(encrypt);
    free(decrypt);
    return 0;
}


