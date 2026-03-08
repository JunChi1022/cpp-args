cmake_minimum_required(VERSION 3.10)
project(cpp_args_test)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Add individual test executables
add_executable(test_basic test/test_basic.cpp)
add_executable(test_underscore_dash test/test_underscore_dash.cpp)
add_executable(test_underscore_input test/test_underscore_input.cpp)
add_executable(test_short_name test/test_short_name.cpp)
add_executable(test_allowed_values test/test_allowed_values.cpp)
add_executable(test_multiple_args test/test_multiple_args.cpp)
add_executable(test_unknown_arg test/test_unknown_arg.cpp)
add_executable(test_missing_value test/test_missing_value.cpp)

# Register each test with CTest
add_test(NAME BasicParsing COMMAND test_basic)
add_test(NAME UnderscoreToDashConversion COMMAND test_underscore_dash)
add_test(NAME UnderscoreInput COMMAND test_underscore_input)
add_test(NAME ShortName COMMAND test_short_name)
add_test(NAME AllowedValues COMMAND test_allowed_values)
add_test(NAME MultipleArguments COMMAND test_multiple_args)
add_test(NAME UnknownArgument COMMAND test_unknown_arg)
add_test(NAME MissingValue COMMAND test_missing_value)

# Set test properties
set_tests_properties(
    BasicParsing
    UnderscoreToDashConversion
    UnderscoreInput
    ShortName
    AllowedValues
    MultipleArguments
    UnknownArgument
    MissingValue
    PROPERTIES
    TIMEOUT 30
    LABELS "unit"
)
