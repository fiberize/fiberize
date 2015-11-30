#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;
using ::testing::TestWithParam;

TEST(Sleeper, ShouldDie) {
    FiberSystem system;
    system.fiberize();

    auto sleeper = system.future([] () {
        context::processForever();
    });

    for (int i = 0; i < 100000; ++i) {
        auto ref = sleeper.copy().run();
        ref.kill();
        EXPECT_THROW(ref.await().get(), Killed);
    }
}
