#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

struct Noop : public Fiber<Unit> {
    Unit run() {}
};

int main() {
    System system;
    
    for (int i = 0; i < 1000; ++i) {
        system.runWithResult<Noop>().finished().await();
    }
    
    return 0;
}
