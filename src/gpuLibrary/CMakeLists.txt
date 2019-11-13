cmake_minimum_required(VERSION 3.15)

find_package(CUDA 10.0 REQUIRED)

project(shastaGpuLibraryStatic)

set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -std=c++11")
# Source files
cuda_add_library(shastaGpuLibraryStatic STATIC ../gpu/GPU.cu)
