#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;

uint32_t messages = 1000000;

Event<FiberRef> init;
Event<void> ready;

Event<void> ping;
Event<void> pong;

struct Ping : public Future<void> {
    void run() override {
        auto peer = init.await();
        
        for (uint32_t sent = 0; sent < messages; ++sent) {
            peer.send(ping);
            pong.await();
        }
    }
};

struct Pong : public Future<void> {
    Pong(FiberRef mainFiber) : mainFiber(mainFiber) {}
    FiberRef mainFiber;

    void run() override {
        auto peer = init.await();
        mainFiber.send(ready);

        for (uint32_t received = 0; received < messages; ++received) {
            ping.await();
            peer.send(pong);
        }
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
