#include "syscall.h"

int main(int argc, char** argv) {
    if (!(argc >= 2)) {
       // ERROR
        return -1;
    }
    return Remove(argv[1]);
}
