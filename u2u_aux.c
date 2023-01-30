#ifndef AUX_C
#define AUX_C
#include <stdio.h>
#include "u2u_aux.h"

int tog_fun(bool t){
    return 1-t;
}

int ascii_to_int(char* str){
    int len = 0;
    int sum = 0;
    int decs[] = {1, 10, 100, 1000, 10000, 100000, 1000000};
    while (str[len] != 0){
        len++;
    }
    for (int c=0; c<len; c++){
        sum = sum + (decs[len-c-1] * (str[c]-48));
    }
    //if (enforce_max && rx_text_len > MESSAGE_MAX_LENGTH){
    //    rx_text_len = MESSAGE_MAX_LENGTH;
    //}
    return sum;
}

void int_to_ascii(char* buf, int i, int p) {
        int tens[] = {100000, 10000, 1000, 100, 10, 1};
        int f = 5-p; //p denotes 'precision' or number of digits.
        int c=0;
        for (c; c<p; c++) {
                buf[c] = 48 + (i % tens[f+c] / (tens[f+c+1]));
        }
        buf[c] = '\0';
        return;
}

int len(const char* s){
    int i = 0;
    while (s[i] != 0){
        i++;
    }
    return i;
}

int cmp(char* s1, const char* s2){
    //Return True if strings compare equal, False if not or if either of the two is null.
    int l1 = len(s1);
    int l2 = len(s2);
    int r = 0;
    int i = 0;
    if (l1*l2){    // Asserting both are non-zero.
        if (l1<l2){ i = l1; }
        else{ i = l2; }
        r = 1;
        for (int c=0; c<i; c++){
            r = r * ((int) s1[c] == (int) s2[c]);
        }
    }
    return r;
}

int copy_str(char* buffer, const char* s, int index){
    //Copy over string items into a message.
    int i = 0;
    while (s[i] !=0){
        buffer[index] = s[i];
        i++;
        index++;
    }
    return index;
}

#endif

