# Reference application for the header-only library.
# Built by default so it stays in sync with cpp_args.hpp changes.
add_executable(cpp_args_example main.cpp)

# Allow "options.h" -> "../cpp_args.hpp" to resolve regardless of build dir
target_include_directories(cpp_args_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)