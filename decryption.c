/**
 * Distributed Files
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TOTAL_ENCRYPTION 2;
#define RSA_d 1571
#define RSA_N 2257
#define RSA_N_digit 4
#define TRANSPOSE 5

char* transpose_decryption(char* str) {
    char* decrypt = malloc(strlen(str) + 1);
    decrypt[strlen(str)] = '\0';
    for(size_t i = 0; i < strlen(str); i++) {
	int round = (strlen(str)-1)/TRANSPOSE + 1;
	int pos = (i/round) + TRANSPOSE*(i%round);
	decrypt[pos] = str[i];
    }
    return decrypt;
}

char* RSA_decryption(char* str) {
    
    char* decrypt = malloc(strlen(str) / RSA_N_digit + 1);
    decrypt[strlen(str) / RSA_N_digit] = '\0';
    for(size_t i = 0; i < strlen(str); i+=RSA_N_digit) {
        char c[RSA_N_digit + 1];
	for(size_t j = 0; j < RSA_N_digit; j++) {
	    c[j] = str[i+j];
	}
	c[RSA_N_digit] = '\0';
        int num;
	sscanf(c, "%d", &num);
	int rs = 1;
        for(int j = 0; j < RSA_d; j++) {
            rs = (rs * num) % RSA_N;
        }
        decrypt[i/RSA_N_digit] = (char)rs;
    }
    return decrypt;
}

char* decryption(char* str, int type) {
    char* result;
    switch(type) {
        case 1:
            result = transpose_decryption(str);
            break;
        case 2:
            result = RSA_decryption(str);
            break;
        default:
            result = NULL;
    }
    return result;
}

int main() {
    char* str = "11030379000606331676099718271509198209970207167610590006096109610006096110590997026121460997105916760261099718251676043509140997026100060435167619710997214621760997198200060261026100060961105909970207091409970173167604350997198200061982026116760435099721460961099702610173167609970207150909611940171409971509096119710997214621760997017315091825000609611059099709612146026101730006096110590997026121460997197121461635";
    char* message = decryption(str, 2);
    printf("%s\n", message);
    char* str2 = "A bnttye iy e b ovnn lweio  dsn srtaafiogdiagn vt ighi hnn nt ocsiggeiot esoekdhght.e n errftbrtn , a io";
    char* message2 = decryption(str2, 1);
    printf("%s\n", message2);
    return 0;
}

