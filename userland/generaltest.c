#include "syscall.h"


int
main(void)
{
    Create("test.txt");
    OpenFileId o = Open("test.txt");
    Write("Hello world\n",12,o);
    Close(o);
    Remove("test.txt");
    return 0;
}
