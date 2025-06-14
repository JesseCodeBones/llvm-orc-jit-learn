cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 17)
project(mlir_lowering VERSION 0.1.0 LANGUAGES C CXX)
find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)
if(MLIR_FOUND)
  message(STATUS "MLIR package found.")
else()
  message(FATAL_ERROR "MLIR package not found.")
endif()

# 添加LLVM和MLIR的包含目录
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})

# 添加 MLIR TableGen 包含目录
include_directories(${MLIR_MAIN_SRC_DIR}/include)
include_directories(${MLIR_BINARY_DIR}/include)


add_definitions(${LLVM_DEFINITIONS})

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
include(AddMLIR)

# 添加LLVM工具
include(HandleLLVMOptions)
include(AddLLVM)
include(TableGen)
include(AddLLVM)
include(AddMLIR)


get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)
get_property(extension_libs GLOBAL PROPERTY MLIR_EXTENSION_LIBS)
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


set(LLVM_TARGET_DEFINITIONS sample.td)
mlir_tablegen(sample.inc -gen-rewriters)
add_public_tablegen_target(SampleIncGen)

add_executable(mlir_lowering main.cpp)

foreach(lib ${MLIR_DIALECT_LIBS})
    message(STATUS "MLIR Library: ${lib}")
endforeach()

target_link_libraries(mlir_lowering ${llvm_libs}
        ${dialect_libs}
        ${conversion_libs}
        ${extension_libs}
        MLIRAnalysis
        MLIRBuiltinToLLVMIRTranslation
        MLIRCallInterfaces
        MLIRCastInterfaces
        MLIRExecutionEngine
        MLIRFunctionInterfaces
        MLIRIR
        MLIRLLVMCommonConversion
        MLIRLLVMToLLVMIRTranslation
        MLIRMemRefDialect
        MLIRParser
        MLIRPass
        MLIRSideEffectInterfaces
        MLIRTargetLLVMIRExport
        MLIRTransforms
)
