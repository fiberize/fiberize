#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

struct Noop : public Fiber<Unit> {
    Unit run() {}
};

int main() {
    System system;

    for (int i = 0; i < 10000000; ++i) {
        system.run<Noop>();
        std::cout << i << std::endl;
    }

    system.allFibersFinished().await(system.mainContext());
    return 0;
}
