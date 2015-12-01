/**
 * Per-scheduler IO event context.
 *
 * @file iocontext.hpp
 * @copyright 2015 Paweł Nowak
 */
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
     * Returns the libuv loop associated with this IO context.
     */
    uv_loop_t* loop();

private:
    uv_loop_t loop_;
    uint64_t lastRun;
};

} // namespace detail
} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_DETAIL_IOCONTEXT_HPP
