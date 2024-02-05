# Include the directory with your source files
include_directories(${CMAKE_SOURCE_DIR}/Software/src/devboard/utils)

add_executable(events_test events_test.cpp test_lib.cpp)

target_compile_definitions(events_test PRIVATE UNIT_TEST)

target_link_libraries(events_test)  # Link to the library from devboard/utils

# Register your tests with CTest
add_test(NAME MyProjectTests COMMAND events_test)
