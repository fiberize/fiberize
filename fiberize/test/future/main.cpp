#include <gtest/gtest.h>
#include <fiberize/fiberize.hpp>

using namespace fiberize;

uint futures = 1000000;

TEST(Futures, ShouldReturnValues) {
    FiberSystem fiberSystem;
    fiberSystem.fiberize();

    uint count = 0;


    std::vector<FutureRef<uint>> refs;
    for (uint i = 0; i < futures; ++i) {
        auto id = fiberSystem.future([&count] (auto x) {
            return x;
        });
        refs.push_back(id.run(i));
    }
    for (uint i = 0; i < futures; ++i) {
        EXPECT_EQ(i, refs[i].await().get());
    }
}
