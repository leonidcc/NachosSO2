#ifndef NACHOS_MACHINE_SYNCHCONSOLE__HH
#define NACHOS_MACHINE_SYNCHCONSOLE__HH
#include "console.hh"
#include "../threads/semaphore.hh"

class SynchConsole {
    public:
        SynchConsole(const char *, const char *);
        ~SynchConsole();

        static void ReadAvail(void *);
        static void WriteDone(void *);
        char GetChar();
        void PutChar();
    private:
        static Console *console;
        static Semaphore *readAvail;
        static Semaphore *writeDone;
}

#endif
