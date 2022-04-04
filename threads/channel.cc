#include "channel.hh"

Channel::Channel(const char *debugName) {
    name = debugName;
}

Channel::~Channel() {
    delete lock;
    delete condition;
    delete name;
}

Channel::Send(int message) {
    // TODO
}

Channel::Receive(int *message) {
    // TODO
    return 0;
}