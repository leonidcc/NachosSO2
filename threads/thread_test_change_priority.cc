#include "thread_test_change_priority.hh"
Thread* hilo1 = new Thread("hilo1", true);
Thread* hilo2 = new Thread("hilo2", true);

void funcion(void * args) {
  while (true) {
    scheduler->Print();
    currentThread->Yield();
    if(currentThread != hilo1) {
      hilo1->ChangePriority(NUM_COLAS - 1);
    }
    if(currentThread != hilo2) {
      hilo2->ChangePriority(NUM_COLAS - 1);
    }
    sleep(2);
  }
}

void ThreadTestChangePriority() {
  scheduler->Print();
  hilo1->Fork(funcion, nullptr);
  hilo2->Fork(funcion, nullptr);
  funcion(nullptr);
}
