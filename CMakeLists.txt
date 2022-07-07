# Bail out if it's an old CMake.
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

# Find a suitable compiler, and declare the version and language.
project(DNS-CPP VERSION 1.2.6 LANGUAGES CXX)

# All artefacts should end up at the root of the build dir for convenience
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Options for compilation
option(DNS-CPP_BUILD_TESTS "Build the tests" ON)

# Declare the dnscpp library.
# The source files to be compiled are defined in a separate subdirectory.
add_library(dnscpp "")

# We require C++11.
target_compile_features(dnscpp PUBLIC cxx_std_11)

# Disable GNU extensions, and define the .so version scheme.
set_target_properties(
  dnscpp PROPERTIES
    # No GNU stuff (otherwise the identifier `unix` would be a #define)
    CXX_EXTENSIONS OFF
    # Creates a symlink from libdnscpp.so to libdnscpp.so.x.y
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  )

# Declare the public include structure.
target_include_directories(dnscpp PUBLIC
  # At build time, this should be the include directory
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  # When installed, this should be the include directory
  $<INSTALL_INTERFACE:include>
)

# Declare what needs to be compiled
add_subdirectory(src)

# dnscpp links with the `resolv` library and with `ev`
# @todo: write proper FindResolv and FindEv cmake modules for this
target_link_libraries(dnscpp PUBLIC -lresolv -lev)

# This defines CMAKE_INSTALL_INCLUDEDIR and CMAKE_INSTALL_LIBDIR
include(GNUInstallDirs)

# Declare how the dnscpp library is to be installed.
# This library is as straightforward as it gets.
install(
  TARGETS dnscpp
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Declare how the public header files are to be installed.
install(
  DIRECTORY include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)

# If tests are enabled then compile the test driver
if(DNS-CPP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
