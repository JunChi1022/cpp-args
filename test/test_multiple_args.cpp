#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

#define TEST_OPTIONS(V)                                                        \
  V(port, p, "Server port number", {})                                         \
  V(log_lvl, l, "Logging verbosity", {"debug", "info"})                        \
  V(output, o, "Output file", {})

DEFINE_ARGUMENTS(TestOption, TestTable, TEST_OPTIONS)

void TestMultipleArguments() {
  std::cout << "Test: Multiple Arguments... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog,    "--port", "3000",   "--log-lvl",
                                   "debug", "-o",     "out.txt"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_port));
  assert(parser.HasArg(OPT_log_lvl));
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_port) == "3000");
  assert(parser.GetArgValue(OPT_log_lvl) == "debug");
  assert(parser.GetArgValue(OPT_output) == "out.txt");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for Argument Parser" << std::endl;
  std::cout << "========================================" << std::endl;

  TestMultipleArguments();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
