#include "syscall.h"


int
main(void)
{
    // Create("test.txt");
    // OpenFileId o = Open("test.txt");
    // Write("Hello world\n",12,o);
    // Close(o);
    // Remove("test.txt");

    // Create("test.txt");
    // OpenFileId o = Open("test.txt");
    // Write("Hello world\n",12,o);
    // Close(o);
    //
    // char buffer[100];
    // OpenFileId id = Open("test.txt");
    // Read(buffer,100, id);
    // Write(buffer,100,1);
    // Close(o);

    char buffer[100];
    Read(buffer,100, 0);
    Write(buffer,100,1);

    return 0;
}
