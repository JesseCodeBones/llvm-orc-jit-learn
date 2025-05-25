cmake_minimum_required(VERSION 3.10.0)
project(orc-jit-1 VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
message(STATUS "LLVM Definitions: ${LLVM_DEFINITIONS}\n")

add_executable(orc-jit-1 main.cpp)

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
        native
)

foreach(LIB ${llvm_libs})
    message(STATUS "${LIB}")
endforeach()

target_link_libraries(orc-jit-1 ${llvm_libs})