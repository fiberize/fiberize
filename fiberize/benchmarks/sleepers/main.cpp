#include <fiberize/fiberize.hpp>
#include <iostream>
#include <thread>

using namespace fiberize;
using namespace std::literals;

const size_t fibers = 100000;

Event<Unit> go;

struct Sleeper : public Future<Unit> {
    Unit run() override {
        go.await();
        return {};
    }
};

int main() {
    FiberSystem system;
    system.fiberize();

    std::vector<FutureRef<Unit>> refs;

    for (size_t i = 0; i < fibers; ++i) {
        refs.push_back(system.run<Sleeper>());
    }

    std::this_thread::sleep_for(5s);

    for (FutureRef<Unit>& ref : refs) {
        ref.send(go);
    }

    for (FutureRef<Unit>& ref : refs) {
        ref.result()->await();
    }
    std::this_thread::sleep_for(5s);

    return 0;
}
