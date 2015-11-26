#include <fiberize/fiberize.hpp>
#include <iostream>
#include <thread>

using namespace fiberize;
using namespace std::literals;

const size_t fibers = 100000;

Event<void> go;

struct Sleeper : public Future<void> {
    void run() override {
        go.await();
    }
};

int main() {
    FiberSystem system;
    system.fiberize();

    std::vector<FutureRef<void>> refs;

    for (size_t i = 0; i < fibers; ++i) {
        refs.push_back(system.future<Sleeper>().run());
    }

    std::this_thread::sleep_for(5s);

    for (FutureRef<void>& ref : refs) {
        ref.send(go);
    }

    for (FutureRef<void>& ref : refs) {
        ref.result()->await();
    }
    std::this_thread::sleep_for(5s);

    return 0;
}
