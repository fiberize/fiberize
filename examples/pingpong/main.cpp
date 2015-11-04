#include <fiberize/fiberize.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using namespace fiberize;

Event<FiberRef> init("init");
Event<int> ping("ping");
Event<int> pong("pong");

struct Ping : public Fiber<Unit> {
    virtual Unit run() {
        auto peer = init.await();
        
        int n = 0;
        while (n < 1000000) {
            peer.emit(ping, n);
            n = pong.await();
        }
    }
};

struct Pong : public Fiber<Unit> {
    virtual Unit run() {
        auto peer = init.await();

        int n;
        while (n < 1000000) {
            n = ping.await();
            peer.emit(pong, n + 1);
        }
    }
};

int main() {
    using namespace std::literals;
    System system;
    
    auto ping = system.run<Ping>();
    auto pong = system.run<Pong>();
    
    ping.emit(init, pong);
    pong.emit(init, ping);
    
    while (true) {
        std::this_thread::sleep_for(1s);
    }
}
