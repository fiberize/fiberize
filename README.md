fiberize 
========

![Build status](https://travis-ci.org/fiberize/fiberize.svg?branch=master)

Fiberize is a C++ framework for high performance parallel (and in the future distributed) computing. It implements an user space scheduler integrated with an evented IO system (based on node.js's libuv). There are three concurrency abstractions:
* fibers - lightweight threads, well suited for sequential tasks,
* futures - computations that eventually return some value,
* actors - objects that respond to messages, after an initialization phase.

Any of them can be run in a thread pool using the user space scheduler (the default) or standalone as an OS thread.

Tasks in fiberize can communicate by sending and receiving events. When a fiber, future or actor waits for an event or performs an IO operation it doesn't block the OS thread it was running on - instead the execution switches to another task. This means that you can write asynchronous and nonblocking code as easly as you would write a synchronous version (think es7 await/async, but faster and multithreaded ;)

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

// Bob will be an actor. Actors begin their life in the same way as fibers, but after the
// initial function exits they don't die - instead they start processing messages.
// That message processing phase is very efficient, as an actor waiting for a message
// does not need a stack.
struct Bob {
    static Event<void> killed;

    HandlerRef handlePing;
    HandlerRef handleKill;

    void operator () (FiberRef main) {
        // Perform the handshake.
        FiberRef peer = hello.await();
        peer.send(ack);

        // Bind a handler that will remain once we exit this loop.
        handlePing = ping.bind([peer] () {
            io::sleep(500ms);
            std::cout << "Pong" << std::endl;
            peer.send(pong);
        });

        // Unlike futures, actors don't (yet, I'm thinking about it!) have a built-in mechanism
        // that would report the fact that actor died. We're going to do it manually.
        handleKill = fiberize::kill.bind([main] () {
            main.send(killed);
        });
    }
};

Event<void> Bob::killed;

int main() {
    // The FiberSystem by default will create an OS thread for each CPU core we have.
    // After initializing the system, we fiberize the current thread. This means it will
    // be able to start and communicate with fibers.
    FiberSystem system;
    FiberRef main = system.fiberize();

    // A task can be created from any function, lambda or function object. FiberSystem has three
    // function: fiber(), future() and actor() that are used to create tasks. These functions
    // return a Builder, which configures and starts the task.
    auto bobRef = system.actor(Bob{}).run(main);
    auto aliceRef = system.future(alice).run(bobRef);

    // Let them run for a few seconds.
    io::sleep(5s);

    // Kill the tasks. The kill() function sends the special kill event which will trigger
    // the Killed exception inside the fiber/future.
    bobRef.kill();
    aliceRef.kill();

    // Wait until both tasks finish.
    Bob::killed.await();
    std::cout << "Bob is dead." << std::endl;
    aliceRef.await();
    std::cout << "Alice is dead." << std::endl;

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
* process ~16 million fibers per second [(fiberize/benchmarks/fps/main.cpp)](fiberize/benchmarks/fps/main.cpp),
* send ~26 million messages per second [(fiberize/benchmarks/echo/main.cpp)](fiberize/benchmarks/echo/main.cpp),
* run ~100000 fibers simultaneously per 1GB of RAM [(fiberize/benchmarks/sleepers/main.cpp)](fiberize/benchmarks/sleepers/main.cpp).

IO and http benchmarks will come when the respective components are done.

Bibliography
==========

* Blumofe, Robert D., and Charles E. Leiserson. "Scheduling multithreaded computations by work stealing." Journal of the ACM (JACM) 46.5 (1999): 720-748.
* Voellmy, Andreas Richard, et al. "Mio: a high-performance multicore io manager for ghc." ACM SIGPLAN Notices 48.12 (2014): 129-140.
* http://lxr.free-electrons.com/source/fs/eventpoll.c
* Han, Sangjin, et al. "MegaPipe: A New Programming Interface for Scalable Network I/O." OSDI. 2012.
* Jeong, E., et al. "mTCP: a highly scalable user-level TCP stack for multicore systems." Proc. 11th USENIX NSDI (2014).
