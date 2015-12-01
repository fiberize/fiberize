#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const uint n = 10000;

Event<FiberRef> ping;
Event<void> pong;
Event<void> finished;

struct Echo {
    HandlerRef handlePing;

    void operator () () {
        handlePing = ping.bind([] (const FiberRef& sender) {
            sender.send(pong);
        });
    }
};

struct Emitter {
    Emitter(FiberRef main,  int initialMessages, int repeat)
        : main(main), initialMessages(initialMessages), repeat(repeat) {}

    FiberRef main;
    int initialMessages;
    int repeat;

    int sent = 0;
    int received = 0;

    HandlerRef handlePong;

    void operator () (FiberRef echo) {
        FiberRef self = context::self();

        while (sent < initialMessages) {
            echo.send(ping, self);
            sent += 1;
        }

        handlePong = pong.bind([=] () {
            received += 1;

            if (sent < repeat) {
                echo.send(ping, self);
                sent += 1;
            }

            if (received >= repeat) {
                echo.kill();
                main.send(finished);
                context::stop();
            }
        });
    }
};

int main() {
    FiberSystem system;
    FiberRef self = system.fiberize();

    auto emitter = system.actor(Emitter{self, 100, 100000});
    for (uint i = 0; i < n; ++i) {
        auto echoRef = system.actor(Echo{}).run();
        emitter.copy().run_(echoRef);
    }

    for (uint i = 0; i < n; ++i)
        finished.await();
    return 0;
}
