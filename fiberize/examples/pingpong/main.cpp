#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

// First we declare some events. The default constructor assigns a locally
// unique id to the event. Events can have attached values.
Event<FiberRef> init; // Initializes the fiber, giving it a reference to its peer.
Event<void> ready;    // Reports back to the main thread that we are ready and waiting for the first ping.

Event<void> ping;
Event<void> pong;

// To create a fiber we derive from the Fiber class and implement the run function.
struct Ping : public Fiber {
    void run() override {
        // init.await() will "block" until the current fiber receives an init message and
        // then return the value attached to this event.
        auto peer = init.await();

        while (true) {
            std::cout << "Ping" << std::endl;
            // peer.send(event, attachedValue) sends an event to the fiber referenced by "peer"
            peer.send(ping);
            pong.await();
        }
    }
};

struct Pong : public Fiber {
    Pong(FiberRef mainFiber) : mainFiber(mainFiber) {}
    FiberRef mainFiber;

    void run() override {
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
    // The FiberSystem by default will create an OS thread for each CPU core we have.
    // After initializing the system, we fiberize the current thread. This means it will
    // be able to communicate with real fibers.
    FiberSystem system;
    FiberRef self = system.fiberize();

    // We create the fibers. Any parameters passed to run will be forwarded to the constructor.
    FiberRef ping = system.fiber<Ping>().run();
    FiberRef pong = system.fiber<Pong>().run(self);

    // Exchange the fiber refs.
    pong.send(init, ping);
    ready.await(); // Awaiting in a fiberized thread continues to process events.
    ping.send(init, pong);

    // Enter an infinite loop processing events.
    EventContext::current()->processForever();
}
