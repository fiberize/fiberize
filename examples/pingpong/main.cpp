#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

Event<FiberRef> init("init");
Event<Unit> ping("ping");
Event<Unit> pong("pong");

struct Ping : public Fiber<Unit> {
    virtual Unit run() {
        auto peer = init.await();
        
        while (true) {
            std::cout << "Ping" << std::endl;
            peer.emit(ping);
            pong.await();
        }
    }
};

struct Pong : public Fiber<Unit> {
    virtual Unit run() {
        auto peer = init.await();

        while (true) {
            ping.await();
            std::cout << "Pong" << std::endl;
            peer.emit(pong);
        }
    }
};

int main() {
    System system;
    
    auto ping = system.runNamed<Ping>("ping");
    auto pong = system.runNamed<Pong>("pong");
    
    ping.emit(init, pong);
    pong.emit(init, ping);
    
    Context::current()->yield();
}
