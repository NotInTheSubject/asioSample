cmake_minimum_required(VERSION 3.7.0)
project(asio-sample VERSION 0.1.0)

if (WIN32)
  # _WIN32_WINNT is using by asio lib
  # In this reason, let's define it
    macro(get_WIN32_WINNT version)
      if(CMAKE_SYSTEM_VERSION)
        set(ver ${CMAKE_SYSTEM_VERSION})
        string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
        string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
        # Check for Windows 10, b/c we'll need to convert to hex 'A'.
        if("${verMajor}" MATCHES "10")
            set(verMajor "A")
            string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
        endif()
        # Remove all remaining '.' characters.
        string(REPLACE "." "" ver ${ver})
        # Prepend each digit with a zero.
        string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
        set(${version} "0x${ver}")
        message("Defining WinNT version: _WIN32_WINNT=${ver}")
    endif()
  endmacro()

  get_WIN32_WINNT(ver)
  
  add_definitions(-D_WIN32_WINNT=${ver})
endif()

if (NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    execute_process(COMMAND conan install --build=missing ${CMAKE_CURRENT_SOURCE_DIR})
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include(CTest)
enable_testing()

add_subdirectory(source)

add_executable(sample main.cpp)
set_property(TARGET sample PROPERTY CXX_STANDARD 20)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_link_libraries(sample ${CONAN_LIBS})
