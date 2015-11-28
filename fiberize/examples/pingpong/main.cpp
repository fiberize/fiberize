#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;
using namespace std::literals;

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
        io::sleep(500ms);
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
            io::sleep(500ms);
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

    // A fiber/future can be created from any function, lambda or function object. The future()
    // function returns a Builder, which is used to configure the future and start it. The only
    // difference between a fiber and a future is that we can await a future to get it's result.
    auto bobRef = system.future(Bob{}).run();
    auto aliceRef = system.future(alice).run(bobRef);

    // Let them run for a few seconds.
    io::sleep(5s);

    // Kill the futures. The kill() function sends a special event which will trigger
    // the Killed exception inside the fiber/future.
    bobRef.kill();
    aliceRef.kill();

    // Wait until both futures finish. The Killed exception is propagated throught
    // the result, so we have to catch it.
    try {
        bobRef.await();
    } catch (Killed&) {
        std::cout << "Bob is dead." << std::endl;
    }
    try {
        aliceRef.await();
    } catch (Killed&) {
        std::cout << "Alice is dead." << std::endl;
    }

    return 0;
}
