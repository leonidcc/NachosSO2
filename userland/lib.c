#include "syscall.h"

unsigned strlen(const char* s)
{
    int c = 0;
    for(;s[c] != '\0'; c++);
    return c;
}

void puts(const char* s)
{
    Write(&s, strlen(s), CONSOLE_OUTPUT);
}

void reverse(const char* buffer, const char*s)
{
    int i = strlen(buffer);
    int j = 0;
    for(;i >= 0; i--, j++) {
        s[j] = buffer[i];
    }
    s[j+1] = '\0';
}

void itoa(int n, const char* s)
{
    int i = 0;
    char buffer[2**32];
    for (; n != 0; i++){
      buffer[i] = (char)n%10;
      n = (int)n/10;
    }
    buffer[i+1] = '\0';

    reverse(buffer, s);
}
