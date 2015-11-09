#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

Event<FiberRef> ping("ping");
Event<Unit> pong("pong");

struct Echo : public Fiber<Void> {
    Void run() {
        while (true) {
            FiberRef sender = ping.await();
            sender.emit(pong);
        }
    }
};

struct Emitter : public Fiber<Unit> {
    Emitter(const FiberRef& echo, int initialMessages, int repeat)
        : echo(echo), initialMessages(initialMessages), repeat(repeat), sent(0), received(0)
        {}
    
    FiberRef echo;
    const int initialMessages;
    const int repeat;
    
    int sent;
    int received;
    
    Unit run() {
        while (sent < initialMessages) {
            echo.emit(ping, self());
            sent += 1;
        }
        
        while (received < repeat) {
            pong.await();
            received += 1;
            
            if (sent < repeat) {
                echo.emit(ping, self());
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
    return 0;
}
