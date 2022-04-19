cmake_minimum_required(VERSION 3.16)
if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(FATAL_ERROR "Do not support non-Linux now!")
endif()

project(depdetector CXX C)

set(CMAKE_CXX_FLAGS "-fpermissive -g")

#set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/igraph)
target_compile_options(igraph PRIVATE -fPIC)

#option(ENABLE_EXAMPLE "Enable example" ON)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/third_party/dbg)
include_directories(igraph)

link_libraries(igraph)
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(src)