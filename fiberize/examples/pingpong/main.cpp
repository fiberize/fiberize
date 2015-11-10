#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

Event<AnyFiberRef> init("init");
Event<Unit> ready("ready");

Event<Unit> ping("ping");
Event<Unit> pong("pong");

struct Ping : public Fiber<Unit> {
    virtual Unit run() {
        auto peer = init.await();
        
        while (true) {
            std::cout << "Ping" << std::endl;
            peer.send(ping);
            pong.await();
        }
    }
};

struct Pong : public Fiber<Unit> {
    Pong(AnyFiberRef mainFiber) : mainFiber(mainFiber) {}
    AnyFiberRef mainFiber;

    virtual Unit run() {
        auto peer = init.await();
        mainFiber.send(ready);

        while (true) {
            ping.await();
            std::cout << "Pong" << std::endl;
            peer.send(pong);
        }
    }
};

int main() {
    FiberSystem system;
    AnyFiberRef self = system.fiberize();
    
    auto ping = system.run<Ping>();
    auto pong = system.run<Pong>(self);
    
    pong.send(init, ping);
    ready.await();
    ping.send(init, pong);
    
    FiberContext::current()->processForever();
}
