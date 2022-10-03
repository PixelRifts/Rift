cmake_minimum_required(VERSION 3.11)
project(Rift)
set(CMAKE_C_STANDARD 11)            # Enable c11 standard

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS *.c *.h)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(simple_example ${SOURCE_FILES})
