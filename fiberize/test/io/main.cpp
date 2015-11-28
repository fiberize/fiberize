#include <fiberize/fiberize.hpp>
#include <gtest/gtest.h>

using namespace fiberize;
using namespace fiberize::io;

std::string fileTest(std::string data, std::string path) {
    int n = data.size() + 1;
    char* inBuffer = strdup(data.c_str());
    char* outBuffer = new char[n];
    Buffer inb(inBuffer, n);
    Buffer oub(outBuffer, n);

    int file = open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0777);
    write<Async>(file, &inb, 1, 0)->await();
    read<Await>(file, &oub, 1, 0);
    close(file);

    free(inBuffer);
    return outBuffer;
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
