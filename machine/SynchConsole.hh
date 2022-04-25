#ifndef NACHOS_MACHINE_SYNCHCONSOLE__HH
#define NACHOS_MACHINE_SYNCHCONSOLE__HH
#include "console.hh"
#include "../threads/semaphore.hh"
#include "../threads/lock.hh"

class SynchConsole {
    public:
        SynchConsole(const char *, const char *);
        ~SynchConsole();

        void ReadAvail(void *);
        void WriteDone(void *);
        char GetChar();
        void PutChar();
    private:
        Console *console;
        Semaphore *readAvail;
        Semaphore *writeDone;
        Lock *readLock;
        Lock *writeLock;
}

#endif
