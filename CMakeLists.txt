cmake_minimum_required(VERSION 3.16)
if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(FATAL_ERROR "Do not support non-Linux now!")
endif()

project(depdetector CXX C)

#set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/igraph)
target_compile_options(igraph PRIVATE -fPIC)

#option(ENABLE_EXAMPLE "Enable example" ON)


# explicitly list all source file
add_library(
  depdetector
  SHARED
  
  src/utils.h
  src/graph.cpp
)


target_include_directories(depdetector PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(depdetector PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(depdetector PUBLIC ${PROJECT_SOURCE_DIR}/third_party/dbg)
target_include_directories(depdetector PRIVATE igraph)

target_link_libraries(depdetector PRIVATE igraph)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/dbg/ DESTINATION  ${CMAKE_INSTALL_PREFIX}/include)

add_subdirectory(src)