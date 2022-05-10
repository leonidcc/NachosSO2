/// Data structures to export a synchronous interface to the raw disk device.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_USERPROG_SYNCHCONSOLE__HH
#define NACHOS_USERPROG_SYNCHCONSOLE__HH

#include "machine/console.hh"
#include "threads/lock.hh"
#include "threads/semaphore.hh"

class SynchConsole {
public:

    /// Initialize a synchronous disk, by initializing the raw Disk.
    SynchConsole();

    /// De-allocate the synch disk data.
    ~SynchConsole();

    /// Read/write a disk sector, returning only once the data is actually
    /// read or written.  These call `Disk::ReadRequest`/`WriteRequest` and
    /// then wait until the request is done.

    char ReadChar();
    void WriteChar(char ch);

    /// Called by the disk device interrupt handler, to signal that the
    /// current disk operation is complete.
    void ReadAvail();
    void WriteDone();

private:
    Console *console; ///< Raw disk device.
    Semaphore *readAvail;  ///< To synchronize requesting thread with the
    Semaphore *writeDone;  ///< To synchronize requesting thread with the
                           ///< interrupt handler.
    Lock *readLock;  ///< Only one read/write request can be sent to the disk at
    Lock *writeLock;  ///< Only one read/write request can be sent to the disk at
                 ///< a time.
};


#endif
