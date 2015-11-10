#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

struct Printer : public Fiber<Unit> {
    Printer(int n): n(n) {}
    
    Unit run() override {
        std::cout << "Hello from fiber #" << n << std::endl;
        return {};
    }
    
    int n;
};

int main() {
    FiberSystem system;
    AnyFiberRef self = system.fiberize();
    system.subscribe(self);
    
    for (int i = 0; i < 1000000; ++i) {
        system.run<Printer>(i);
    }

    system.allFibersFinished().await();
    return 0;
}
