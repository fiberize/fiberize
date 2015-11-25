#include <fiberize/io/detail/iocontext.hpp>
#include <fiberize/fibersystem.hpp>
#include <fiberize/future.hpp>

#include <chrono>
#include <thread>

using namespace std::literals;

namespace fiberize {
namespace io {
namespace detail {

class Dispatcher : public Future<Unit> {
public:
    Dispatcher(uv_loop_t* loop)
        : loop(loop) {}

    Unit run() override {
        while (true) {
            uv_run(loop, UV_RUN_NOWAIT);
            yield();
        }
    }

private:
    uv_loop_t* loop;
};

IOContext::IOContext() {
    uv_loop_init(&loop());
}

IOContext::~IOContext() {
    // TODO: stop the dispatcher if it's running
    uv_loop_close(&loop());
}

void IOContext::startDispatcher() {
    Scheduler::current()->system()->run<Dispatcher>(&loop());
}

void IOContext::stopDispatcher() {
    // TODO: kill the fiber
}

void IOContext::runLoop() {
    stopped = false;
    while (!stopped) {
        uv_run(&loop(), UV_RUN_DEFAULT);
        std::this_thread::sleep_for(1ms);
    }
}

void IOContext::runLoopNoWait() {
    uv_run(&loop(), UV_RUN_NOWAIT);
}

void IOContext::stopLoop() {
    stopped = true;
    uv_stop(&loop());
}

uv_loop_t& IOContext::loop() {
    return loop_;
}

} // namespace detail
} // namespace io
} // namespace fiberize
