#include <fiberize/io/detail/iocontext.hpp>
#include <fiberize/event.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/event-inl.hpp>
#include <fiberize/fiberref-inl.hpp>
#include <fiberize/future.hpp>
#include <fiberize/fibersystem.hpp>

#include <chrono>
#include <thread>

using namespace std::literals;

namespace fiberize {
namespace io {
namespace detail {

class Dispatcher : public Future<void> {
public:
    static Event<void> kill;

    Dispatcher(uv_loop_t* loop)
        : loop(loop), killed(false) {}

    void run() override {
        auto _kill = kill.bind([&] () {
            killed = true;
        });

        while (!killed) {
            uv_run(loop, UV_RUN_NOWAIT);
            yield();
            process();
        }
    }

private:
    uv_loop_t* loop;
    bool killed;
};

Event<void> Dispatcher::kill;

IOContext::IOContext() {
    uv_loop_init(loop());
}

IOContext::~IOContext() {
    uv_loop_close(loop());
}

void IOContext::startDispatcher() {
    dispatcher = Scheduler::current()->system()->run<Dispatcher>(loop());
}

void IOContext::stopDispatcher() {
    dispatcher.send(Dispatcher::kill);
    dispatcher = FutureRef<void>();
}

void IOContext::runLoop() {
    stopped = false;
    while (!stopped) {
        uv_run(loop(), UV_RUN_DEFAULT);
        std::this_thread::sleep_for(1ms);
    }
}

void IOContext::runLoopNoWait() {
    uv_run(loop(), UV_RUN_NOWAIT);
}

void IOContext::stopLoop() {
    stopped = true;
    uv_stop(loop());
}

uv_loop_t* IOContext::loop() {
    return &loop_;
}

} // namespace detail
} // namespace io
} // namespace fiberize
