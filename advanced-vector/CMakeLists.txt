cmake_minimum_required(VERSION 3.16)
project(advanced_vector)


set(CMAKE_CXX_STANDARD 17)


include_directories(.)


add_executable(advanced_vector
        main.cpp
        vector.h)