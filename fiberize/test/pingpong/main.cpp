#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;

uint32_t messages = 100000;

Event<AnyFiberRef> init("init");
Event<Unit> ready("ready");

Event<Unit> ping("ping");
Event<Unit> pong("pong");

struct Ping : public Fiber<Unit> {
    Unit run() override {
        auto peer = init.await();
        
        for (uint32_t sent = 0; sent < messages; ++sent) {
            peer.send(ping);
            pong.await();
        }
        
        return {};
    }
};

struct Pong : public Fiber<Unit> {
    Pong(AnyFiberRef mainFiber) : mainFiber(mainFiber) {}
    AnyFiberRef mainFiber;

    Unit run() override {
        auto peer = init.await();
        mainFiber.send(ready);

        for (uint32_t received = 0; received < messages; ++received) {
            ping.await();
            peer.send(pong);
        }

        return {};
    }
};

TEST(PingPong, PingPong) {
    FiberSystem system;
    AnyFiberRef self = system.fiberize();
    system.subscribe(self);
    
    auto ping = system.run<Ping>();
    auto pong = system.run<Pong>(self);
    
    pong.send(init, ping);
    ready.await();
    ping.send(init, pong);
    
    system.allFibersFinished().await();
}
