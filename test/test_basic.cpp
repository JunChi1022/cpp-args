#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

#define TEST_OPTIONS(V)                                                        \
  V(port, p, "Server port number", SEPARATE, {})                                 \
  V(log_lvl, l, "Logging verbosity", SEPARATE, {"debug", "info"})                \
  V(output, o, "Output file", SEPARATE, {})

DEFINE_ARGS(TestOption, TestTable, TEST_OPTIONS)

void TestBasicParsing() {
  std::cout << "Test: Basic Parsing... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--port", "8080"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_port));
  assert(parser.GetArgValue(OPT_port) == "8080");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for Argument Parser" << std::endl;
  std::cout << "========================================" << std::endl;

  TestBasicParsing();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
