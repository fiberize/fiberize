/**
 * The libuv wrapper template and helper macros.
 *
 * @file libuvwrapper.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_IO_DETAIL_LIBUVWRAPPER_HPP
#define FIBERIZE_IO_DETAIL_LIBUVWRAPPER_HPP

#include <fiberize/scheduler.hpp>
#include <fiberize/eventcontext.hpp>
#include <fiberize/io/mode.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/refrencecounted.hpp>
#include <fiberize/context.hpp>

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
struct TryToComplete {
    static void execute(const std::shared_ptr<Promise<Value>>& promise, Request* req) {
        promise->tryToComplete(extractor(req));
    }
};

template <typename Request, void (*extractor)(Request*)>
struct TryToComplete<void, Request, extractor> {
    static void execute(const std::shared_ptr<Promise<void>>& promise, Request*) {
        promise->tryToComplete();
    }
};

/**
 * Closure enviroment used for Await mode operations.
 */
template <typename Request, void (*cleanup)(Request*)>
struct AwaitEnv : public fiberize::detail::ReferenceCountedAtomic {
    AwaitEnv() {
        request.data = this;
        condition = false;
        dirty = false;
        scheduler = Scheduler::current();
        block = scheduler->currentControlBlock();
        block->grab();
    }

    ~AwaitEnv() {
        block->drop();
        if (dirty)
            cleanup(&request);
    }

    static void callback(Request* req) {
        auto ctx = reinterpret_cast<AwaitEnv*>(req->data);

        /**
         * Set the condition to true and reschedule the fiber, if necesssary.
         */
        {
            boost::unique_lock<fiberize::detail::ControlBlockMutex> lock(ctx->block->mutex);
            ctx->condition = true;
            if (ctx->block->status == fiberize::detail::Suspended) {
                /// @todo what if the scheduler is destroyed? This isn't a big problem right now, because it only
                ///       happens after shutdown. This should be fixed for the graceful shutdown patch.
                ctx->scheduler->enable(ctx->block, std::move(lock));
            }
        }

        ctx->drop();
    }

    Request request;
    bool condition;
    bool dirty;
    fiberize::detail::ControlBlock* block;
    Scheduler* scheduler;
};

/**
 * Environment for blocking mode. All it does is handle the cleanup.
 */
template <typename Request, void (*cleanup)(Request*)>
struct BlockEnv {
    BlockEnv() {
        dirty = false;
    }

    ~BlockEnv() {
        if (dirty)
            cleanup(&request);
    }

    bool dirty;
    Request request;
};

/**
 * Closure environment for the Async mode.
 */
template <typename Value, typename Request, void (*cleanup)(Request*), Value (*extractor)(Request*)>
struct AsyncEnv : public fiberize::detail::ReferenceCountedAtomic {
    AsyncEnv(const std::shared_ptr<Promise<Value>>& promise_) {
        request.data = this;
        dirty = false;
        promise = promise_;
        scheduler = Scheduler::current();
    }

    ~AsyncEnv() {
        if (dirty)
            cleanup(&request);
    }

    static void callback(Request* req) {
        auto ctx = reinterpret_cast<AsyncEnv*>(req->data);

        /**
         * Check if the promise still exists. The user could release it.
         */
        auto promise = ctx->promise.lock();
        if (promise) {
            /**
             * Temporarily swap the current scheduler.
             * @todo what if the scheduler is destroyed?
             */
            SwapScheduler swapScheduler(ctx->scheduler);

            /**
             * Complete the promise.
             */
            if (req->result >= 0) {
                TryToComplete<Value, Request, extractor>::execute(promise, req);
            } else {
                promise->tryToFail(std::make_exception_ptr(
                    std::system_error(-req->result, std::system_category())
                ));
            }
        }

        ctx->drop();
    }

    Request request;
    bool dirty;
    std::weak_ptr<Promise<Value>> promise;
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
    static Result<Value, Mode> execute(Args&&... args) {
        return execute(Mode(), std::forward<Args>(args)...);
    }

    /**
     * Await mode wrapper.
     */
    template <typename... Args>
    static Result<Value, Await> execute(Await, Args&&... args) {
        using Env = AwaitEnv<Request, cleanup>;

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
    static Result<Value, Block> execute(Block, Args&&... args) {
        /**
         * Create the request on the stack, as there are not async callbacks.
         */
        BlockEnv<Request, cleanup> env;

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
    static Result<Value, Async> execute(Async, Args&&... args) {
        using Env = AsyncEnv<Value, Request, cleanup, extractor>;

        /**
         * Create the promise that will hold the result.
         */
        auto promise = std::make_shared<Promise<Value>>();

        /**
         * Create the environment.
         */
        boost::intrusive_ptr<Env> env(new Env(promise));

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

        return promise;
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
    Result<Value, Mode> Name ( FIBERIZE_IO_DETAIL_DEFINE_ARGS(Args) ) {                       \
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
    template Result<Value, Await> Name<Await> ( FIBERIZE_IO_DETAIL_DEFINE_ARGS(Args) );       \
    template Result<Value, Block> Name<Block> ( FIBERIZE_IO_DETAIL_DEFINE_ARGS(Args) );       \
    template Result<Value, Async> Name<Async> ( FIBERIZE_IO_DETAIL_DEFINE_ARGS(Args) );

#define FIBERIZE_IO_DETAIL_WRAPPER(Module, Name, Extractor, Value, Args)    \
    FIBERIZE_IO_DETAIL_DEFINE_WRAPPER(Module, Name, Extractor, Value, Args) \
    FIBERIZE_IO_DETAIL_INSTANTIATE(Value, Name, Args)

} // namespace detail
} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_DETAIL_LIBUVWRAPPER_HPP
