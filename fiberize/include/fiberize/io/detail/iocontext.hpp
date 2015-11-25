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
     * Starts the dispatcher fiber.
     */
    void startDispatcher();

    /**
     * Stops the dispatcher fiber.
     */
    void stopDispatcher();

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
    FutureRef<Unit> dispatcher;
};

} // namespace detail
} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_DETAIL_IOCONTEXT_HPP
