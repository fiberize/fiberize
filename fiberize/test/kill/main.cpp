#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;
using ::testing::TestWithParam;

uint fibers = 100000;

TEST(Sleeper, ShouldDie) {
    FiberSystem system;
    system.fiberize();

    auto sleeper = system.future([] () {
        context::processForever();
    });

    std::vector<FutureRef<void>> refs;

    for (uint i = 0; i < fibers; ++i) {
        refs.push_back(sleeper.copy().run());
    }

    for (uint i = 0; i < fibers; ++i) {
        refs[i].kill();
    }

    for (uint i = 0; i < fibers; ++i) {
        EXPECT_THROW(refs[i].await().get(), Killed);
    }

    refs.clear();
}
