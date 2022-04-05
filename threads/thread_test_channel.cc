#include "thread_test_channel.hh"
Channel *canal = new Channel("canal");

void
send(void *arg) {
  int buffer = 0;
  while(true) {
    canal->Send(buffer++);
  }
}

void
receive(void *arg) {
  int buffer;
  while(true) {
    canal->Receive(&buffer);
  }
}

void ThreadTestChannel() {
  Thread *receiveThread = new Thread("receive");
  void * arg = NULL;
  receiveThread->Fork(send, arg);
  receive(arg);
}
