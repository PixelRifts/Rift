cmake_minimum_required(VERSION 3.11)
project(Rift)
set(CMAKE_C_STANDARD 11) # Enable c11 standard

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS source/*.c source/*.h)

add_executable(Rift ${SOURCE_FILES})
target_include_directories(Rift PRIVATE source/)

if(MSVC)
    target_include_directories(Rift PRIVATE third-party/include/)
    target_link_directories(Rift PRIVATE third-party/lib/)
    target_link_libraries(Rift LLVM-C)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(/Zc:preprocessor)
endif()
if(UNIX)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS, "${LLVM_INCLUDE_DIRS}")
    include_directories(${LLVM_INCLUDE_DIRS})
    target_include_directories(Rift PRIVATE third-party/lib)
    add_definitions(${LLVM_DEFINITIONS})
    llvm_map_components_to_libnames(llvm_libs
            Analysis
            Core
            ExecutionEngine
            InstCombine
            Object
            OrcJIT
            RuntimeDyld
            ScalarOpts
            Support
            native)
    target_link_libraries(Rift ${llvm_libs} m)
endif()