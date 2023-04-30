# The tests/ subdirectory is used for implementing tests. Each separate test
# will be in its own subdirectory under tests/, ending with suffix _test.

# Yes, globbing is terrible practice in CMake but I don't care.
file(GLOB test_dirs *_test)

foreach(test_dir ${test_dirs})
    add_subdirectory(${test_dir})
endforeach()
