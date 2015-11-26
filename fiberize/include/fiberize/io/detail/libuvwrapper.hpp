#ifndef FIBERIZE_IO_DETAIL_LIBUVWRAPPER_HPP
#define FIBERIZE_IO_DETAIL_LIBUVWRAPPER_HPP

#include <fiberize/scheduler.hpp>
#include <fiberize/eventcontext.hpp>
#include <fiberize/io/mode.hpp>
#include <fiberize/detail/controlblock.hpp>

#include <system_error>

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

        uvfunction(Scheduler::current()->ioContext().loop(), &req, std::forward<Args>(args)..., [] (Request* req) {
            auto ctx = reinterpret_cast<AwaitContext*>(req->data);
            boost::unique_lock<fiberize::detail::ControlBlockMutex> lock(ctx->controlBlock->mutex);
            ctx->condition = true;
            if (ctx->controlBlock->status == fiberize::detail::Suspended) {
                Scheduler::current()->enable(ctx->controlBlock, std::move(lock));
            }
        });

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
        uvfunction(Scheduler::current()->ioContext().loop(), &req, std::forward<Args>(args)..., nullptr);

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

        uvfunction(Scheduler::current()->ioContext().loop(), &ctx->request, std::forward<Args>(args)..., [] (Request* req) {
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

        return promise;
    }
};

#define FIBERIZE_IO_DETAIL_INSTANTIATE_MODES(Value, Name, ...)     \
    template Result<Value, Await> Name<Await>(__VA_ARGS__);        \
    template Result<Value, Block> Name<Block>(__VA_ARGS__);        \
    template Result<Value, Async> Name<Async>(__VA_ARGS__);

} // namespace detail
} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_DETAIL_LIBUVWRAPPER_HPP
