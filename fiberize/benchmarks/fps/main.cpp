#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const size_t fibers = 1000 * 1000 * 4;
const size_t spawners = 8;

struct Noop : public Fiber {
    void run() override {}
};

struct Spawner : public Fiber {
    void run() override {
        for (size_t i = 1; i <= fibers; ++i) {
            system()->run<Noop>();
            if (i % 100 == 0) {
                yield();
            }
        }
    }
};

int main() {
    FiberSystem system;
    FiberRef self = system.fiberize();
    system.subscribe(self);

    for (size_t i = 0; i < spawners; ++i) {
        system.run<Spawner>();
    }

    system.allFibersFinished().await();
    return 0;
}
