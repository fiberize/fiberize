#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const size_t fibers = 1000000;

Event<void> finished;
std::atomic<size_t> spawned(0);

FiberRef mainThread;

struct Printer : public Fiber {
    Printer(int n): n(n) {}
    
    void run() override {
        std::cout << "Hello from fiber #" << n << std::endl;

        if (std::atomic_fetch_add(&spawned, size_t(1)) == fibers - 1) {
            mainThread.send(finished);
        }
    }
    
    int n;
};

int main() {
    FiberSystem system;
    mainThread = system.fiberize();

    auto printer = system.fiber<Printer>();
    for (size_t i = 0; i < fibers; ++i) {
        printer.run_(i);
    }

    finished.await();
    return 0;
}
