#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

struct Printer : public Fiber {
    Printer(int n): n(n) {}
    
    void run() override {
        std::cout << "Hello from fiber #" << n << std::endl;
    }
    
    int n;
};

int main() {
    FiberSystem system;
    FiberRef self = system.fiberize();
    system.subscribe(self);
    
    for (int i = 0; i < 1000000; ++i) {
        system.run<Printer>(i);
    }

    system.allFibersFinished().await();
    return 0;
}
