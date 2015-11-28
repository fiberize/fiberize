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

// Fibers can be defined as function objects, possibly with some methods and state.
struct Bob {
    void operator () () {
        // Perform the handshake.
        FiberRef peer = hello.await();
        peer.send(ack);

        // Enter the loop.
        while (true) {
            ping.await();
            io::sleep(500ms);
            std::cout << "Pong" << std::endl;
            peer.send(pong);
        }
    }
};

int main() {
    // The FiberSystem by default will create an OS thread for each CPU core we have.
    // After initializing the system, we fiberize the current thread. This means it will
    // be able to start and communicate with fibers.
    FiberSystem system;
    system.fiberize();

    // A fiber/future can be created from any function, lambda or function object. The future()
    // function returns a Builder, which is used to configure the future and start it. The only
    // difference between a fiber and a future is that we can await a future to get it's result.
    auto bobRef = system.future(Bob{}).run();
    auto aliceRef = system.future(alice).run(bobRef);

    // Let them run for a few seconds.
    io::sleep(5s);

    // Kill the futures. The kill() function sends a special event which will trigger
    // the Killed exception inside the fiber/future.
    bobRef.kill();
    aliceRef.kill();

    // Wait until both futures finish. The Killed exception is propagated throught
    // the result, so we have to catch it.
    try {
        bobRef.result()->await();
    } catch (Killed&) {
        std::cout << "Bob is dead." << std::endl;
    }
    try {
        aliceRef.result()->await();
    } catch (Killed&) {
        std::cout << "Alice is dead." << std::endl;
    }

    return 0;
}
```

Building
========

The easiest way to build the library is to use docker:
```bash
> docker build github.com/fiberize/fiberize.git
```

Dependencies you need if you're not using docker are boost, google test, cmake, automake and libtool. Once you have them run:
```bash
> git clone github.com/fiberize/fiberize.git
> cd libuv 
> sh autogen.sh
> ./configure --prefix=/usr
> make && make check
> sudo make install
> cd ../
> mkdir fiberize/build
> cd fiberize/build
> cmake .. && make
> sudo make install
```
This will install both libuv (into /usr) and fiberize (into /usr/local). Warning: this will overwrite your libuv version. The bundled libuv version differs slightly from mainstream, but should be compatible.

To link with the library add -lfiberize to c++ compiler options.

Performance
===========

fiberize is built for performance. Currently, when running on a 4 core Intel i7-4702MQ it can:
* process ~12 million fibers per second [(fiberize/benchmarks/fps/main.cpp)](fiberize/benchmarks/fps/main.cpp),
* send ~8 million messages per second [(fiberize/benchmarks/echo/main.cpp)](fiberize/benchmarks/echo/main.cpp),
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
