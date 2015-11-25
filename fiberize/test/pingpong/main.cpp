#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;

uint32_t messages = 1000000;

Event<FiberRef> init;
Event<Unit> ready;

Event<Unit> ping;
Event<Unit> pong;

struct Ping : public Future<Unit> {
    Unit run() override {
        auto peer = init.await();
        
        for (uint32_t sent = 0; sent < messages; ++sent) {
            peer.send(ping);
            pong.await();
        }

        return {};
    }
};

struct Pong : public Future<Unit> {
    Pong(FiberRef mainFiber) : mainFiber(mainFiber) {}
    FiberRef mainFiber;

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
    FiberRef self = system.fiberize();
    
    auto ping = system.run<Ping>();
    auto pong = system.run<Pong>(self);
    
    pong.send(init, ping);
    ready.await();
    ping.send(init, pong);

    ping.result()->await();
    pong.result()->await();
}
