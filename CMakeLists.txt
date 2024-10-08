cmake_minimum_required(VERSION 3.14) # 3.1 requred for target sources

# use cpp11
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

Project(Mapper LANGUAGES CXX)

enable_testing()
add_subdirectory(lib/googletest)
add_executable(mapper-test src/MapTest.cpp src/Mapper.h src/Mapper.cpp src/Map.h src/Map.cpp src/ConcurrentMap.cpp src/ConcurrentMap.h src/Semaphore.cpp src/Semaphore.h)
target_link_libraries(mapper-test gtest gtest_main pthread)
include(GoogleTest)
gtest_discover_tests(mapper-test)

add_executable(mapper src/MapperCli.cpp src/Mapper.h src/Mapper.cpp src/Map.cpp src/Map.h src/ConcurrentMap.cpp src/ConcurrentMap.h src/Semaphore.cpp src/Semaphore.h)
target_link_libraries(mapper pthread)
