cmake_minimum_required(VERSION 3.15)
project(code)

set(CMAKE_CXX_STANDARD 17)

add_executable(code main.cpp DataBlock.h Cache.h System.h Include.h Core.h Tester.h Task.h)