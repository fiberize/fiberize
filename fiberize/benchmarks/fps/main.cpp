#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const size_t fibers = 1000 * 1000 * 4;
const size_t spawners = 8;

Event<void> finished;
std::atomic<size_t> spawned(0);

FiberRef mainThread;

struct Noop : public Fiber {
    void run() override {
        if (std::atomic_fetch_add(&spawned, size_t(1)) == fibers * spawners - 1) {
            mainThread.send(finished);
        }
    }
};

struct Spawner : public Fiber {
    void run() override {
        auto builder = system()->fiber<Noop>();
        for (size_t i = 1; i <= fibers; ++i) {
            builder.run_();
            if (i % 100 == 0) {
                yield();
            }
        }
    }
};

int main() {
    FiberSystem system;
    mainThread = system.fiberize();

    for (size_t i = 0; i < spawners; ++i) {
        system.fiber<Spawner>().run_();
    }

    finished.await();
    return 0;
}
