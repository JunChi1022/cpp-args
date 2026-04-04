#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>
#include <sstream>

using namespace cppargs;

#define TEST_OPTIONS(V)                                                        \
  V(port, p, "Server port number", SEPARATE, {})                                 \
  V(log_lvl, l, "Logging verbosity", SEPARATE, {"debug", "info"})                \
  V(output, o, "Output file", SEPARATE, {})

DEFINE_ARGS(TestOption, TestTable, TEST_OPTIONS)

void TestCustomErrorStream() {
  std::cout << "Test: Custom Error Stream... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--log-lvl", "invalid"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  
  // Set custom error stream
  std::ostringstream errStream;
  parser.SetErrMsgStream(&errStream);
  
  // Parse should fail due to invalid value
  assert(!parser.Parse(argc, argv));
  
  // Check that error message was written to custom stream
  std::string errorMsg = errStream.str();
  std::cout << "\n  Error message: '" << errorMsg << "'" << std::endl;
  
  // Verify error message content
  assert(errorMsg.find("Invalid value 'invalid'") != std::string::npos);
  
  // Verify normalized option name (with '-' not '_')
  assert(errorMsg.find("--log-lvl") != std::string::npos);
  assert(errorMsg.find("--log_lvl") == std::string::npos); // Should NOT contain underscore
  
  // Verify no "Error: " prefix
  assert(errorMsg.find("Error: ") == std::string::npos);
  
  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestDefaultErrorStream() {
  std::cout << "Test: Default Error Stream (std::cerr)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--log-lvl", "invalid"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  // Don't set custom stream - should use std::cerr by default
  
  // Parse should fail due to invalid value
  assert(!parser.Parse(argc, argv));
  
  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestMissingValueError() {
  std::cout << "Test: Missing Value Error with Custom Stream... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--log-lvl"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  
  // Set custom error stream
  std::ostringstream errStream;
  parser.SetErrMsgStream(&errStream);
  
  // Parse should fail due to missing value
  assert(!parser.Parse(argc, argv));
  
  // Check error message
  std::string errorMsg = errStream.str();
  std::cout << "\n  Error message: '" << errorMsg << "'" << std::endl;
  
  assert(errorMsg.find("Missing value for") != std::string::npos);
  
  // Verify normalized option name (with '-' not '_')
  assert(errorMsg.find("--log-lvl") != std::string::npos);
  assert(errorMsg.find("--log_lvl") == std::string::npos); // Should NOT contain underscore
  
  // Verify no "Error: " prefix
  assert(errorMsg.find("Error: ") == std::string::npos);
  
  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestUnknownArgError() {
  std::cout << "Test: Unknown Argument Error with Custom Stream... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--unknown-option"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  
  // Set custom error stream
  std::ostringstream errStream;
  parser.SetErrMsgStream(&errStream);
  
  // Parse should fail due to unknown argument
  assert(!parser.Parse(argc, argv));
  
  // Check error message
  std::string errorMsg = errStream.str();
  assert(errorMsg.find("Unknown argument") != std::string::npos);
  assert(errorMsg.find("--unknown-option") != std::string::npos);
  
  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestNullErrorStream() {
  std::cout << "Test: Null Stream Falls Back to std::cerr... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--log-lvl", "invalid"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  
  // Set null stream - should fall back to std::cerr
  parser.SetErrMsgStream(nullptr);
  
  // Parse should fail due to invalid value
  assert(!parser.Parse(argc, argv));
  
  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for Custom Error Stream" << std::endl;
  std::cout << "========================================" << std::endl;

  TestCustomErrorStream();
  TestDefaultErrorStream();
  TestMissingValueError();
  TestUnknownArgError();
  TestNullErrorStream();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
