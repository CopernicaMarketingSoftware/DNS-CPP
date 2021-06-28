# Declare executables
add_executable(stress stress.cpp)
add_executable(lookup lookup.cpp)
add_executable(reverse reverse.cpp)
add_executable(hosts hosts.cpp)

# Declare all deps
target_link_libraries(stress PRIVATE dnscpp)
target_link_libraries(lookup PRIVATE dnscpp)
target_link_libraries(reverse PRIVATE dnscpp)
target_link_libraries(hosts PRIVATE dnscpp)

# Find googletest
find_package(GTest REQUIRED)

# googletest requires threads
find_package(Threads REQUIRED)

# declare test driver executable
add_executable(test-dnscpp
  test_loopback.cpp
)

# add path to googletest's include directory
target_include_directories(test-dnscpp PRIVATE ${GTEST_INCLUDE_DIRS})

# link with necessary libraries
target_link_libraries(test-dnscpp PRIVATE dnscpp ${GTEST_MAIN_LIBRARIES} ${GTEST_LIBRARIES} Threads::Threads)
