#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;

uint32_t messages = 1000000;

Event<FiberRef> hello;
Event<void> ack;

Event<void> ping;
Event<void> pong;

void alice(FiberRef peer) {
    using namespace context;

    peer.send(hello, self());
    ack.await();

    for (uint32_t sent = 0; sent < messages; ++sent) {
        std::cout << "Ping" << std::endl;
        peer.send(ping);
        pong.await();
    }
}

void bob() {
    FiberRef peer = hello.await();
    peer.send(ack);

    // Enter the loop.
    for (uint32_t received = 0; received < messages; ++received) {
        ping.await();
        std::cout << "Pong" << std::endl;
        peer.send(pong);
    }
}

TEST(PingPong, PingPong) {
    FiberSystem system;
    FiberRef self = system.fiberize();
    
    auto bobRef = system.future(bob).run();
    auto aliceRef = system.future(alice).run(bobRef);
    
    bobRef.result()->await();
    aliceRef.result()->await();
}
