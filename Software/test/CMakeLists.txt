add_executable(events_test ../src/devboard/utils/events_test.cpp test_lib.cpp)

target_compile_definitions(events_test PRIVATE UNIT_TEST)

target_link_libraries(events_test)  # Link to the library from devboard/utils
