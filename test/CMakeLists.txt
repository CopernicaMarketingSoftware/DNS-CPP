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

# @todo: use a proper test framework like googletest
