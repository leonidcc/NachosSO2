/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_simple.hh"
#include "system.hh"
#include "semaphore.hh"
#include <stdio.h>
#include <string.h>
#ifdef SEMAPHORE_TEST
    Semaphore *sem = new Semaphore("SimpleThreadTestSem", 3);
#endif

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
void
SimpleThread(void *name_)
{   
    // Reinterpret arg `name` as a string.
    char *name = (char *) name_;

#ifdef SEMAPHORE_TEST
    DEBUG('s', "Thread \'%s\' -> P()\n", name);
    sem->P();
#endif
    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++) {
        printf("*** Thread `%s` is running: iteration %u\n", name, num);
        currentThread->Yield();
    }
#ifdef SEMAPHORE_TEST
    DEBUG('s', "Thread \'%s\' -> V()\n", name);
    sem->V();
#endif
    printf("!!! Thread `%s` has finished\n", name);
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void
ThreadTestSimple()
{   
    char *name2 = new char [64];
    char *name3 = new char [64];
    char *name4 = new char [64];
    char *name5 = new char [64];
    strncpy(name2, "2nd", 64);
    strncpy(name3, "3rd", 64);
    strncpy(name4, "4th", 64);
    strncpy(name5, "5th", 64);
    Thread *thread2 = new Thread(name2);
    Thread *thread3 = new Thread(name3);
    Thread *thread4 = new Thread(name4);
    Thread *thread5 = new Thread(name5);
    thread2->Fork(SimpleThread, (void *) name2);
    thread3->Fork(SimpleThread, (void *) name3);
    thread4->Fork(SimpleThread, (void *) name4);
    thread5->Fork(SimpleThread, (void *) name5);
    SimpleThread((void *) "1st");
}
