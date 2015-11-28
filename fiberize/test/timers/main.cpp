#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>
#include <chrono>

using namespace fiberize;
using namespace std::literals;

uint32_t timers = 10000;

TEST(Sleep, ShouldWork) {
    FiberSystem system;
    FiberRef self = system.fiberize();

    std::vector<FutureRef<void>> refs;

    auto sleeper = system.future([] () {
        io::sleep(1s);
    });

    for (size_t i = 0; i < timers; ++i) {
        refs.push_back(sleeper.copy().run());
    }

    io::sleep<io::Block>(500ms);
    io::sleep<io::Await>(500ms);

    for (FutureRef<void>& ref : refs) {
        ref.result()->await();
    }
}
