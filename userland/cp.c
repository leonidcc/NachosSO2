#include "syscall.h"
#include "lib.h"
#define MAX_BUFF 1024

int main(int argc, char** argv)
{
    char c;
    if (!(argc >= 3)) {
        // avisar error
        return -1;
    }
    OpenFileId o1 = Open(argv[1]);
    char *ruta = concat(concat(argv[2], "/"), argv[1])
    Create(ruta);
    OpenFileId o2 = Open(ruta);

    int i = 0;
    Read(&c, 1, o1);
    while (c != EOF) {
        Write(c, 1, o2);
        Read(&c, 1, o1);
    }

    Close(o1);
    Close(o2);
}
