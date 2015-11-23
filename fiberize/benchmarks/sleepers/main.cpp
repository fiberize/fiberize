#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const size_t fibers = 100000;

struct Sleeper : public Fiber {
    void run() override {
        processForever();
    }
};

int main() {
    std::cout << sizeof(detail::FiberControlBlock) << std::endl;

    FiberSystem system;
    system.fiberize();

    for (size_t i = 0; i < fibers; ++i) {
        system.run_<Sleeper, BlockingDequeMailbox>();
    }

    EventContext::current()->processForever();
}
