#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

Event<FiberRef> init("init");
Event<Unit> ready("ready");

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
    Pong(FiberRef mainFiber) : mainFiber(mainFiber) {}
    FiberRef mainFiber;

    virtual Unit run() {
        auto peer = init.await();
        mainFiber.emit(ready);

        while (true) {
            ping.await();
            std::cout << "Pong" << std::endl;
            peer.emit(pong);
        }
    }
};

int main() {
    System system;
    FiberRef self = system.fiberize();
    
    auto ping = system.run<Ping>();
    auto pong = system.run<Pong>(self);
    
    pong.emit(init, ping);
    ready.await();
    ping.emit(init, pong);
    
    FiberContext::current()->processForever();
}
