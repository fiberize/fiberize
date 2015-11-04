#include <fiberize/fiberize.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using namespace fiberize;

class Ping : public Fiber<Unit> {
    virtual Unit run() {        
        std::cout << "Ping" << std::endl;
        return {};
    }
};

class Pong : public Fiber<Unit> {
    virtual Unit run() {
        std::cout << "Pong" << std::endl;
        return {};
    }
};

int main() {
    using namespace std::literals;
    System system;
    
    auto ping = system.run<Ping>();
    auto pong = system.run<Pong>();
    
    while (true) {
        std::this_thread::sleep_for(1s);
    }
}
