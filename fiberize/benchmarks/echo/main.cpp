#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

const uint n = 16;

Event<FiberRef> ping;
Event<void> pong;

struct Echo : public Fiber {
    void run() override {
        while (true) {
            ping.await().send(pong);
        }
    }
};

struct Emitter : public Future<void> {
    Emitter(const FiberRef& echo, int initialMessages, int repeat)
        : echo(echo), initialMessages(initialMessages), repeat(repeat), sent(0), received(0)
        {}
    
    FiberRef echo;
    const int initialMessages;
    const int repeat;
    
    int sent;
    int received;
    
    void run() override {
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
};

int main() {
    FiberSystem system;
    system.fiberize();

    std::vector<FutureRef<void>> emitters;

    for (uint i = 0; i < n; ++i) {
        auto echo = system.fiber<Echo>().run();
        auto emitter = system.future<Emitter>().run(echo, 100, 5000000);
        emitters.push_back(emitter);
    }

    for (FutureRef<void>& emitter : emitters)
        emitter.result()->await();
    return 0;
}
