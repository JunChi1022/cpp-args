#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

using namespace cppargs;

#define TEST_OPTIONS(V)                                                        \
  V(port, p, "Server port number", SEPARATE, {})                                 \
  V(log_lvl, l, "Logging verbosity", SEPARATE, {"debug", "info"})                \
  V(output, o, "Output file", SEPARATE, {})

DEFINE_ARGS(TestOption, TestTable, TEST_OPTIONS)

void TestUnknownArgument() {
  std::cout << "Test: Unknown Argument Detection... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--unknown", "value"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  // Should fail because --unknown is not defined
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for Argument Parser" << std::endl;
  std::cout << "========================================" << std::endl;

  TestUnknownArgument();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
