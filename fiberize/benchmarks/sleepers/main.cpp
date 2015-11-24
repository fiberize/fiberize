#include <fiberize/fiberize.hpp>
#include <iostream>
#include <thread>

using namespace fiberize;
using namespace std::literals;

const size_t fibers = 100000;

Event<Unit> go;

struct Sleeper : public Fiber {
    void run() override {
        go.await();
    }
};

int main() {
    FiberSystem system;
    FiberRef self = system.fiberize();
    system.subscribe(self);

    std::vector<FiberRef> refs;

    for (size_t i = 0; i < fibers; ++i) {
        refs.push_back(system.run<Sleeper>());
    }

    std::this_thread::sleep_for(5s);

    for (FiberRef& ref : refs) {
        ref.send(go);
    }
    refs.clear();

    system.allFibersFinished().await();
    std::this_thread::sleep_for(5s);

    return 0;
}
