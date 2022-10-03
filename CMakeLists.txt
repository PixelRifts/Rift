cmake_minimum_required(VERSION 3.11)
project(Rift)
set(CMAKE_C_STANDARD 11) # Enable c11 standard

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS source/*.c source/*.h)

add_executable(Rift ${SOURCE_FILES})
target_include_directories(Rift PRIVATE third-party/include/)
target_include_directories(Rift PRIVATE source/)
target_link_directories(Rift PRIVATE third-party/lib/)
target_link_libraries(Rift LLVM-C)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
