#include <fiberize/io/sleep.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/context.hpp>
#include <fiberize/io/detail/libuvwrapper.hpp>
#include <fiberize/scopedpin.hpp>

#include <uv.h>

namespace fiberize {
namespace io {

template <>
void millisleep<Block>(const std::chrono::milliseconds& duration) {
    std::this_thread::sleep_for(duration);
}

template <>
void millisleep<Await>(const std::chrono::milliseconds& duration) {
    using Env = detail::AwaitHandleEnv<uv_timer_t>;
    uv_loop_t* loop = Scheduler::current()->ioContext().loop();
    ScopedPin pin;

    /**
     * Create the environment for the request. The environment cannot be located on the stack, because
     * some event handler could throw an exception before the callback is complete.
     */
    boost::intrusive_ptr<Env> env(new Env);

    /**
     * Initialize the timer.
     */
    int code = uv_timer_init(loop, &env->handle);
    if (code < 0) {
        throw std::system_error(-code, std::system_category());
    }

    /**
     * Grab a reference for the callback and start the timer.
     */
    env->grab();
    code = uv_timer_start(&env->handle, Env::callback, duration.count(), std::numeric_limits<uint64_t>::max());
    if (code < 0) {
        env->drop();
        throw std::system_error(-code, std::system_category());
    }

    /**
     * Wait until the callback fires.
     */
    context::processUntil(env->condition);
}

static void noResult(uv_timer_t*) {}

template <>
Event<Result<void>> millisleep<Async>(const std::chrono::milliseconds& duration) {
    using Env = detail::AsyncHandleEnv<void, uv_timer_t, noResult>;
    uv_loop_t* loop = Scheduler::current()->ioContext().loop();
    ScopedPin pin;

    /**
     * Create the environment.
     */
    boost::intrusive_ptr<Env> env(new Env);

    /**
     * Initialize the timer.
     */
    int code = uv_timer_init(loop, &env->handle);
    if (code < 0) {
        throw std::system_error(-code, std::system_category());
    }

    /**
     * Grab a reference for the callback and start the timer.
     */
    env->grab();
    code = uv_timer_start(&env->handle, Env::callback, duration.count(), std::numeric_limits<uint64_t>::max());
    if (code < 0) {
        env->drop();
        throw std::system_error(-code, std::system_category());
    }

    /**
     * Returns the event.
     */
    return env->event;
}

} // namespace io
} // namespace fiberize
