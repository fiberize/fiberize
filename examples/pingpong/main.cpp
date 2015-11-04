#include <fiberize/fiberize.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using namespace fiberize;

Event<int> ping("ping");
Event<int> pong("pong");

class Ping : public Fiber<Void> {
    virtual Void run() {
        while (true) {
            std::cout << "Ping" << std::endl;
            context()->yield();
        }
    }
};

class Pong : public Fiber<Void> {
    virtual Void run() {
        while (true) {
            std::cout << "Pong" << std::endl;
            context()->yield();
        }
    }
};

class Printer : public Fiber<Unit> {
public:
    Printer(std::string text): text(text) {}
    
    virtual Unit run() {
        std::cout << text << std::endl;
    }
    
    std::string text;
};

int main() {
    using namespace std::literals;
    System system;
    
    auto ping = system.run<Ping>();
    auto pong = system.run<Pong>();
    auto lol = system.run<Printer>("Hi");
    
    while (true) {
        std::this_thread::sleep_for(1s);
    }
}
