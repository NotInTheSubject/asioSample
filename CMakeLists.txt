cmake_minimum_required(VERSION 3.7.0)
project(asio-sample VERSION 0.1.0)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    execute_process(COMMAND conan install ${CMAKE_CURRENT_SOURCE_DIR})
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include(CTest)
enable_testing()

add_executable(sample main.cpp)
set_property(TARGET sample PROPERTY CXX_STANDARD 17)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_link_libraries(sample ${CONAN_LIBS})
