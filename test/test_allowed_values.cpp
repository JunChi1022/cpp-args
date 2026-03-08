#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

#define TEST_OPTIONS(V)                                                        \
  V(port, p, "Server port number", {})                                         \
  V(log_lvl, l, "Logging verbosity", {"debug", "info"})                        \
  V(output, o, "Output file", {})

DEFINE_ARGS(TestOption, TestTable, TEST_OPTIONS, EMPTY_MACRO)

void TestAllowedValues() {
  std::cout << "Test: Allowed Values Validation... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--log-lvl", "invalid"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  // Should fail because "invalid" is not in {"debug", "info"}
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for Argument Parser" << std::endl;
  std::cout << "========================================" << std::endl;

  TestAllowedValues();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
