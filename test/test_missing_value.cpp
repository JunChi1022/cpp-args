#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

#define TEST_OPTIONS(V)                                                        \
  V(port, p, "Server port number", OPTION, {})                                 \
  V(log_lvl, l, "Logging verbosity", OPTION, {"debug", "info"})                \
  V(output, o, "Output file", OPTION, {})

DEFINE_ARGS(TestOption, TestTable, TEST_OPTIONS)

void TestMissingValue() {
  std::cout << "Test: Missing Value Detection... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--port"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  // Should fail because --port requires a value
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for Argument Parser" << std::endl;
  std::cout << "========================================" << std::endl;

  TestMissingValue();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
