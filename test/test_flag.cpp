#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

using namespace cppargs;

// Test with flags only - using unified DEFINE_ARGS macro
#define TEST_FLAGS_ONLY(F)                                                     \
  F(help, h, "print help info", FLAG, {})                                      \
  F(verbose, v, "enable verbose mode", FLAG, {})

DEFINE_ARGS(FlagOption1, FlagTable1, TEST_FLAGS_ONLY)

// Test with mixed options and flags - using unified DEFINE_ARGS macro
#define TEST_MIXED(F)                                                          \
  F(port2, p, "Server port number", SEPARATE, {})                                \
  F(log_lvl2, l, "Logging verbosity", SEPARATE, {"debug", "info"})               \
  F(help2, h, "print help info", FLAG, {})                                     \
  F(verbose2, v, "enable verbose mode", FLAG, {})

DEFINE_ARGS(MixedOption2, MixedTable2, TEST_MIXED)

void TestFlagOnly() {
  std::cout << "Test: Flag Only... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--help", "-v"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(FlagTable1);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_help));
  assert(parser.HasArg(OPT_verbose));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestMixedFlagsAndOptions() {
  std::cout << "Test: Mixed Flags and Options... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--help2", "--port2", "8080", "-v"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(MixedTable2);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_help2));
  assert(parser.HasArg(OPT_verbose2));
  assert(parser.HasArg(OPT_port2));
  assert(parser.GetArgValue(OPT_port2) == "8080");
  assert(!parser.HasArg(OPT_log_lvl2));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestFlagWithShortName() {
  std::cout << "Test: Flag With Short Name... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-h", "--verbose"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(FlagTable1);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_help));
  assert(parser.HasArg(OPT_verbose));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  TestFlagOnly();
  TestMixedFlagsAndOptions();
  TestFlagWithShortName();
  std::cout << "All flag tests PASSED!" << std::endl;
  return 0;
}
