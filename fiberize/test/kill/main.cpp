#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;
using ::testing::TestWithParam;

class Sleeper : public Future<void> {
public:
    void run () {
        processForever();
    }
};

TEST(Sleeper, ShouldDie) {
    FiberSystem system;
    system.fiberize();

    auto sleeper = system.future<Sleeper>().run();
    sleeper.kill();
    EXPECT_THROW(sleeper.result()->await(), Killed);
}
