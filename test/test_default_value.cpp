#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

#define TEST_OPTIONS(V)                                                        \
  V(port, p, "Server port number", OPTION, {})                                 \
  V(log_lvl, l, "Logging verbosity", OPTION, {"debug", "info"})                \
  V(output, o, "Output file", OPTION, {})                                      \
  V(host, h, "Server host", OPTION, {})

DEFINE_ARGS(TestOption, TestTable, TEST_OPTIONS)

void TestDefaultValueWhenOptionNotProvided() {
  std::cout << "Test: Default Value When Option Not Provided... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--port", "8080"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  
  // port was provided, should return actual value
  assert(parser.GetArgValue(OPT_port, "default_port") == "8080");
  
  // log_lvl was not provided, should return default value
  assert(parser.GetArgValue(OPT_log_lvl, "warning") == "warning");
  
  // output was not provided, should return default value
  assert(parser.GetArgValue(OPT_output, "/tmp/output.txt") == "/tmp/output.txt");
  
  // host was not provided, should return empty string (default parameter)
  assert(parser.GetArgValue(OPT_host) == "");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestDefaultValueWithEmptyString() {
  std::cout << "Test: Default Value With Empty String... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--port", "8080"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  
  // Test with explicit empty string as default
  assert(parser.GetArgValue(OPT_log_lvl, "") == "");
  
  // Test with empty string when option exists but has no value
  assert(parser.GetArgValue(OPT_port, "") == "8080");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestBackwardCompatibility() {
  std::cout << "Test: Backward Compatibility (No Default Parameter)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  
  // Test that old code without default parameter still works
  assert(parser.GetArgValue(OPT_port) == "");
  assert(parser.GetArgValue(OPT_log_lvl) == "");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for GetArgValue Default Value" << std::endl;
  std::cout << "=================================================" << std::endl;

  TestDefaultValueWhenOptionNotProvided();
  TestDefaultValueWithEmptyString();
  TestBackwardCompatibility();

  std::cout << "=================================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
