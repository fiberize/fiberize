#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;
using ::testing::TestWithParam;

TEST(Sleeper, ShouldDie) {
    FiberSystem system;
    system.fiberize();

    auto sleeper = system.future([] () {
        context::processForever();
    }).run();
    sleeper.kill();
    EXPECT_THROW(sleeper.await(), Killed);
}
