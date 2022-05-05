
#include "synch_console.hh"


/// Disk interrupt handler.  Need this to be a C routine, because C++ cannot
/// handle pointers to member functions.
// static void
// DiskRequestDone(void *arg)
// {
//     ASSERT(arg != nullptr);
//     SynchDisk *disk = (SynchDisk *) arg;
//     disk->RequestDone();
// }
static void
ConsoleReadAvail(void *arg)
{
    ASSERT(arg != nullptr);
    SynchConsole *console = (SynchConsole *) arg;
    console->ReadAvail();
}

static void
ConsoleWriteDone(void *arg)
{
    ASSERT(arg != nullptr);
    SynchConsole *console = (SynchConsole *) arg;
    console->WriteDone();
}

// SynchDisk::SynchDisk(const char *name)
// {
//     semaphore = new Semaphore("synch disk", 0);
//     lock = new Lock("synch disk lock");
//     disk = new Disk(name, DiskRequestDone, this);
// }
SynchConsole::SynchConsole()
{
    console   = new Console(nullptr, nullptr, ConsoleReadAvail, ConsoleWriteDone, this);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    readLock  = new Lock("read console lock");
    writeLock = new Lock("write console lock");
}

/// De-allocate data structures needed for the synchronous disk abstraction.
// SynchDisk::~SynchDisk()
// {
//     delete disk;
//     delete lock;
//     delete semaphore;
// }
SynchConsole::~SynchConsole()
{
    delete console;
    delete readAvail;
    delete writeDone;
    delete readLock;
    delete writeLock;
}

// void
// SynchDisk::ReadSector(int sectorNumber, char *data)
// {
//     ASSERT(data != nullptr);
//
//     lock->Acquire();  // Only one disk I/O at a time.
//     disk->ReadRequest(sectorNumber, data);
//     semaphore->P();   // Wait for interrupt.
//     lock->Release();
// }
char
SynchConsole::ReadChar()
{
    readLock->Acquire();
    readAvail->P();        // Wait for character to arrive.
    char ch = console->GetChar();
    readLock->Release();
    return ch;
}

// void
// SynchDisk::WriteSector(int sectorNumber, const char *data)
// {
//     ASSERT(data != nullptr);
//
//     lock->Acquire();  // only one disk I/O at a time
//     disk->WriteRequest(sectorNumber, data);
//     semaphore->P();   // wait for interrupt
//     lock->Release();
// }
void
SynchConsole::WriteChar(char ch)
{
    writeLock->Acquire();
    console->PutChar(ch);  // Echo it!
    writeDone->P();        // Wait for write to finish.
    writeLock->Release();
}

/// Disk interrupt handler.  Wake up any thread waiting for the disk
/// request to finish.
// void
// SynchDisk::RequestDone()
// {
//     semaphore->V();
// }
void
SynchConsole::ReadAvail()
{
    readAvail->V();
}

void
SynchConsole::WriteDone()
{
    writeDone->V();
}
