#ifndef AUX_H
#define AUX_H
#include <stdio.h>
//#include "u2u_def.h"


int tog_fun(bool t);
int ascii_to_int(char* str);
void int_to_ascii(char* buf, int i, int p);
int len(const char* s);
int cmp(char* s1, const char* s2);
int copy_str(char* buffer, const char* s, int index);

#endif
