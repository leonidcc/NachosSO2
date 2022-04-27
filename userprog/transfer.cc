/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"


void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    // TODO: implement.
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outBuffer = (unsigned char) temp;
        outBuffer++;
    } while (count < byteCount);

}

// resuelto por defecto
bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outString = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void WriteBufferToUser(const char *buffer, int userAddress,
                       unsigned byteCount)
{
    // TODO: implement.
    ASSERT(buffer != nullptr);
    ASSERT(userAddress != 0);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    while (*buffer != '\0' && count <= byteCount) {
        ASSERT(machine->WriteMem(userAddress, 1, *buffer));
        count++;
        buffer++;
    }
}

void WriteStringToUser(const char *string, int userAddress)
{
    // TODO: implement.
    ASSERT(string != nullptr);
    ASSERT(userAddress != 0);

    while (*string != '\0') {
        ASSERT(machine->WriteMem(userAddress, 1, *string));
        string++;
    }
}
