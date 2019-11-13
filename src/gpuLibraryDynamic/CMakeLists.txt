cmake_minimum_required(VERSION 3.10)

find_package(CUDA 10.0 REQUIRED)

project(shastaGpuLibraryDynamic)

set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -std=c++11")
# Source files
cuda_add_library(shastaGpuLibraryDynamic SHARED ../gpu/GPU.cu)
