fiberize 
========

![Build status](https://travis-ci.org/fiberize/fiberize.svg?branch=master)

Fiberize is a C++ framework for high performance parallel (and in the future distributed) computing. It implements an user space scheduler integrated with an evented IO system (based on node.js's libuv). There are four concurrency abstractions:
* fibers - lightweight threads implemented entirely in user space,
* futures - computations that eventually return some value, executed asynchronously,
* actors (wip) - even more lightweight then fibers, but less flexible,
* threads (wip) - full blown OS threads, when you want to run a continous process, e.g. OpenGL rendering or a multiplayer game server.

Fibers in fiberize can communicate by sending and receiving events. When a fiber waits for an event or performs an IO operation it doesn't block the OS thread it was running on - instead the execution switches to another fiber. This means that you can write asynchronous and nonblocking code as easly as you would write a synchronous version (think es7 await/async, but faster and multithreaded ;)

Example
=======

The following example (examples/pingpong/main.cpp) starts two fibers that play ping pong with events.

``` C++
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
    FiberRef ping = system.run<Ping>();
    FiberRef pong = system.run<Pong>(self);

    // Exchange the fiber refs.
    pong.send(init, ping);
    ready.await(); // Awaiting in a fiberized thread (and not a real fiber) *blocks*.
    ping.send(init, pong);

    // Enter an infinite loop processing events.
    EventContext::current()->processForever();
}
```

Building
========

The easiest way to build the library is to use docker:
```bash
> docker build github.com/fiberize/fiberize.git
```

Dependencies you need if you're not using docker are boost, google test and cmake. Once you have them run:
```bash
> git clone github.com/fiberize/fiberize.git
> mkdir fiberize/build
> cd fiberize/build
> cmake .. && make
optionally> sudo make install
```

To link with the library add -lfiberize to c++ compiler options.

Performance
===========

fiberize is built for performance. Currently, when running on a 4 core Intel i7-4702MQ it can:
* process ~10 million fibers per second [(fiberize/benchmarks/fps/main.cpp)](fiberize/benchmarks/fps/main.cpp),
* send ~6.8 million messages per second [(fiberize/benchmarks/echo/main.cpp)](fiberize/benchmarks/echo/main.cpp),
* run ~100000 fibers simultaneously per 1GB of RAM [(fiberize/benchmarks/sleepers/main.cpp)](fiberize/benchmarks/sleepers/main.cpp).

IO and http benchmarks will come when the respective components are done.

TODO list and ideas
=========

The project is pretty new and has a long and ambitious todo list :)

* complete the low-level IO library
* http library
* high-level stream based IO library
* remoting tunnels - the first step to cluster support is to establish a tunnel between two systems, that allows them to lookup remote fibers and send events. It should be possible to link two tunnels, creating a relay node. Of course this requires serialization: protocol buffers and JSON/BSON are what I'm currently looking at.
* clustering - the idea is to use existing p2p and DHT implementations to create a cluster management layer. The job of the management layer is to maintain a mapping from system UUIDs to lists of IP addresses/ports, global fiber paths to system UUIDs, a list of relay nodes, etc. Using this information remoting tunnels can be established, possibly routing around NATs using a technique like [ICE](https://tools.ietf.org/html/rfc5245) and the relay nodes.

Bibliography
==========

* Blumofe, Robert D., and Charles E. Leiserson. "Scheduling multithreaded computations by work stealing." Journal of the ACM (JACM) 46.5 (1999): 720-748.
* Voellmy, Andreas Richard, et al. "Mio: a high-performance multicore io manager for ghc." ACM SIGPLAN Notices 48.12 (2014): 129-140.
* http://lxr.free-electrons.com/source/fs/eventpoll.c
* Han, Sangjin, et al. "MegaPipe: A New Programming Interface for Scalable Network I/O." OSDI. 2012.
* Jeong, E., et al. "mTCP: a highly scalable user-level TCP stack for multicore systems." Proc. 11th USENIX NSDI (2014).
