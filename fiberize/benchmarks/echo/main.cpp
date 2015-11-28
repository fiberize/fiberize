#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const uint n = 16;

Event<FiberRef> ping;
Event<void> pong;

void echo() {
    while (true) {
        ping.await().send(pong);
    }
}

void emitter(FiberRef echo, int initialMessages, int repeat) {
    using namespace context;

    int sent = 0;
    int received = 0;

    while (sent < initialMessages) {
        echo.send(ping, self());
        sent += 1;
    }

    while (received < repeat) {
        pong.await();
        received += 1;

        if (sent < repeat) {
            echo.send(ping, self());
            sent += 1;
        }
    }
}

int main() {
    FiberSystem system;
    system.fiberize();

    std::vector<FutureRef<void>> emitters;

    for (uint i = 0; i < n; ++i) {
        auto echoRef = system.fiber(echo).run();
        auto emitterRef = system.future(emitter).run(echoRef, 100, 5000000);
        emitters.push_back(emitterRef);
    }

    for (FutureRef<void>& emitter : emitters)
        emitter.await();
    return 0;
}
