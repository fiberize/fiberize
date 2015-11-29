/**
 * The libuv wrapper template and helper macros.
 *
 * @file libuvwrapper.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_IO_DETAIL_LIBUVWRAPPER_HPP
#define FIBERIZE_IO_DETAIL_LIBUVWRAPPER_HPP

#include <fiberize/scheduler.hpp>
#include <fiberize/context.hpp>
#include <fiberize/io/mode.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/refrencecounted.hpp>
#include <fiberize/context.hpp>
#include <fiberize/scopedpin.hpp>

#include <system_error>

#include <boost/preprocessor.hpp>
#include <boost/intrusive_ptr.hpp>

namespace fiberize {
namespace io {
namespace detail {

/**
 * Temporarily swaps the current scheduler.
 */
struct SwapScheduler {
    SwapScheduler(Scheduler* newScheduler) {
        oldScheduler = Scheduler::current();
        newScheduler->makeCurrent();
    }

    ~SwapScheduler() {
        if (oldScheduler) {
            oldScheduler->makeCurrent();
        } else {
            Scheduler::resetCurrent();
        }
    }

    Scheduler* oldScheduler;
};

template <typename Value, typename Request, Value (*extractor)(Request*)>
struct ExtractAndSend {
    static void execute(const FiberRef& self, const Event<Result<Value>>& event, Request* req) {
        self.send(event, extractor(req));
    }
};

template <typename Request, void (*extractor)(Request*)>
struct ExtractAndSend<void, Request, extractor> {
    static void execute(const FiberRef& self, const Event<Result<void>>& event, Request*) {
        self.send(event);
    }
};

/**
 * Closure enviroment used for Await mode operations.
 */
template <typename Request, void (*cleanup)(Request*)>
struct AwaitRequestEnv : public fiberize::detail::ReferenceCountedAtomic {
    AwaitRequestEnv() {
        request.data = this;
        condition = false;
        dirty = false;
        scheduler = Scheduler::current();
        block = scheduler->currentControlBlock();
        block->grab();
    }

    virtual ~AwaitRequestEnv() {
        block->drop();
        if (dirty)
            cleanup(&request);
    }


    void completed() {
        /**
         * Set the condition to true and reschedule the fiber, if necesssary.
         */
        boost::unique_lock<fiberize::detail::ControlBlockMutex> lock(block->mutex);
        condition = true;
        if (block->status == fiberize::detail::Suspended) {
            /// @todo what if the scheduler is destroyed? This isn't a big problem right now, because it only
            ///       happens after shutdown. This should be fixed for the graceful shutdown patch.
            scheduler->enable(block, std::move(lock));
        }
    }

    static void callback(Request* req) {
        auto ctx = reinterpret_cast<AwaitRequestEnv*>(req->data);
        ctx->completed();
        ctx->drop();
    }

    Request request;
    bool condition;
    bool dirty;
    fiberize::detail::ControlBlock* block;
    Scheduler* scheduler;
};

template <typename Handle>
struct AwaitHandleEnv : public fiberize::detail::ReferenceCounted {
    AwaitHandleEnv() {
        handle.data = this;
        condition = false;
        scheduler = Scheduler::current();
        block = scheduler->currentControlBlock();
        block->grab();
    }

    virtual ~AwaitHandleEnv() {
        block->drop();
    }

    void completed() {
        /**
         * Set the condition to true and reschedule the fiber, if necesssary.
         */
        boost::unique_lock<fiberize::detail::ControlBlockMutex> lock(block->mutex);
        condition = true;
        if (block->status == fiberize::detail::Suspended) {
            /// @todo what if the scheduler is destroyed? This isn't a big problem right now, because it only
            ///       happens after shutdown. This should be fixed for the graceful shutdown patch.
            scheduler->enable(block, std::move(lock));
        }
    }

    static void callback(Handle* handle) {
        auto ctx = reinterpret_cast<AwaitHandleEnv*>(handle->data);
        ctx->completed();
        uv_close(reinterpret_cast<uv_handle_t*>(handle), [] (uv_handle_t* handle) {
            reinterpret_cast<AwaitHandleEnv*>(handle->data)->drop();
        });
    }

    Handle handle;
    bool condition;
    fiberize::detail::ControlBlock* block;
    Scheduler* scheduler;
};

/**
 * Environment for blocking mode. All it does is handle the cleanup.
 */
template <typename Request, void (*cleanup)(Request*)>
struct BlockRequestEnv {
    BlockRequestEnv() {
        dirty = false;
    }

    ~BlockRequestEnv() {
        if (dirty)
            cleanup(&request);
    }

    bool dirty;
    Request request;
};

template <typename Handle>
struct BlockHandleEnv {
    BlockHandleEnv() {
        handle.data = this;
    }

    void cleanup() {
        uv_close(reinterpret_cast<uv_handle_t*>(handle), [] (uv_handle_t* handle) {
            delete reinterpret_cast<BlockHandleEnv*>(handle->data);
        });
    }

    Handle handle;
};

/**
 * Closure environment for the Async mode.
 */
template <typename Value, typename Request, void (*cleanup)(Request*), Value (*extractor)(Request*)>
struct AsyncRequestEnv : public fiberize::detail::ReferenceCountedAtomic {
    AsyncRequestEnv() {
        request.data = this;
        dirty = false;
        self = context::self();
        scheduler = context::scheduler();
    }

    virtual ~AsyncRequestEnv() {
        if (dirty)
            cleanup(&request);
    }

    void completed() {
        /**
         * Temporarily swap the current scheduler.
         * @todo what if the scheduler is destroyed?
         */
        SwapScheduler swapScheduler(scheduler);

        /**
         * Send the result as an event.
         */
        if (request.result >= 0) {
            ExtractAndSend<Value, Request, extractor>::execute(self, event, &request);
        } else {
            self.send(event, std::make_exception_ptr(
                std::system_error(-request.result, std::system_category())
            ));
        }
    }

    static void callback(Request* req) {
        auto ctx = reinterpret_cast<AsyncRequestEnv*>(req->data);
        ctx->completed();
        ctx->drop();
    }

    Request request;
    bool dirty;
    Event<Result<Value>> event;
    FiberRef self;
    Scheduler* scheduler;
};

template <typename Value, typename Handle, Value (*extractor)(Handle*)>
struct AsyncHandleEnv : public fiberize::detail::ReferenceCounted {
     AsyncHandleEnv() {
        handle.data = this;
        self = context::self();
        scheduler = context::scheduler();
    }

    void completed() {
        /**
         * Temporarily swap the current scheduler.
         * @todo what if the scheduler is destroyed?
         */
        SwapScheduler swapScheduler(scheduler);

        /**
         * Send the result as an event.
         */
        ExtractAndSend<Value, Handle, extractor>::execute(self, event, &handle);
    }

    static void callback(Handle* handle) {
        auto ctx = reinterpret_cast<AsyncHandleEnv*>(handle->data);
        ctx->completed();
        uv_close(reinterpret_cast<uv_handle_t*>(handle), [] (uv_handle_t* handle) {
            reinterpret_cast<AsyncHandleEnv*>(handle->data)->drop();
        });
    }

    Handle handle;
    Event<Result<Value>> event;
    FiberRef self;
    Scheduler* scheduler;
};

/**
 * Implements generic libuv wrappers for all IO modes. The parameters are:
 * @tparam Value Type of the result.
 * @tparam Request Type of the request.
 * @tparam cleanup Function used to cleanup the request.
 * @tparam UVFunctionType Type of the libuv function we are wrapping.
 * @tparam uvfunction The wrapped function.
 * @tparam extractor Function used to extract the result from a completed request.
 */
template <typename Value, typename Request, void (*cleanup)(Request*), typename UVFunctionType, UVFunctionType uvfunction, Value (*extractor)(Request*)>
struct LibUVWrapper {
    /**
     * Executes the wrapped function in the given IO mode. The arguments are inserted into the call
     * after the pointer to the request structure.
     */
    template <typename Mode, typename... Args>
    static IOResult<Value, Mode> execute(Args&&... args) {
        return execute(Mode(), std::forward<Args>(args)...);
    }

    /**
     * Await mode wrapper.
     */
    template <typename... Args>
    static IOResult<Value, Await> execute(Await, Args&&... args) {
        using Env = AwaitRequestEnv<Request, cleanup>;
        ScopedPin pin;

        /**
         * Create the environment for the request. The environment cannot be located on the stack, because
         * some event handler could throw an exception before the callback is complete.
         */
        boost::intrusive_ptr<Env> env(new Env);

        /**
         * Start the IO operation and grab a reference (for the callback) to prevent the environment
         * from being destroyed.
         */
        env->grab();
        int code = uvfunction(Scheduler::current()->ioContext().loop(),
            &env->request, std::forward<Args>(args)..., Env::callback);

        /**
         * The operation could fail instantly.
         */
        if (code < 0) {
            env->drop();
            throw std::system_error(-code, std::system_category());
        }

        /**
         * Process events until the callback completes. Mark the request as dirty.
         */
        env->dirty = true;
        context::processUntil(env->condition);

        /**
         * Request finished. Extract the result.
         */
        if (env->request.result >= 0) {
            return extractor(&env->request);
        } else {
            throw std::system_error(-env->request.result, std::system_category());
        }
    }

    template <typename... Args>
    static IOResult<Value, Block> execute(Block, Args&&... args) {
        using Env = BlockRequestEnv<Request, cleanup>;
        ScopedPin pin;

        /**
         * Create the request on the stack, as there are not async callbacks.
         */
        Env env;

        /**
         * Do the IO operation.
         */
        int code = uvfunction(Scheduler::current()->ioContext().loop(), &env.request, std::forward<Args>(args)..., nullptr);

        /**
         * There are two ways an error could be reported: the returned code or req.result. Check them.
         */
        if (code < 0) {
            throw std::system_error(-code, std::system_category());
        }

        /**
         * The request will require cleanup.
         */
        env.dirty = true;

        if (env.request.result >= 0) {
            return extractor(&env.request);
        } else {
            throw std::system_error(-env.request.result, std::system_category());
        }
    }

    template <typename... Args>
    static IOResult<Value, Async> execute(Async, Args&&... args) {
        using Env = AsyncRequestEnv<Value, Request, cleanup, extractor>;
        ScopedPin pin;

        /**
         * Create the environment.
         */
        boost::intrusive_ptr<Env> env(new Env);

        /**
         * Grab a reference for the callback and start the IO operation.
         */
        env->grab();
        int code = uvfunction(Scheduler::current()->ioContext().loop(),
            &env->request, std::forward<Args>(args)..., Env::callback);

        /**
         * The operation could fail instantly.
         */
        if (code < 0) {
            env->drop();
            throw std::system_error(-code, std::system_category());
        }

        /**
         * The request will require cleanup.
         */
        env->dirty = true;

        return env->event;
    }
};

// Ph'nglui mglw'nafh Cthulhu R'lyeh wgah'nagl fhtagn..

#define FIBERIZE_IO_DETAIL_REM(...) __VA_ARGS__
#define FIBERIZE_IO_DETAIL_EAT(...)

#define FIBERIZE_IO_DETAIL_STRIP(x) FIBERIZE_IO_DETAIL_EAT x
#define FIBERIZE_IO_DETAIL_PAIR(x) FIBERIZE_IO_DETAIL_REM x

#define FIBERIZE_IO_DETAIL_DEFINE_MEMBERS_EACH(r, data, x) FIBERIZE_IO_DETAIL_PAIR(x);
#define FIBERIZE_IO_DETAIL_DEFINE_ARGS_EACH(r, data, i, x) BOOST_PP_COMMA_IF(i) FIBERIZE_IO_DETAIL_PAIR(x)
#define FIBERIZE_IO_DETAIL_DEFINE_FORWARD_EACH(r, data, i, x) BOOST_PP_COMMA_IF(i) FIBERIZE_IO_DETAIL_STRIP(x)

#define FIBERIZE_IO_DETAIL_DEFINE_MEMBERS(args) BOOST_PP_SEQ_FOR_EACH(FIBERIZE_IO_DETAIL_DEFINE_MEMBERS_EACH, data, BOOST_PP_VARIADIC_TO_SEQ args)
#define FIBERIZE_IO_DETAIL_DEFINE_ARGS(args) BOOST_PP_SEQ_FOR_EACH_I(FIBERIZE_IO_DETAIL_DEFINE_ARGS_EACH, data, BOOST_PP_VARIADIC_TO_SEQ args)
#define FIBERIZE_IO_DETAIL_DEFINE_FORWARD(args) BOOST_PP_SEQ_FOR_EACH_I(FIBERIZE_IO_DETAIL_DEFINE_FORWARD_EACH, data, BOOST_PP_VARIADIC_TO_SEQ args)

#define FIBERIZE_IO_DETAIL_DEFINE_WRAPPER(Module, Name, Extractor, Value, Args)               \
    template <typename Mode>                                                                  \
    IOResult<Value, Mode> Name ( FIBERIZE_IO_DETAIL_DEFINE_ARGS(Args) ) {                     \
        return detail::LibUVWrapper<                                                          \
            Value,                                                                            \
            uv_ ## Module ## _t,                                                              \
            uv_ ## Module ## _req_cleanup,                                                    \
            decltype(& uv_ ## Module ## _ ## Name),                                           \
            uv_ ## Module ## _ ## Name,                                                       \
            Extractor                                                                         \
        >::execute<Mode>(FIBERIZE_IO_DETAIL_DEFINE_FORWARD(Args));                            \
    }

#define FIBERIZE_IO_DETAIL_INSTANTIATE(Value, Name, Args)                                     \
    template IOResult<Value, Await> Name<Await> ( FIBERIZE_IO_DETAIL_DEFINE_ARGS(Args) );     \
    template IOResult<Value, Block> Name<Block> ( FIBERIZE_IO_DETAIL_DEFINE_ARGS(Args) );     \
    template IOResult<Value, Async> Name<Async> ( FIBERIZE_IO_DETAIL_DEFINE_ARGS(Args) );

#define FIBERIZE_IO_DETAIL_WRAPPER(Module, Name, Extractor, Value, Args)    \
    FIBERIZE_IO_DETAIL_DEFINE_WRAPPER(Module, Name, Extractor, Value, Args) \
    FIBERIZE_IO_DETAIL_INSTANTIATE(Value, Name, Args)

} // namespace detail
} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_DETAIL_LIBUVWRAPPER_HPP
