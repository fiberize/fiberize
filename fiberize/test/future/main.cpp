#include <gtest/gtest.h>
#include <fiberize/fiberize.hpp>

using namespace fiberize;

uint futures = 1000000;

TEST(Futures, ShouldReturnValues) {
    FiberSystem fiberSystem;
    fiberSystem.fiberize();

    auto id = fiberSystem.future([] (auto x) {
        return x;
    });

    std::vector<FutureRef<uint>> refs;
    for (uint i = 0; i < futures; ++i) {
        refs.push_back(id.copy().run(i));
    }
    for (uint i = 0; i < futures; ++i) {
        EXPECT_EQ(i, refs[i].await().get());
    }
}
