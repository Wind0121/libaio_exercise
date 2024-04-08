cmake_minimum_required(VERSION 3.0.0)
project(libaio_exercise VERSION 0.1.0 LANGUAGES C CXX)

add_executable(libaio_exercise main.cc)
target_link_libraries( libaio_exercise
    aio
)

