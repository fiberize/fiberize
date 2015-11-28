#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>
#include <chrono>

using namespace fiberize;
using namespace std::literals;

uint32_t timers = 100000;

TEST(Sleep, ShouldWork) {
    FiberSystem fiberSystem;
    FiberRef self = fiberSystem.fiberize();

    std::vector<FutureRef<void>> refs;

    auto sleeper = fiberSystem.future([] () {
        io::sleep(1s);
    });

    for (size_t i = 0; i < timers; ++i) {
        refs.push_back(sleeper.copy().run());
    }

    io::sleep<io::Block>(500ns);
    io::sleep<io::Await>(500ns);

    for (FutureRef<void>& ref : refs) {
        ref.await();
    }

    refs.clear();
}
