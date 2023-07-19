cmake_minimum_required(VERSION 3.26.4)

enable_language(CXX)
project(rcx LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE RCX_SOURCE_FILES src/*.cxx)
add_executable(${PROJECT_NAME} ${RCX_SOURCE_FILES})

find_package(LLVM 15.0.7 REQUIRED)

file(GLOB SUBPROJECTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/*)
foreach(SUBPROJECTS_ITER ${SUBPROJECTS_DIR})
  add_subdirectory(${SUBPROJECTS_ITER})
endforeach()

add_dependencies(${PROJECT_NAME} argparse)
add_dependencies(${PROJECT_NAME} spdlog)

llvm_map_components_to_libnames(llvm_libs support core irreader)

target_include_directories(
  ${PROJECT_NAME} PRIVATE
  # includes
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${LLVM_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/spdlog/include
  )
target_link_libraries(
  ${PROJECT_NAME}
  spdlog::spdlog
  argparse
  ${llvm_libs}
)
# target_compile_options(
#   ${PROJECT_NAME} PRIVATE
#   "$<IF:$<STREQUAL:${CMAKE_CXX_COMPILER_ID},Clang>,-ferror-limit,-fmax-errors>=1024"
# )