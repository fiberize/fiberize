cmake_minimum_required(VERSION 2.6)
project(fibr)

add_executable(fibr main.cpp)

install(TARGETS fibr RUNTIME DESTINATION bin)
