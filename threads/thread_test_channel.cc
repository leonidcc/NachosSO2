#include "thread_test_channel.hh"
Channel *canal = new Channel("canal");

void send(void *arg) {
  int buffer = 5;
  while(true) {
    printf("send %d\n", buffer);
    canal->Send(buffer);
  }
}

void receive(void *arg) {
  int *buffer = NULL;
  while(true) {
    canal->Receive(buffer);
    printf("%d", buffer[0]);
    // printf("receive %d\n", buffer[0]);
  }
}

void ThreadTestChannel() {
  Thread *receiveThread = new Thread("receive");
  receiveThread->Fork(receive, NULL);
  send(NULL);
}
