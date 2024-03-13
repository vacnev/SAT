cmake_minimum_required(VERSION 3.1...3.28)

project(fousaty)

set(CMAKE_CXX_STANDARD 17)
set(CXX_OPTIONS -Wall -Wextra -Werror)
set(CXX_DEBUG_OPTIONS -g)
set(CXX_RELEASE_OPTIONS -O2)

set(FOUSATY_LIBS
		src/solver.cpp)

add_executable(fousaty app/main.cpp)
add_library(fousaty-static STATIC ${FOUSATY_LIBS})

target_include_directories(fousaty PUBLIC src/)
target_link_libraries(fousaty fousaty-static)
