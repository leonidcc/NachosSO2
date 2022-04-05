#include "channel.hh"

Channel::Channel(const char *debugName) {
    name = debugName;
    // strcat((char*)debugName , "-lockChannel")
    lock = new Lock("lockChannel");
    // strcat((char*)debugName , "-conditionChannel"
    full = new Condition("fullConditionChannel", lock);
    empty = new Condition("emptyConditionChannel", lock);
    ready = true;
}

Channel::~Channel() {
    delete lock;
    delete full;
    delete empty;
    delete name;
}

const char *
Channel::GetName() const {
  return name;
}

void
Channel::Send(int message) {
    lock->Acquire();
    while (ready) {
      empty->Wait();
    }

    printf("send %d\n", message);
    buffer = message;
    ready = true;

    full->Signal();
    lock->Release();
}

void
Channel::Receive(int *message) {
    lock->Acquire();
    while (!ready) {
      full->Wait();
    }

    printf("receive %d\n", buffer);

    message[0] = buffer;

    ready = false;
    empty->Signal();
    lock->Release();
}
