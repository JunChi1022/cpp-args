#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

#define TEST_OPTIONS(V)                                                        \
  V(port, p, "Server port number", {})                                         \
  V(log_lvl, l, "Logging verbosity", {"debug", "info"})                        \
  V(output, o, "Output file", {})

DEFINE_ARGUMENTS(TestOption, TestTable, TEST_OPTIONS)

void TestUnderscoreToDashConversion() {
  std::cout << "Test: Underscore to Dash Conversion... ";

  int argc;
  const char *prog = "./test";
  // User can input --log-lvl (with dash) even though macro uses log_lvl
  std::vector<std::string> args = {prog, "--log-lvl", "debug"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_log_lvl));
  assert(parser.GetArgValue(OPT_log_lvl) == "debug");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for Argument Parser" << std::endl;
  std::cout << "========================================" << std::endl;

  TestUnderscoreToDashConversion();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
