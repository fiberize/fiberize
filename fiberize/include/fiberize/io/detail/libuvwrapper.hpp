/**
 * Scary file implementing the libuv wrapper template and helper macros.
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

#include <system_error>

#include <boost/preprocessor.hpp>

namespace fiberize {
namespace io {
namespace detail {

struct AwaitContext {
    bool condition;
    fiberize::detail::ControlBlock* controlBlock;
};

template <typename Value, typename Request>
struct AsyncContext {
    Request request;
    std::weak_ptr<Promise<Value>> promise;
    FiberSystem* system;
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

template <typename Value, typename Request, void (*cleanup)(Request*), typename UVFunctionType, UVFunctionType uvfunction, Value (*extractor)(Request*)>
struct LibUVWrapper {
    template <typename Mode, typename... Args>
    static Result<Value, Mode> execute(Args&&... args) {
        return execute(Mode(), std::forward<Args>(args)...);
    }

    struct ScopedCleanup {
        ScopedCleanup(Request* req) {
            this->req = req;
        }

        ~ScopedCleanup() {
            cleanup(req);
        }

        Request* req;
    };

    template <typename... Args>
    static Result<Value, Await> execute(Await, Args&&... args) {
        AwaitContext ctx;
        ctx.condition = false;
        ctx.controlBlock = Scheduler::current()->currentControlBlock();

        Request req;
        req.data = &ctx;

        int code = uvfunction(Scheduler::current()->ioContext().loop(), &req, std::forward<Args>(args)..., [] (Request* req) {
            auto ctx = reinterpret_cast<AwaitContext*>(req->data);
            boost::unique_lock<fiberize::detail::ControlBlockMutex> lock(ctx->controlBlock->mutex);
            ctx->condition = true;
            if (ctx->controlBlock->status == fiberize::detail::Suspended) {
                Scheduler::current()->enable(ctx->controlBlock, std::move(lock));
            }
        });

        if (code < 0) {
            throw std::system_error(-code, std::system_category());
        }

        EventContext::current()->processUntil(ctx.condition);

        ScopedCleanup finalize(&req);
        if (req.result >= 0) {
            return extractor(&req);
        } else {
            throw std::system_error(-req.result, std::system_category());
        }
    }

    template <typename... Args>
    static Result<Value, Block> execute(Block, Args&&... args) {
        Request req;
        int code = uvfunction(Scheduler::current()->ioContext().loop(), &req, std::forward<Args>(args)..., nullptr);

        if (code < 0) {
            throw std::system_error(-code, std::system_category());
        }

        ScopedCleanup finalize(&req);
        if (req.result >= 0) {
            return extractor(&req);
        } else {
            throw std::system_error(-req.result, std::system_category());
        }
    }

    template <typename... Args>
    static Result<Value, Async> execute(Async, Args&&... args) {
        auto promise = std::make_shared<Promise<Value>>();

        auto ctx = new AsyncContext<Value, Request>;
        ctx->promise = promise;
        ctx->system = Scheduler::current()->system();
        ctx->request.data = ctx;

        int code = uvfunction(Scheduler::current()->ioContext().loop(), &ctx->request, std::forward<Args>(args)..., [] (Request* req) {
            auto ctx = reinterpret_cast<AsyncContext<Value, Request>*>(req->data);
            auto promise = ctx->promise.lock();
            if (promise) {
                if (Scheduler::current() == nullptr) {
                    ctx->system->fiberize();
                }

                if (req->result >= 0) {
                    TryToComplete<Value, Request, extractor>::execute(promise, req);
                } else {
                    promise->tryToFail(std::make_exception_ptr(
                        std::system_error(-req->result, std::system_category())
                    ));
                }
            }
            cleanup(&ctx->request);
            delete ctx;
        });

        if (code < 0) {
            delete ctx;
            throw std::system_error(-code, std::system_category());
        }

        return promise;
    }
};

#define FIBERIZE_IO_DETAIL_REM(...) __VA_ARGS__
#define FIBERIZE_IO_DETAIL_EAT(...)

// Strip off the type
#define FIBERIZE_IO_DETAIL_STRIP(x) FIBERIZE_IO_DETAIL_EAT x
// Show the type without parenthesis
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
