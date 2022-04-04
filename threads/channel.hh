#ifndef NACHOS_THREADS_CONDITION__HH
#define NACHOS_THREADS_CONDITION__HH
#include "lock.hh"
#include "thread.hh"
#include "condition.hh"

class Channel {
public:
    Channel(const char *debugName);
    ~Channel();

    const char *GetName() const;

    void SetMsg(char *msg);
    void Send(int message);
    void Receive(int *message);
private:
    const char *name;
    char *buffer;
    Lock *lock;
    Condition *condition;
    int value;
};

#endif