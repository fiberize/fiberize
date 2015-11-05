#ifndef FIBERIZE_LOCALITY_HPP
#define FIBERIZE_LOCALITY_HPP

#include <cinttypes>

namespace fiberize {

enum Locality : uint8_t {
    DevNull = 0,
    Local = 1,
    Interprocess = 2,
    Remote = 3
};

    
} // namespace fiberize

#endif // FIBERIZE_LOCALITY_HPP
