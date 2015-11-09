#ifndef FIBERIZE_TYPES_HPP
#define FIBERIZE_TYPES_HPP

#include <cstdlib>

namespace fiberize {
  
/**
 * Void is a type with no values.
 */
struct Void {
private:
    Void() {};

public:
    /**
     * Ex Falso Quodlibet.
     */
    template <typename A>
    A absurd() {
        abort();
    }
};

/**
 * Unit is a type with exactly one value.
 */
struct Unit {};

    
} // namespace fiberize

#endif // FIBERIZE_TYPES_HPP
