cmake_minimum_required(VERSION 3.0)
find_package(CUDA 10.0)
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -std=c++11")

project(shastaGpu)

# C++ dialect.
add_definitions(-std=c++14)

# Compilation warnings.
if(NOT MACOS)
    add_definitions(-Wall -Wconversion -Wno-unused-result)
endif(NOT MACOS)

# 16-byte compare and swap.
# This is required by dset64.hpp/dset64-gccAtomic.hpp".
add_definitions(-mcx16)

# Definitions needed to eliminate dependency on the boost system library.
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)

# Definitions that control what components get built.
add_definitions(-DSHASTA_HTTP_SERVER)
add_definitions(-DSHASTA_BUILD_FOR_GPU)

# Sources files.
file(GLOB GPUSRCS ../src/gpu/*.cu)
cuda_add_library(mygpu ${GPUSRCS})
target_include_directories(mygpu PUBLIC ../src/gpu)

# Sources files.
file(GLOB LIBSRCS ../src/*.cpp ../src/gpu/*.cpp ../src/gpu/*.cu)
add_library(mylib ${LIBSRCS})
target_include_directories(mylib PUBLIC ../src ../src/gpu)
target_link_libraries(mylib
    mygpu)

# Sources files.
file(GLOB SOURCES ../srcMain/*.cpp)

# Define our library.
add_executable(shastaGpu ${SOURCES})
target_include_directories(shastaGpu PUBLIC ../srcMain)
target_link_libraries(shastaGpu
    mylib
    atomic boost_system boost_program_options boost_chrono spoa png z -lpthread
    -Wl,--whole-archive -lpthread -Wl,--no-whole-archive)
