#include "channel.hh"

Channel::Channel(const char *debugName) {
    name = debugName;
    // strcat((char*)debugName , "-lockChannel")
    lock = new Lock("lockChannel");
    // strcat((char*)debugName , "-conditionChannel"
    full = new Condition("fullConditionChannel", lock);
    empty = new Condition("emptyConditionChannel", lock);
    ready = false;
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

    DEBUG('c', "Channel %s Send \"%d\"\n", GetName(), message);

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

    DEBUG('c', "Channel %s Receive \"%d\"\n", GetName(), *message);

    message[0] = buffer;

    ready = false;
    empty->Signal();
    lock->Release();
}
