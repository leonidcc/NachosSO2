/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"
#include <stdio.h>
#define MAX_IT 5

void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        bool bandera = false;
        for (int i = 0; i < MAX_IT; i++) {
            if (machine->ReadMem(userAddress, 1, &temp)) {
                userAddress++;
                bandera = true;
                break;
            }
        }
        ASSERT(bandera);
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
        bool bandera = false;
        for (int i = 0; i < MAX_IT; i++) {
            if (machine->ReadMem(userAddress, 1, &temp)) {
                userAddress++;
                bandera = true;
                break;
            }
        }
        ASSERT(bandera);
        *outString = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void WriteBufferToUser(const char *buffer, int userAddress, unsigned byteCount){
    ASSERT(buffer != nullptr);
    ASSERT(userAddress != 0);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    while (count < byteCount) {
        bool bandera = false;
        for (int i = 0; i < MAX_IT; i++ ) {
            if (machine->WriteMem(userAddress, 1, *buffer)) {
                userAddress++;
                bandera = true;
                break;
            }
        }
        ASSERT(bandera);
        buffer++;
        count++;
    }
}

void WriteStringToUser(const char *string, int userAddress){
    ASSERT(string != nullptr);
    ASSERT(userAddress != 0);

    while (*string != '\0') {
        bool bandera = false;
        for (int i = 0; i < MAX_IT; i++ ) {
            if (machine->WriteMem(userAddress, 1, *string)) {
                userAddress++;
                bandera = true;
                break;
            }
        }
        ASSERT(bandera);
        userAddress++;
        string++;
    }
    // ASSERT(machine->WriteMem(userAddress, 1, '\0'));
}
