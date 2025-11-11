#include <stdio.h>
#include <stdlib.h>


const unsigned char CRC7_POLY = 0x91;

int len(const char* s){
    int i = 0;
    while (s[i] != 0){ i++; }
    return i;
}

char get_crc(char* buffer, char length){
    char i, j, crc = 0;
    for (i=0; i<length; i++){
        crc ^= buffer[i];
        for (j=0; j<8; j++){
            if (crc & 1){ crc ^= CRC7_POLY; }
            crc >>= 1;
        }
    }
    return crc;
}


int crc_test_function(char* buffer){
    int r;
    int length = len(buffer);
    r = get_crc(buffer, length);
    return r;
}


int main(int argc, char* argv[]) {
    int i;
    int crc_val;

    if (argc>1){
        crc_val = crc_test_function(argv[1]);
        printf("%d\n", crc_val);

    }else{
        char buffer[255];
        printf("No arguments provided. Please provide string for crc check.\n", buffer);
        scanf("%127[^\n]", buffer);
        crc_val = crc_test_function(buffer);
        printf("CRC test \"%s\", result %d\n", buffer, crc_val);
    }
    return 0;
}
