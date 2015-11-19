FROM ubuntu:15.10

MAINTAINER Paweł Nowak pawel@livetalk.lol

RUN apt-get update

# Install build tools.
RUN apt-get install -y build-essential g++ git cmake

# Install boost.
RUN apt-get install -y libboost-dev libboost-context-dev libboost-thread-dev libboost-system-dev

# Install and compile google test.
RUN apt-get install -y libgtest-dev && cd /usr/src/gtest && cmake . && make && cp lib*.a /usr/lib

# Install tlmalloc.
RUN apt-get install -y libgoogle-perftools-dev

# Build and install fiberize.
COPY fiberize/ /usr/src/fiberize/
RUN mkdir -p /tmp/build/fiberize && cd /tmp/build/fiberize && cmake /usr/src/fiberize/ -DCMAKE_BUILD_TYPE=Release && make -j8 && make -j8 test && make install
