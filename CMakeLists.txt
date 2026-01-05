cmake_minimum_required(VERSION 3.26.4)

# enable_language(CXX)
project(rcx LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE RCX_SOURCE_FILES src/*.cxx)
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

include(ExternalProject)
ExternalProject_Add(argparse_ep
			PREFIX		${EXT_DIR_argparse}
			GIT_REPOSITORY	"https://github.com/p-ranav/argparse.git"
			GIT_TAG		master
			INSTALL_DIR 	${IST_DIR_argparse}
			CMAKE_ARGS
				-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
				-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
				-DCMAKE_INSTALL_PREFIX:PATH=${IST_DIR_argparse}
				-DCMAKE_CXX_STANDARD=17
				-DCMAKE_CXX_STANDARD_REQUIRED=ON
#			DEPENDS	${PROJECT_NAME}
			STEP_TARGETS build
			)

set_property(TARGET argparse_ep PROPERTY CXX_STANDARD 17)
#set_property(TARGET argparse_ep PROPERTY
#		    INTERFACE_INCLUDE_DIRECTORIES APPEND "${EXT_DIR_argparse}")
#set_property(TARGET argparse_ep PROPERTY
#		    IMPORTED_LOCATION "${IST_DIR_argparse}/${CMAKE_STATIC_LIBRARY_PREFIX}argparse${CMAKE_STATIC_LIBRARY_SUFFIX}")

ExternalProject_Get_Property(argparse_ep INSTALL_DIR)

#include_directories(${INSTALL_DIR}/include)
#link_directories(${INSTALL_DIR}/lib)
set(ARGPARSE_INCLUDE_DIR "${INSTALL_DIR}/include")
set(ARGPARSE_LIBRARY_DIR "${INSTALL_DIR}/lib")

add_dependencies(${PROJECT_NAME} argparse_ep)
add_dependencies(${PROJECT_NAME} spdlog)
add_definitions(${LLVM_DEFINITIONS})

target_include_directories(
  ${PROJECT_NAME} PRIVATE
  # includes
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${LLVM_INCLUDE_DIR}
  ${ARGPARSE_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/spdlog/include
  )
target_link_directories(${PROJECT_NAME} PRIVATE ${ARGPARSE_LIBRARY_DIR})

# llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE
  spdlog::spdlog
#  argparse
  ${llvm_libs}
  LLVMSupport
)
# target_compile_options(
#   ${PROJECT_NAME} PRIVATE
#   "$<IF:$<STREQUAL:${CMAKE_CXX_COMPILER_ID},Clang>,-ferror-limit,-fmax-errors>=1024"
# )
