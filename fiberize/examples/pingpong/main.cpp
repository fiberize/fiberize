#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

// First we declare some events. The default constructor assigns a locally
// unique id to the event. Events can have attached values.

Event<FiberRef> hello;
Event<void> ack;

Event<void> ping;
Event<void> pong;

// Alice and Bob are going to play ping pong. Alice starts.
void alice(FiberRef peer) {
    // The fiberize::context namespace contains helper functions available in fibers.
    // The one we need is self(), which returns a reference to the currently running fiber.
    using namespace context;

    // In the first step we perform a handshake with the peer.
    // peer.send(event, attachedValue) sends an event to the fiber referenced by "peer"
    peer.send(hello, self());
    // hello.await() will "block" until the current fiber receives an ack message.
    // Execution will switch to another fiber when this fiber is waiting.
    ack.await();

    // Now we enter an the main loop.
    while (true) {
        std::cout << "Ping" << std::endl;
        peer.send(ping);
        pong.await();
    }
}

// Fibers can be defined as function objects, possibly with some methods and state.
struct Bob {
    void operator () () {
        // Perform the handshake.
        FiberRef peer = hello.await();
        peer.send(ack);

        // Enter the loop.
        while (true) {
            ping.await();
            std::cout << "Pong" << std::endl;
            peer.send(pong);
        }
    }
};

int main() {
    // The FiberSystem by default will create an OS thread for each CPU core we have.
    // After initializing the system, we fiberize the current thread. This means it will
    // be able to start and communicate with fibers.
    FiberSystem system;
    system.fiberize();

    // A fiber can be created from any function, lambda or function object. The fiber(...)
    // function returns a Builder, which is used to configure the fiber and start it.
    auto bobRef = system.fiber(Bob{}).run();
    auto aliceRef = system.fiber(alice).run(bobRef);

    // Enter an infinite loop processing events.
    EventContext::current()->processForever();
}
