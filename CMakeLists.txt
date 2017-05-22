# Author:  Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
# Created: May 2017
# This software is copyrighted under the BSD 3-Clause License. 

cmake_minimum_required(VERSION 2.8.12)
project(PerformanceBenchmark CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# DSA to target (mandatory)
set(BENCHMARK_DSA "" CACHE STRING "The SDAccel DSA (firmware) to target.")
# Performance-related configuration 
set(BENCHMARK_COMPUTE_DEPTH 20 CACHE STRING "Number of compute stages in a deep pipeline.")
set(BENCHMARK_COMPUTE_WIDTH 4 CACHE STRING "Width of data path in number of elements.")
set(BENCHMARK_DATA_TYPE "float" CACHE STRING "Floating point data type.")
set(BENCHMARK_TARGET_CLOCK 0 CACHE STRING "Target clock speed. If set to zero, the default of the DSA will be used.")
set(BENCHMARK_ADDS_PER_STAGE 1 CACHE STRING "Number of additions to perform per stage.")
set(BENCHMARK_MULTS_PER_STAGE 1 CACHE STRING "Number of mults to perform per stage.")
set(BENCHMARK_ADD_CORE "FAddSub_fulldsp" CACHE STRING "Floating point core to use for addition. Must match data type.")
set(BENCHMARK_MULT_CORE "FMul_fulldsp" CACHE STRING "Floating point core to use for multiplication. Must match data type.")

# Other configuration
set(BENCHMARK_HLS_TARGET_TIMING 3.3333 CACHE STRING "Target timing of HLS.")
set(BENCHMARK_PART_NAME "xcku115-flvb2104-2-e") 
set(BENCHMARK_KEEP_INTERMEDIATE OFF CACHE STRING
    "Keep intermediate SDAccel files")

# Enforce DSA and DIMMs to be manually specified
if(BENCHMARK_DSA STREQUAL "")
  message(FATAL_ERROR "Please specify the DSA to use by setting -DBENCHMARK_DSA=<value>")
endif()
if(BENCHMARK_COMPUTE_DEPTH LESS 1)
  message(FATAL_ERROR "Please specify number of compute stages with -DBENCHMARK_COMPUTE_DEPTH=<value>.")
endif()
if(BENCHMARK_COMPUTE_WIDTH LESS 1)
  message(FATAL_ERROR "Please specify SIMD width of compute stages with -DBENCHMARK_COMPUTE_WIDTH=<value>.")
endif()

# Dependencies
find_package(SDAccel REQUIRED)
include_directories(${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/include ${SDAccel_INCLUDE_DIRS} ${SDAccel_ROOT_DIR}/Vivado_HLS/include)
set(BENCHMARK_LIBS ${SDAccel_LIBRARIES})

# Compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Dependencies
set(BENCHMARK_HLS_DEPENDS
    include/PerformanceBenchmark.h.in
    kernel/PerformanceBenchmark.cpp
    include/hlslib/SDAccel.h)

# Configure files 
configure_file(include/PerformanceBenchmark.h.in PerformanceBenchmark.h)
configure_file(script/Synthesis.tcl.in Synthesis.tcl)

# Synthesis
add_custom_target(synthesis
  COMMAND ${SDAccel_VIVADO_HLS} -f Synthesis.tcl
  DEPENDS ${BENCHMARK_HLS_DEPENDS})

# Testing
enable_testing()
add_executable(Testbench src/Testbench.cpp kernel/PerformanceBenchmark.cpp)
target_link_libraries(Testbench ${BENCHMARK_LIBS})
add_test(Testbench Testbench)

# Hardware kernel 
add_executable(ExecuteKernel.exe src/ExecuteKernel.cpp)
target_link_libraries(ExecuteKernel.exe ${BENCHMARK_LIBS})
set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS} 
  -O3
  # Includes
  -I${CMAKE_BINARY_DIR}
  -I${CMAKE_SOURCE_DIR}/include
  # Source
  ${CMAKE_SOURCE_DIR}/kernel/PerformanceBenchmark.cpp
  # Flags
  --kernel PerformanceBenchmark 
  --xdevice ${BENCHMARK_DSA}
  --xp prop:kernel.PerformanceBenchmark.kernel_flags="${CMAKE_CXX_FLAGS}")
if(BENCHMARK_TARGET_CLOCK GREATER 0)
  set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS}
  --kernel_frequency=${BENCHMARK_TARGET_CLOCK})
endif()
add_custom_target(kernel
  COMMAND ${SDAccel_XOCC} -t hw ${BENCHMARK_XOCC_FLAGS}
  -o perf_benchmark.xclbin
  DEPENDS ${BENCHMARK_HLS_DEPENDS})
