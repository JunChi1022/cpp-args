cmake_minimum_required(VERSION 3.10)
project(cpp_args_test)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Helper function to add a test with all required configuration
function(add_cpp_test TEST_NAME TEST_FILE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 30 LABELS "unit")
endfunction()

# Add all test executables
add_cpp_test(test_basic test/test_basic.cpp)
add_cpp_test(test_underscore_dash test/test_underscore_dash.cpp)
add_cpp_test(test_underscore_input test/test_underscore_input.cpp)
add_cpp_test(test_short_name test/test_short_name.cpp)
add_cpp_test(test_allowed_values test/test_allowed_values.cpp)
add_cpp_test(test_multiple_args test/test_multiple_args.cpp)
add_cpp_test(test_unknown_arg test/test_unknown_arg.cpp)
add_cpp_test(test_missing_value test/test_missing_value.cpp)
add_cpp_test(test_flag test/test_flag.cpp)
add_cpp_test(test_joined_option test/test_joined_option.cpp)
add_cpp_test(test_inputs_unknown test/test_inputs_unknown.cpp)
add_cpp_test(test_similar_option test/test_similar_option.cpp)
add_cpp_test(test_option_group test/test_option_group.cpp)
add_cpp_test(test_multiple_values test/test_multiple_values.cpp)
add_cpp_test(test_default_value test/test_default_value.cpp)
add_cpp_test(test_separate_or_joined test/test_separate_or_joined.cpp)
add_cpp_test(test_option_parsing test/test_option_parsing.cpp)
add_cpp_test(test_alias test/test_alias.cpp)
add_cpp_test(test_render test/test_render.cpp)
add_cpp_test(test_eq_join test/test_eq_join.cpp)
add_cpp_test(test_custom_error_stream test/test_custom_error_stream.cpp)
add_cpp_test(test_comma_list test/test_comma_list.cpp)
add_cpp_test(test_regression test/test_regression.cpp)

# Reference example application
add_subdirectory(example)
