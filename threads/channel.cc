#include "channel.hh"

Channel::Channel(const char *debugName) {
    name = debugName;
    // strcat((char*)debugName , "-lockChannel")
    lock = new Lock("lockChannel");
    // strcat((char*)debugName , "-conditionChannel"
    condition = new Condition("conditionChannel", lock);
    buffer = NULL;
    ready = true;
}

Channel::~Channel() {
    delete lock;
    delete condition;
    delete name;
    delete buffer;
}

const char *
Channel::GetName() const {
  return name;
}

void
Channel::Send(int message) {
    lock->Acquire();
    buffer = (int *)malloc(sizeof(int));
    buffer[0] = message;
    ready = true;
    // enviamos el mensaje y esperamos
    condition->Wait();
    lock->Release();
}

void
Channel::Receive(int *message) {
    lock->Acquire();
    if (!ready) {
      condition->Wait();
    }
    // recibimos y notificamos ?
    message = buffer;

    ready = false;
    condition->Signal();
    lock->Release();
}
