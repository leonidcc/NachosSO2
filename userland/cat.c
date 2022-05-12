#include "syscall.h"
#define MAX_BUFF 1024

int main(int argc, char** argv)
{
    char buffer[MAX_BUFF];
    if (!(argc >= 2)) {
        // avisar error
        return -1;
    }
    OpenFileId o = Open(argv[1]);
    if (o == -1) {
        // error no existe archivo
        return -1;
    }

    int i = 0;
    do {
        Read(&buffer[i], 1, o);
        Write(buffer[i], 1, CONSOLE_OUTPUT);
    } while (buffer[i++] != EOF && i < MAX_BUFF);
    Close(o);
}
