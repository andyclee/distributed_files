#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "compression.h"

int main() {
    char* str = "Alice was beginning to get very tired.";
    char* comp = encryption(str);
    printf("%s\n", comp);

    char* decomp = decryption(comp);
    printf("%s\n", decomp);
    return 0;
}
