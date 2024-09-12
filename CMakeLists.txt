cmake_minimum_required(VERSION 3.15)
project(asio-sample CXX)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
  execute_process(COMMAND conan install ${CMAKE_CURRENT_SOURCE_DIR} --build=missing --output-folder=${CMAKE_BINARY_DIR})
endif()

include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)

find_package(Boost REQUIRED)

add_executable(${PROJECT_NAME} main.cc)
target_link_libraries(${PROJECT_NAME} boost::boost)
