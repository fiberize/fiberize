#include <fiberize/io/file.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/fibersystem.hpp>

#include <system_error>

namespace fiberize {
namespace io {

using namespace fiberize::detail;

File::File(int fd) : file(fd) {}

File::File(File&& other) {
    file = other.file;
    other.file = -1;
}

File& File::operator=(File&& other) {
    file = other.file;
    other.file = -1;
    return *this;
}

struct AwaitContext {
    bool condition;
    ControlBlock* controlBlock;
};

template <>
File File::open<Await>(const char* path, int flags, int mode) {
    AwaitContext ctx;
    ctx.condition = false;
    ctx.controlBlock = Scheduler::current()->currentControlBlock();

    uv_fs_t req;
    req.data = &ctx;

    uv_fs_open(Scheduler::current()->ioContext().loop(), &req, path, flags, mode, [] (uv_fs_t* req) {
        auto ctx = reinterpret_cast<AwaitContext*>(req->data);
        boost::unique_lock<ControlBlockMutex> lock(ctx->controlBlock->mutex);
        ctx->condition = true;
        if (ctx->controlBlock->status == Suspended) {
            Scheduler::current()->enable(ctx->controlBlock, std::move(lock));
        }
    });

    EventContext::current()->processUntil(ctx.condition);

    if (req.result >= 0) {
        return File(req.result);
    } else {
        throw std::system_error(-req.result, std::system_category());
    }
}

template <>
File File::open<Block>(const char* path, int flags, int mode) {
    uv_fs_t req;
    uv_fs_open(Scheduler::current()->ioContext().loop(), &req, path, flags, mode, nullptr);

    if (req.result >= 0) {
        return File(req.result);
    } else {
        throw std::system_error(-req.result, std::system_category());
    }
}

template <typename A, typename F>
class RequestClosure {
public:
    RequestClosure(const std::shared_ptr<Promise<A>>& promise, F&& result)
        : promiseWeak(promise), result(std::forward<F>(result)), system(Scheduler::current()->system()) {
        request.data = this;
    }

    uv_fs_cb callback() {
        return [] (uv_fs_t* req) {
            auto self = reinterpret_cast<RequestClosure*>(req->data);
            self->execute();
            delete self;
        };
    }

    uv_fs_t request;

private:
    void execute() {
        auto promise = promiseWeak.lock();
        if (promise) {
            if (Scheduler::current() == nullptr) {
                system->fiberize();
            }

            if (request.result >= 0) {
                promise->tryToComplete(result(request));
            } else {
                promise->tryToFail(std::make_exception_ptr(
                    std::system_error(request.result, std::system_category())
                ));
            }
        }
    }

    std::weak_ptr<Promise<A>> promiseWeak;
    F result;
    FiberSystem* system;
};

template <typename A, typename F>
RequestClosure<A, F>* newRequestClosure(const std::shared_ptr<Promise<A>>& promise, F&& result) {
    return new RequestClosure<A, F>(promise, std::forward<F>(result));
}

template <>
std::shared_ptr<Promise<File>> File::open<Async>(const char* path, int flags, int mode) {
    auto promise = std::make_shared<Promise<File>>();
    auto closure = newRequestClosure(promise, [] (uv_fs_t& req) {
        return File(req.result);
    });
    uv_fs_open(Scheduler::current()->ioContext().loop(), &closure->request, path, flags, mode, closure->callback());
    return promise;
}

template <>
void File::close<Await>() {
    AwaitContext ctx;
    ctx.condition = false;
    ctx.controlBlock = Scheduler::current()->currentControlBlock();

    uv_fs_t req;
    req.data = &ctx;

    uv_fs_close(Scheduler::current()->ioContext().loop(), &req, file, [] (uv_fs_t* req) {
        auto ctx = reinterpret_cast<AwaitContext*>(req->data);
        boost::unique_lock<ControlBlockMutex> lock(ctx->controlBlock->mutex);
        ctx->condition = true;
        if (ctx->controlBlock->status == Suspended) {
            Scheduler::current()->enable(ctx->controlBlock, std::move(lock));
        }
    });

    EventContext::current()->processUntil(ctx.condition);

    if (req.result >= 0) {
        file = -1;
    } else {
        throw std::system_error(-req.result, std::system_category());
    }
}

template <>
void File::close<Block>() {
    uv_fs_t req;
    uv_fs_close(Scheduler::current()->ioContext().loop(), &req, file, nullptr);

    if (req.result >= 0) {
        file = -1;
    } else {
        throw std::system_error(-req.result, std::system_category());
    }
}

template <>
std::shared_ptr<Promise<Unit>> File::close<Async>() {
    auto promise = std::make_shared<Promise<Unit>>();
    auto closure = newRequestClosure(promise, [] (uv_fs_t&) {
        return Unit {};
    });
    uv_fs_close(Scheduler::current()->ioContext().loop(), &closure->request, file, closure->callback());
    return promise;
}

template <>
ssize_t File::read<Await>(const Buffer bufs[], uint nbufs, int64_t offset) {
    AwaitContext ctx;
    ctx.condition = false;
    ctx.controlBlock = Scheduler::current()->currentControlBlock();

    uv_fs_t req;
    req.data = &ctx;

    uv_fs_read(Scheduler::current()->ioContext().loop(), &req, file, detail::static_buffer_cast(bufs), nbufs, offset, [] (uv_fs_t* req) {
        auto ctx = reinterpret_cast<AwaitContext*>(req->data);
        boost::unique_lock<ControlBlockMutex> lock(ctx->controlBlock->mutex);
        ctx->condition = true;
        if (ctx->controlBlock->status == Suspended) {
            Scheduler::current()->enable(ctx->controlBlock, std::move(lock));
        }
    });

    EventContext::current()->processUntil(ctx.condition);

    if (req.result >= 0) {
        return req.result;
    } else {
        throw std::system_error(-req.result, std::system_category());
    }
}

template <>
ssize_t File::read<Block>(const Buffer bufs[], uint nbufs, int64_t offset) {
    uv_fs_t req;
    uv_fs_read(Scheduler::current()->ioContext().loop(), &req, file, detail::static_buffer_cast(bufs), nbufs, offset, nullptr);

    if (req.result >= 0) {
        return req.result;
    } else {
        throw std::system_error(-req.result, std::system_category());
    }
}

template <>
std::shared_ptr<Promise<ssize_t>> File::read<Async>(const Buffer bufs[], uint nbufs, int64_t offset) {
    auto promise = std::make_shared<Promise<ssize_t>>();
    auto closure = newRequestClosure(promise, [] (uv_fs_t& req) {
        return req.result;
    });
    uv_fs_read(Scheduler::current()->ioContext().loop(), &closure->request, file, detail::static_buffer_cast(bufs), nbufs, offset, closure->callback());
    return promise;
}

template <>
ssize_t File::write<Await>(const Buffer bufs[], uint nbufs, int64_t offset) {
    AwaitContext ctx;
    ctx.condition = false;
    ctx.controlBlock = Scheduler::current()->currentControlBlock();

    uv_fs_t req;
    req.data = &ctx;

    uv_fs_write(Scheduler::current()->ioContext().loop(), &req, file, detail::static_buffer_cast(bufs), nbufs, offset, [] (uv_fs_t* req) {
        auto ctx = reinterpret_cast<AwaitContext*>(req->data);
        boost::unique_lock<ControlBlockMutex> lock(ctx->controlBlock->mutex);
        ctx->condition = true;
        if (ctx->controlBlock->status == Suspended) {
            Scheduler::current()->enable(ctx->controlBlock, std::move(lock));
        }
    });

    EventContext::current()->processUntil(ctx.condition);

    if (req.result >= 0) {
        return req.result;
    } else {
        throw std::system_error(-req.result, std::system_category());
    }
}

template <>
ssize_t File::write<Block>(const Buffer bufs[], uint nbufs, int64_t offset) {
    uv_fs_t req;
    uv_fs_write(Scheduler::current()->ioContext().loop(), &req, file, detail::static_buffer_cast(bufs), nbufs, offset, nullptr);

    if (req.result >= 0) {
        return req.result;
    } else {
        throw std::system_error(-req.result, std::system_category());
    }
}

template <>
std::shared_ptr<Promise<ssize_t>> File::write<Async>(const Buffer bufs[], uint nbufs, int64_t offset) {
    auto promise = std::make_shared<Promise<ssize_t>>();
    auto closure = newRequestClosure(promise, [] (uv_fs_t& req) {
        return req.result;
    });
    uv_fs_write(Scheduler::current()->ioContext().loop(), &closure->request, file, detail::static_buffer_cast(bufs), nbufs, offset, closure->callback());
    return promise;
}

} // namespace io
} // namespace fiberize
