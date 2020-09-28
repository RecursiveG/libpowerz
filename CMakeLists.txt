cmake_minimum_required(VERSION 3.11)
project(POWERZ VERSION 0.0.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -ggdb -O0")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

##
## Third party libraries
##
include(FetchContent)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG        7.0.3
)
FetchContent_Declare(
        backward-cpp
        GIT_REPOSITORY https://github.com/bombela/backward-cpp.git
        GIT_TAG        v1.5
)

FetchContent_MakeAvailable(fmt)
FetchContent_MakeAvailable(backward-cpp)

##
## libraries
##
add_library(powerz STATIC)
target_sources(powerz PRIVATE powerz/serial.cpp powerz/kt001.cpp)
target_include_directories(powerz PRIVATE include/powerz INTERFACE include)
target_link_libraries(powerz PRIVATE fmt)

##
## binaries
##
add_executable(repl demo/repl.cpp ${BACKWARD_ENABLE})
target_include_directories(repl PRIVATE demo)
target_link_libraries(repl PRIVATE -ldw powerz)
