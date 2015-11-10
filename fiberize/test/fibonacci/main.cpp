#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;
using ::testing::TestWithParam;

struct Fibonacci : public Fiber<uint64_t> {
    Fibonacci(uint64_t n) : n(n) {}
    uint64_t n;
    
    uint64_t run() override {
        if (n <= 1) {
            return 1;
        } else {
            auto x = system()->run<Fibonacci>(n-2); x.watch(self());
            auto y = system()->run<Fibonacci>(n-1); y.watch(self());
            return x.finished().await() + y.finished().await();
        }
    }
};

uint64_t fibonacci(uint64_t n) {
    if (n <= 1) {
        return 1;
    } else {
        return fibonacci(n-2) + fibonacci(n-1);
    }
}

TEST(Fibonacci, ComputesFibonacciSequence) {
    System system;
    system.fiberize();
    
    for (uint64_t n = 0; n < 10; ++n) {
        EXPECT_EQ(fibonacci(n), system.run<Fibonacci>(n).finished().await());
    }
}
