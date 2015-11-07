#ifndef FIBERIZE_TYPES_HPP
#define FIBERIZE_TYPES_HPP

namespace fiberize {
  
/**
 * Void is a type with no values.
 */
struct Void {
private:
    Void() {};
};

/**
 * Unit is a type with exactly one value.
 */
struct Unit {};
    
    
} // namespace fiberize

#endif // FIBERIZE_TYPES_HPP
