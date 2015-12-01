#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const size_t fibersPerLine = 1000 * 100;
const size_t lines = 100;

Event<void> finished;

FiberRef mainThread;

void fiber(size_t i) {
    if (i == fibersPerLine) {
        mainThread.send(finished);
    } else {
        context::system()->fiber(fiber).run_(i+1);
    }
}

int main() {
    FiberSystem system;
    mainThread = system.fiberize();

    for (size_t i = 0; i < lines; ++i) {
        system.fiber(fiber).run_(1);
    }

    for (size_t i = 0; i < lines; ++i) {
        finished.await();
    }

    return 0;
}
