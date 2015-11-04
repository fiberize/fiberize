#include <fiberize/fiberize.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using namespace fiberize;

class Printer : public Fiber<Unit> {
public:
    Printer(int n): n(n) {}
    
private:
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
    while (true) {
        std::this_thread::sleep_for(1s);
    }
}
