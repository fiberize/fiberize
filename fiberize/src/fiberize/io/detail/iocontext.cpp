#include <fiberize/io/detail/iocontext.hpp>
#include <fiberize/event.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/event-inl.hpp>
#include <fiberize/fiberref-inl.hpp>
#include <fiberize/fibersystem.hpp>
#include <fiberize/builder-inl.hpp>

#include <chrono>
#include <thread>

using namespace std::literals;

namespace fiberize {
namespace io {
namespace detail {

/**
 * 10 milliseconds.
 */
const uint64_t timeInterval = 1000 * 1000 * 10;

IOContext::IOContext() {
    lastRun = 0;
    uv_loop_init(loop());
}

IOContext::~IOContext() {
    uv_loop_close(loop());
}

bool IOContext::poll() {
    lastRun = uv_hrtime_fast();
    return uv_run(loop(), UV_RUN_NOWAIT);
}

void IOContext::throttledPoll() {
    uint64_t time = uv_hrtime_fast();
    if (time - lastRun > timeInterval) {
        lastRun = time;
        uv_run(loop(), UV_RUN_NOWAIT);
    }
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
