#include <fiberize/io/detail/iocontext.hpp>
#include <fiberize/fibersystem.hpp>
#include <fiberize/future.hpp>
#include <fiberize/eventimpl.hpp>

#include <chrono>
#include <thread>

using namespace std::literals;

namespace fiberize {
namespace io {
namespace detail {

class Dispatcher : public Future<Unit> {
public:
    static Event<Unit> kill;

    Dispatcher(uv_loop_t* loop)
        : loop(loop), killed(false) {}

    Unit run() override {
        auto _kill = kill.bind([&] (const Unit&) {
            killed = true;
        });

        while (!killed) {
            uv_run(loop, UV_RUN_NOWAIT);
            yield();
            process();
        }

        return {};
    }

private:
    uv_loop_t* loop;
    bool killed;
};

Event<Unit> Dispatcher::kill;

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
    dispatcher = FutureRef<Unit>();
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
