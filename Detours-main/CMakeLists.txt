# CMakeLists.txt for Detours

# Set the minimum required CMake version
cmake_minimum_required(VERSION 3.5)

# Define the project and version
project(detours VERSION 4.0)

# Create the Detours library target
add_library(detours STATIC
        src/creatwth.cpp
        src/detours.cpp
        src/detours.h
        src/detver.h
        src/disasm.cpp
        src/disolarm.cpp
        src/disolarm64.cpp
        src/disolia64.cpp
        src/disolx64.cpp
        src/disolx86.cpp
        src/image.cpp
        src/modules.cpp
        src/uimports.cpp)

# This file is included and not compiled on its own
set_property (
        SOURCE src/uimports.cpp
        APPEND PROPERTY HEADER_FILE_ONLY true)

target_compile_definitions(detours PRIVATE _X86_)
target_include_directories(detours PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
