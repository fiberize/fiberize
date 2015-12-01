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

// Bob will be an actor. Actors begin their life in the same way as fibers, but after the
// initial function exits they don't die - instead they start processing messages.
// That message processing phase is very efficient, as an actor waiting for a message
// does not need a stack.
struct Bob {
    static Event<void> killed;

    HandlerRef handlePing;
    HandlerRef handleKill;

    void operator () (FiberRef main) {
        // Perform the handshake.
        FiberRef peer = hello.await();
        peer.send(ack);

        // Bind a handler that will remain once we exit this loop.
        handlePing = ping.bind([peer] () {
            io::sleep(500ms);
            std::cout << "Pong" << std::endl;
            peer.send(pong);
        });

        // Unlike futures, actors don't (yet, I'm thinking about it!) have a built-in mechanism
        // that would report the fact that actor died. We're going to do it manually.
        handleKill = fiberize::kill.bind([main] () {
            main.send(killed);
        });
    }
};

Event<void> Bob::killed;

int main() {
    // The FiberSystem by default will create an OS thread for each CPU core we have.
    // After initializing the system, we fiberize the current thread. This means it will
    // be able to start and communicate with fibers.
    FiberSystem system;
    FiberRef main = system.fiberize();

    // A task can be created from any function, lambda or function object. FiberSystem has three
    // function: fiber(), future() and actor() that are used to create tasks. These functions
    // return a Builder, which configures and starts the task.
    auto bobRef = system.actor(Bob{}).run(main);
    auto aliceRef = system.future(alice).run(bobRef);

    // Let them run for a few seconds.
    io::sleep(5s);

    // Kill the tasks. The kill() function sends the special kill event which will trigger
    // the Killed exception inside the fiber/future.
    bobRef.kill();
    aliceRef.kill();

    // Wait until both tasks finish.
    Bob::killed.await();
    std::cout << "Bob is dead." << std::endl;
    aliceRef.await();
    std::cout << "Alice is dead." << std::endl;

    return 0;
}
