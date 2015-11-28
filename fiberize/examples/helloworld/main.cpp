#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const size_t fibers = 1000000;

Event<void> finished;
std::atomic<size_t> spawned(0);

int main() {
    FiberSystem system;
    FiberRef mainThread = system.fiberize();

    auto printer = system.fiber([mainThread] (int n) {
        std::cout << "Hello from fiber #" << n << std::endl;

        if (std::atomic_fetch_add(&spawned, size_t(1)) == fibers - 1) {
            mainThread.send(finished);
        }
    });

    for (size_t i = 0; i < fibers; ++i) {
        printer.run_(i);
    }

    finished.await();
    return 0;
}
