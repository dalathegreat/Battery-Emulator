# Include the directory with your source files
include_directories(${CMAKE_SOURCE_DIR}/Software/src/devboard ${CMAKE_SOURCE_DIR}/Software/src/devboard/utils . )

# Create a variable to store the list of test files
file(GLOB TEST_SOURCES utils/*.cpp)

# Loop through each test source file and create an executable
foreach(TEST_SOURCE ${TEST_SOURCES})
    # Extract the test name without extension
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    # Create an executable for the test
    add_executable(${TEST_NAME} ${TEST_SOURCE} test_lib.cpp)

    # Apply the target_compile_definitions for the test
    target_compile_definitions(${TEST_NAME} PRIVATE UNIT_TEST)

endforeach()
