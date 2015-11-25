#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;
using namespace fiberize::io;

class Writer : public Future<std::string> {
public:
    Writer(const std::string& data, const std::string& path)
        : data(strdup(data.c_str())), n(data.size() + 1), path(path)
        {}

    ~Writer() {
        free(data);
    }

    char* data;
    size_t n;
    std::string path;

    std::string run() override {
        char* buffer = new char[n];
        Buffer wrb(data, n);
        Buffer rdb(buffer, n);

        File file = File::open(block, path.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0777);
        file.write(async, &wrb, 1, 0)->await();
        file.read(await, &rdb, 1, 0);
        file.close(block);

        return buffer;
    }
};

FiberSystem fiberSystem;

std::string data = "Hello world!";
std::string path = "/tmp/jiqomfio3n9g0j";

TEST(File, ThreadReadsAndWrites) {
    EXPECT_EQ(data, Writer(data, path).run());
}

TEST(File, FiberReadsAndWrites) {
    EXPECT_EQ(data, fiberSystem.run<Writer>(data, path).result()->await());
}

int main(int argc, char **argv) {
    fiberSystem.fiberize();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
