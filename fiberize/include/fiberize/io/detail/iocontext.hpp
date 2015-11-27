#ifndef FIBERIZE_IO_DETAIL_IOCONTEXT_HPP
#define FIBERIZE_IO_DETAIL_IOCONTEXT_HPP

#include <uv.h>

#include <fiberize/fiberref.hpp>

namespace fiberize {
namespace io {
namespace detail {

class IOContext {
public:
    IOContext();
    ~IOContext();

    /**
     * Run the event loop once, exiting immediately if there are no events.
     * @returns whether any event was processed.
     */
    bool poll();

    /**
     * Run the event loop, not more often then some predefined time.
     */
    void throttledPoll();

    /**
     * Run the event loop in the current thread.
     */
    void runLoop();

    /**
     * Run the event loop once, exiting immediately if there are no events.
     */
    void runLoopNoWait();

    /**
     * Stops the running event loop.
     */
    void stopLoop();

    /**
     * Returns the libuv loop associated with this IO context.
     */
    uv_loop_t* loop();

private:
    uv_loop_t loop_;
    bool stopped;
    uint64_t lastRun;
};

} // namespace detail
} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_DETAIL_IOCONTEXT_HPP
