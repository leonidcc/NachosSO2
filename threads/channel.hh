#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH
#include "lock.hh"
#include "thread.hh"
#include "condition.hh"
#include <cstdlib>
#include <string.h>

class Channel {
public:
    Channel(const char *debugName);
    ~Channel();

    const char *GetName() const;

    void Send(int message);
    void Receive(int *message);
private:
    const char *name;

    Lock *lock;
    Condition *condition;
    int* buffer;
    bool ready;
};

#endif
