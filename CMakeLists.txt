cmake_minimum_required(VERSION 3.26.4)

# enable_language(CXX)
project(rcx LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE RCX_SOURCE_FILES CONFIGURE_DEPENDS src/*.cxx)
add_executable(${PROJECT_NAME} ${RCX_SOURCE_FILES})

find_package(LLVM 15.0.7 REQUIRED)

file(GLOB SUBPROJECTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/*)
list(FILTER SUBPROJECTS_DIR EXCLUDE REGEX "argparse") # but not for this
foreach(SUBPROJECTS_ITER ${SUBPROJECTS_DIR})
  add_subdirectory(${SUBPROJECTS_ITER})
endforeach()

set(SRC_DIR_argparse "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/argparse")
set(EXT_DIR_argparse "${PROJECT_BINARY_DIR}/external/argparse")
set(IST_DIR_argparse "${EXT_DIR_argparse}/out")

include(FetchContent)

FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
  GIT_TAG v3.0
  GIT_SHALLOW TRUE
)

set(BOOST_INCLUDE_LIBRARIES unordered)
set(BOOST_ENABLE_CMAKE ON)

FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost.git
  GIT_TAG boost-1.87.0
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  GIT_SUBMODULE_RECURSE FALSE
  GIT_SUBMODULES
    tools/cmake
    libs/unordered
    libs/assert
    libs/config
    libs/container_hash
    libs/core
    libs/describe
    libs/mp11
    libs/predef
    libs/static_assert
    libs/throw_exception
)

FetchContent_MakeAvailable(argparse)
FetchContent_MakeAvailable(Boost)

add_dependencies(${PROJECT_NAME} argparse)
add_dependencies(${PROJECT_NAME} spdlog)
add_dependencies(${PROJECT_NAME} Boost::unordered)
add_definitions(${LLVM_DEFINITIONS})

target_include_directories(
  ${PROJECT_NAME} PRIVATE
  # includes
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${LLVM_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/spdlog/include
  )
target_link_directories(${PROJECT_NAME} PRIVATE ${ARGPARSE_LIBRARY_DIR})

# llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE
  spdlog::spdlog
  argparse
  ${llvm_libs}
  LLVMSupport

  Boost::unordered
)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-comment)
# target_compile_options(
#   ${PROJECT_NAME} PRIVATE
#   "$<IF:$<STREQUAL:${CMAKE_CXX_COMPILER_ID},Clang>,-ferror-limit,-fmax-errors>=1024"
# )
