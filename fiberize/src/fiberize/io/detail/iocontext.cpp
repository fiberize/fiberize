#include <fiberize/io/detail/iocontext.hpp>
#include <fiberize/event.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/event-inl.hpp>
#include <fiberize/fiberref-inl.hpp>
#include <fiberize/future.hpp>
#include <fiberize/fibersystem.hpp>
#include <fiberize/builder-inl.hpp>

#include <chrono>
#include <thread>

using namespace std::literals;

namespace fiberize {
namespace io {
namespace detail {

class Dispatcher : public Fiber {
public:
    Dispatcher(uv_loop_t* loop)
        : loop(loop) {}

    void run() override {
        uint64_t k = 0;
        for (;;++k) {
            if (k % 100 == 0)
                uv_run(loop, UV_RUN_NOWAIT);
            yield();
            process();
        }
    }

private:
    uv_loop_t* loop;
};

IOContext::IOContext() {
    uv_loop_init(loop());
}

IOContext::~IOContext() {
    uv_loop_close(loop());
}

void IOContext::startDispatcher() {
    dispatcher = Scheduler::current()->system()->fiber<Dispatcher>().bound().run(loop());
}

void IOContext::stopDispatcher() {
    dispatcher.kill();
    dispatcher = FiberRef();
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
