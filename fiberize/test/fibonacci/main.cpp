#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;
using ::testing::TestWithParam;

struct Fibonacci : public Future<uint64_t> {
    Fibonacci(uint64_t n) : n(n) {}
    uint64_t n;
    
    uint64_t run() override {
        if (n <= 1) {
            return 1;
        } else {
            auto fibbonacci = system()->future<Fibonacci>();
            auto x = fibbonacci.run(n-2);
            auto y = fibbonacci.run(n-1);
            return x.result()->await() + y.result()->await();
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
    FiberSystem system;
    system.fiberize();
    
    for (uint64_t n = 0; n < 20; ++n) {
        EXPECT_EQ(fibonacci(n), system.future<Fibonacci>().run(n).result()->await());
    }
}
