################################################################################
#                                                                              #
# Project:      Filtering DNS Resolver                                         #
# University:   Faculty of Information Technology, BUT                         #
# Subject:      ISA: Network Applications and Network Administration           #
#                                                                              #
# File:         CMakeLists.txt                                                 #
# Author:       Jan Kalina <xkalinj00>                                         #
#                                                                              #
# Created:      23.09.2025                                                     #
# Last edit:    24.09.2025                                                     #
#                                                                              #
# Description:  This file contains the CMake configuration for the project     #
#               Filtering DNS Resolver for the ISA course. It sets up the      #
#               project, specifies the C++20 standard collects source files,   #
#               creates a static library, defines the main executable, and     #
#               configures testing. This 'CMakeLists.txt' is inspired by       #
#               'CMakeLists.txt' I created for previous projects at BUT FIT    #
#               (e.g., in IPK and ICP courses).                                #
#                                                                              #
################################################################################

################################################################################
#                                                                              #
#                        BASIC CMakeLists.txt SETTINGS                         #
#                                                                              #
################################################################################

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.15)

# Set the project name (source location) and version
project(src LANGUAGES C CXX)
set(EXECUTABLE "dns")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set compilation flags
set(WARNING_FLAGS -Wall -Wextra -Werror -pedantic -Wshadow -Wconversion -Wsign-conversion -Wnull-dereference)
set(SANITIZE_FLAGS -fsanitize=address -fsanitize=undefined)
set(THREAD_FLAGS -pthread)
set(RELEASE_FLAGS -O2 -DNDEBUG)
set(TEST_FLAGS -O1 -g3 -DTESTING)
set(DEBUG_FLAGS -O0 -g3 -DDEBUG)

# Default build type to Release if not specified
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(${RELEASE_FLAGS} ${THREAD_FLAGS})
    message(STATUS "Build type: RELEASE with '${RELEASE_FLAGS} ${THREAD_FLAGS}'")
elseif(CMAKE_BUILD_TYPE STREQUAL "Test")
    add_compile_options(${TEST_FLAGS} ${WARNING_FLAGS} ${THREAD_FLAGS} ${SANITIZE_FLAGS})
    add_link_options(${SANITIZE_FLAGS})
    message(STATUS "Build type: TEST with '${TEST_FLAGS} ${WARNING_FLAGS} ${THREAD_FLAGS} ${SANITIZE_FLAGS}'")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(${DEBUG_FLAGS} ${WARNING_FLAGS} ${THREAD_FLAGS} ${SANITIZE_FLAGS})
    add_link_options(${SANITIZE_FLAGS})
    message(STATUS "Build type: DEBUG with '${DEBUG_FLAGS} ${WARNING_FLAGS} ${THREAD_FLAGS} ${SANITIZE_FLAGS}'")
endif()


################################################################################
#                                                                              #
#                             BUILD CONFIGURATION                              #
#                                                                              #
################################################################################

###                                          ###
#  Platform independency & Basic build config  #
###                                          ###

# Platform-specific output directory (.exe vs. no suffix, Windows vs. Linux)
if(UNIX)
    set(PLATFORM_NAME "Linux")
    set(RUNTIME_EXT "")
elseif(WIN32)
    message(WARNING "Windows platform is not officially supported.")
endif()

# Ensure the existence of the build directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# Set the output directory for the app release binary (dns)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/..")
set(CMAKE_BIN_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# Include necessary directories (e.g., src)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/build)

###                                                              ###
#  Build library from project files (dns-lib) (except 'main.cpp')  #
###                                                              ###

# Collect all source files in the 'src' directory
file(GLOB_RECURSE SOURCES
     ${CMAKE_SOURCE_DIR}/src/*.cpp
     ${CMAKE_SOURCE_DIR}/src/*.h
     ${CMAKE_SOURCE_DIR}/src/*.hpp
     )

# Exclude main.cpp to avoid including it in the library
list(REMOVE_ITEM SOURCES ${CMAKE_SOURCE_DIR}/src/App/main.cpp)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
        )

# Create a static library from the source files
add_library(${EXECUTABLE}-lib STATIC ${SOURCES})

target_include_directories(${EXECUTABLE}-lib PUBLIC ${CMAKE_SOURCE_DIR}/src)


###                          ###
#  Build main executabe (dns)  #
###                          ###

add_executable(${EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/App/main.cpp)

target_link_libraries(${EXECUTABLE} PRIVATE ${EXECUTABLE}-lib)


################################################################################
#                                                                              #
#                              TEST CONFIGURATION                              #
#                                                                              #
################################################################################
#[[

enable_testing()

# Fetch Google Test framework
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        main
        )
FetchContent_MakeAvailable(googletest)

# Find all test source files
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS test/*.cpp test/*.hpp)

###                                 ###
#  Build individual test executables  #
###                                 ###

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME}
                          PRIVATE
                          ${EXECUTABLE}-lib
                          gtest
                          gtest_main
                          )
    target_link_options(${TEST_NAME} PRIVATE
                        -Wl,--wrap=getifaddrs
                        -Wl,--wrap=freeifaddrs
                        )
    set_target_properties(${TEST_NAME} PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test/bin"
                          )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

###                              ###
#  Build combined test executable  #
###                              ###

add_executable(${EXECUTABLE}-test ${TEST_SOURCES})
target_link_libraries(${EXECUTABLE}-test
                      PRIVATE
                      ${EXECUTABLE}-lib
                      gtest
                      gtest_main
                      )
target_link_options(${EXECUTABLE}-test PRIVATE
                    -Wl,--wrap=getifaddrs
                    -Wl,--wrap=freeifaddrs
                    )
set_target_properties(${EXECUTABLE}-test PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test/bin"
                      )
add_test(NAME ${EXECUTABLE}-test COMMAND ${EXECUTABLE}-test)
]]

### end of file CMakeLists.txt ###
