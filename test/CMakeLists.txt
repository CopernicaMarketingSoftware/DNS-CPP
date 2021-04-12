# Declare the test driver
add_executable(test-dnscpp
  stress.cpp
)

# The test driver links with dnscpp
target_link_libraries(test-dnscpp PRIVATE
  dnscpp
)

# @todo: use a proper test framework like googletest
# @todo: do something about the other files in this directory
