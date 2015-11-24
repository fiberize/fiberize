#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const uint n = 16;

Event<FiberRef> ping;
Event<Unit> pong;

struct Echo : public Fiber {
    void run() override {
        while (true) {
            FiberRef sender = ping.await();
            sender.send(pong);
        }
    }
};

struct Emitter : public Future<Unit> {
    Emitter(const FiberRef& echo, int initialMessages, int repeat)
        : echo(echo), initialMessages(initialMessages), repeat(repeat), sent(0), received(0)
        {}
    
    FiberRef echo;
    const int initialMessages;
    const int repeat;
    
    int sent;
    int received;
    
    Unit run() override {
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

        return {};
    }
};

int main() {
    FiberSystem system;
    system.fiberize();

    std::vector<FutureRef<Unit>> emitters;

    for (uint i = 0; i < n; ++i) {
        auto echo = system.run<Echo>();
        auto emitter = system.run<Emitter>(echo, 100, 100000);
        emitters.push_back(emitter);
    }

    for (FutureRef<Unit>& emitter : emitters)
        emitter.result()->await();
    return 0;
}
