#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;
using ::testing::TestWithParam;

uint64_t fibonacciFuture(uint64_t n) {
    using namespace context;

    if (n <= 1) {
        return 1;
    } else {
        auto fibonacci = system()->future(fibonacciFuture);
        auto x = fibonacci.copy().run(n-2);
        auto y = fibonacci.run(n-1);
        return x.await().get() + y.await().get();
    }
}

uint64_t fibonacci(uint64_t n) {
    if (n <= 1) {
        return 1;
    } else {
        return fibonacci(n-2) + fibonacci(n-1);
    }
}

TEST(Fibonacci, ComputesFibonacciSequence) {
    FiberSystem system;
    system.fiberize();

    auto fibonacciPar = system.future(fibonacciFuture);
    for (int i = 0; i < 100; ++i) {
        for (uint64_t n = 0; n <= 20; ++n) {
            EXPECT_EQ(fibonacci(n), fibonacciPar.copy().run(n).await().get());
        }
    }
}
