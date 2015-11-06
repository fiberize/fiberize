#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

struct Noop : public Fiber<Unit> {
    Unit run() {}
};

int main() {
    System system;

    for (int i = 0; i < 100; ++i) {
        system.run<Noop>();
    }

    system.allFibersFinished().await(system.mainContext());
    return 0;
}
