#ifndef FIBERIZE_RUNNABLE_HPP
#define FIBERIZE_RUNNABLE_HPP

namespace fiberize {

/**
 * Anything that can be run.
 */
struct Runnable {
    virtual ~Runnable() {};
    virtual void operator () () = 0;
};

} // namespace fiberize

#endif // FIBERIZE_RUNNABLE_HPP
