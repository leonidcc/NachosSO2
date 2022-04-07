/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_join.hh"
#include "system.hh"
#include "semaphore.hh"
#include "channel.hh"
#include "thread.hh"
#include  <unistd.h>
#include <stdio.h>

void func(void* arg){
  sleep(1);
}

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
void ThreadTestJoin(){
  Thread *t = new Thread("Hijo", true);
  void * arg = NULL;
  t->Fork(func, arg);
  printf("Esperando\n");
  t->Join();
  printf("Llego\n");
}
