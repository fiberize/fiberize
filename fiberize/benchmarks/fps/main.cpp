#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const size_t fibers = 1000 * 1000;
const size_t spawners = 8;

struct Noop : public Fiber<Unit> {
    Unit run() { return {}; }
};

struct Spawner : public Fiber<Unit> {
    Unit run() {
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
    System system;

    for (size_t i = 0; i < spawners; ++i) {
        system.run<Spawner>();
    }

    system.allFibersFinished().await(system.mainContext());
    return 0;
}
