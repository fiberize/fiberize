#include <fiberize/fiberize.hpp>
#include <iostream>
#include <thread>

using namespace fiberize;
using namespace std::literals;

const size_t fibers = 100000;

int main() {
    FiberSystem system;
    system.fiberize();

    std::vector<FutureRef<void>> refs;

    Event<void> go;
    auto sleeper = system.future([&go] () {
        go.await();
    });

    for (size_t i = 0; i < fibers; ++i) {
        refs.push_back(sleeper.copy().run());
    }

    std::this_thread::sleep_for(5s);

    for (FutureRef<void>& ref : refs) {
        ref.send(go);
    }

    for (FutureRef<void>& ref : refs) {
        ref.await();
    }
    std::this_thread::sleep_for(5s);

    return 0;
}
