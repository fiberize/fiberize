#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;

std::string fileTest(std::string data, std::string path) {
    int n = data.size() + 1;
    char* inBuffer = strdup(data.c_str());
    char* outBuffer = new char[n];
    io::Buffer inb(inBuffer, n);
    io::Buffer oub(outBuffer, n);

    int file = io::open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0777);
    io::write<io::Async>(file, &inb, 1, 0).await();
    io::fsync(file);
    io::read<io::Block>(file, &oub, 1, 0);
    io::close(file);

    free(inBuffer);
    std::string out = outBuffer;
    delete[] outBuffer;
    return out;
}

FiberSystem fiberSystem;

std::string data = "Hello world!";
std::string path = "/tmp/jiqomfio3n9g0j";

TEST(File, ThreadReadsAndWrites) {
    EXPECT_EQ(data, fileTest(data, path));
}

TEST(File, FiberReadsAndWrites) {
    EXPECT_EQ(data, fiberSystem.future(fileTest).run(data, path).result()->await());
}

int main(int argc, char **argv) {
    fiberSystem.fiberize();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
