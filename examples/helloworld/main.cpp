#include <fiberize/fiberize.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using namespace fiberize;

struct Printer : public Fiber<Unit> {
    Printer(int n): n(n) {}
    
    virtual Unit run() {
        std::cout << "Hello from fiber #" << n << std::endl;
    }

    int n;
};

int main() {
    using namespace std::literals;
    System system;
    
    for (int i = 0; i < 1000000; ++i) {
        system.run<Printer>(i);
    }
    
    system.shutdown();
    Context::current()->yield();
}
