#include <fiberize/detail/executor.hpp>

namespace fiberize {
namespace detail {

Executor::Executor(): thread(&Executor::run, this) {

}

void Executor::execute(ControlBlock* controlBlock) {
    runQueue.push(controlBlock);
}

void Executor::run() {
    ControlBlock* controlBlock;
    while (true) {
        while (!runQueue.pop(controlBlock));
        boost::context::jump_fcontext(&returnContext, controlBlock->context, reinterpret_cast<intptr_t>(controlBlock));
    }
}
    
} // namespace detail    
} // namespace fiberize
