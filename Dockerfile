FROM ubuntu:15.10

MAINTAINER Paweł Nowak pawel@livetalk.lol

RUN apt-get update

# Install build tools.
RUN apt-get install -y wget build-essential g++ git cmake valgrind libbz2-dev

# Install boost with valgrind support.
RUN wget http://downloads.sourceforge.net/project/boost/boost/1.59.0/boost_1_59_0.tar.gz
RUN tar xzf boost_1_59_0.tar.gz
RUN cd boost_1_59_0/tools/build && ./bootstrap.sh && ./b2 -j8 toolset=gcc install --prefix=/usr
RUN cd boost_1_59_0 && b2 -j8 toolset=gcc valgrind=on --prefix=/usr install

# Build and install fiberize.
COPY fiberize/ fiberize/
RUN mkdir -p build/fiberize && cd build/fiberize && cmake ../../fiberize/ -DCMAKE_BUILD_TYPE=Release && make -j8 && make install
