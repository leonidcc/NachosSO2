#include "SynchConsole.hh"

static void
SynchReadAvail(void *arg)
{
  ASSERT(arg != nullptr);
  SynchConsole *console = (SynchConsole *)arg;
  console->ReadAvail();
}

static void
SynchWriteDone(void *arg)
{
  ASSERT(arg != nullptr);
  SynchConsole *console = (SynchConsole *)arg;
  console->WriteDone();
}

SynchConsole::SynchConsole(const char *in, const char *out)
{
    this->console = new Console(in, out, SynchReadAvail, SynchWriteDone, this);
    this->readAvail = new Semaphore("read avail", 0);
    this->writeDone = new Semaphore("write done", 0);
    this->readLock = new Lock("read lock");
    this->writeLock = new Lock("write lock");
}

SynchConsole::~SynchConsole()
{
    delete console;
    delete readAvail;
    delete writeDone;
    delete readLock;
    delete writeLock;
}

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

char
SynchConsole::GetChar()
{
    readLock->Acquire();
    readAvail->P();
    char c = console->GetChar();
    DEBUG('e', "GetChar %c \n", c);
    readLock->Release();
    return c;
}

void
SynchConsole::PutChar(char c)
{
    DEBUG('e', "PutChar %c \n", c);
    writeLock->Acquire();
    writeDone->P();
    console->PutChar(c);
    writeLock->Release();
}
