#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

Event<AnyFiberRef> ping("ping");
Event<Unit> pong("pong");

struct Echo : public Fiber<Void> {
    Void run() override {
        while (true) {
            AnyFiberRef sender = ping.await();
            sender.send(pong);
        }
    }
};

struct Emitter : public Fiber<Unit> {
    Emitter(const AnyFiberRef& echo, int initialMessages, int repeat)
        : echo(echo), initialMessages(initialMessages), repeat(repeat), sent(0), received(0)
        {}
    
    AnyFiberRef echo;
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
    System system;
    system.fiberize();
    auto echo = system.run<Echo>();
    auto emitter = system.run<Emitter>(echo, 100, 1000000);
    emitter.finished().await();
    exit(0);
}
