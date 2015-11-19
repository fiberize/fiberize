#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const size_t fibers = 1000 * 1000 * 10;
const size_t spawners = 8;

struct Noop : public Fiber<Unit> {
    Unit run() override { return {}; }
};

struct Spawner : public Fiber<Unit> {
    Unit run() override {
        for (size_t i = 1; i <= fibers; ++i) {
            system()->run<Noop>();
            if (i % 100 == 0) {
                yield();
            }
        }
        return {};
    }
};

int main() {
    FiberSystem system;
    AnyFiberRef self = system.fiberize();
    system.subscribe(self);

    for (size_t i = 0; i < spawners; ++i) {
        system.run<Spawner>();
    }

    system.allFibersFinished().await();
    return 0;
}
