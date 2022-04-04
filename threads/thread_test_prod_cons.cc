/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"
#include <stdio.h>

int itemCount = 0;
int capacidadBuffer = 10;
Lock *lock = new Lock("lockProducerConsumer");
Condition *condition = new Condition("conditionProducerConsumer", lock);

void add() {
  while (itemCount == capacidadBuffer) {
    condition->Wait();
  }

  itemCount ++;

  if (itemCount > 0) {
    condition->Signal();
  }
}

void remove() {
  while (itemCount == 0) {
    condition->Wait();
  }

  itemCount --;

  if (itemCount < capacidadBuffer) {
    condition->Signal();
  }
}

void
producer(void *arg) {
  while (true) {
    printf("%s - %d\n", currentThread->GetName(), itemCount);
    lock->Acquire();
    add();
    lock->Release();
  }
}

void
consumer(void *arg) {
  while(true) {
    printf("%s - %d\n", currentThread->GetName(), itemCount);
    lock->Acquire();
    remove();
    lock->Release();
  }
}

void
ThreadTestProdCons()
{
  Thread *hilo1 = new Thread("consumer1");
  Thread *hilo2 = new Thread("consumer2");
  Thread *hilo3 = new Thread("consumer3");
  void *arg = NULL;
  hilo1->Fork(consumer, arg);
  hilo2->Fork(consumer, arg);
  hilo3->Fork(consumer, arg);
  producer(arg);
}
