cmake_minimum_required(VERSION 3.27)
project(exception)

set(CMAKE_CXX_STANDARD 23)

add_executable(exception main.cpp
        stacktraced_exception.cpp
        stacktraced_exception.h)

find_package(Boost REQUIRED COMPONENTS)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(exception PRIVATE ${Boost_LIBRARIES})

include_directories(${minhook_INCLUDE_DIRS})
target_link_libraries(exception PRIVATE minhook)
