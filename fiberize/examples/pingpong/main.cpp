#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

Event<FiberRef> init("init");
Event<Unit> ready("ready");

Event<Unit> ping("ping");
Event<Unit> pong("pong");

struct Ping : public Fiber<Unit> {
    virtual Unit run() {
        auto peer = await(init);
        
        while (true) {
            std::cout << "Ping" << std::endl;
            peer.emit(ping);
            await(pong);
        }
    }
};

struct Pong : public Fiber<Unit> {
    virtual Unit run() {
        auto peer = await(init);
        system()->mainFiber().emit(ready);

        while (true) {
            await(ping);
            std::cout << "Pong" << std::endl;
            peer.emit(pong);
        }
    }
};

int main() {
    System system;
    
    auto ping = system.runNamed<Ping>("ping");
    auto pong = system.runNamed<Pong>("pong");
    
    pong.emit(init, ping);
    ready.await(system.mainContext());
    ping.emit(init, pong);
    
    system.mainContext()->processForever();
}
