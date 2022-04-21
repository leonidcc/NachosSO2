#include "SynchConsole.hh"

SynchConsole::SynchConsole(const char *in, const char *out)
{
    this->console = new Console(in, out, ReadAvail, WriteDone, this);
    this->readAvail = new Semaphore("read avail", 0);
    this->writeDone = new Semaphore("write done", 0);
}

static void
SynchConsole::ReadAvail(void * p)
{
    readAvail->V();
}

static void
SynchConsole::WriteDone(void * p)
{
    writeDone->V();
}

char
SynchConsole::GetChar()
{
    readAvail->P();
    return console->GetChar();
}

void
SynchConsole::PutChar(char c)
{
    writeDone->P();
    console->PutChar(c);
}
