#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

// Test with joined options - like compiler flags
#define TEST_JOINED_OPTIONS(F)                                                 \
  F(library, L, "Link library", JOINED, {"cuda", "stdc++", "pthread"})         \
  F(include, I, "Include directory", JOINED, {})                               \
  F(output, o, "Output file", OPTION, {})                                      \
  F(verbose, v, "Verbose mode", FLAG, {})

DEFINE_ARGS(JoinedTest, JoinedTable, TEST_JOINED_OPTIONS)

void TestJoinedShortOption() {
  std::cout << "Test: Joined Short Option (-lcuda)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-Lcuda", "-I/usr/local/include"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(JoinedTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "cuda");
  assert(parser.HasArg(OPT_include));
  assert(parser.GetArgValue(OPT_include) == "/usr/local/include");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestJoinedLongOption() {
  std::cout << "Test: Joined Long Option (--library=cuda)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--library=cuda", "--include=/opt/include"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(JoinedTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "cuda");
  assert(parser.HasArg(OPT_include));
  assert(parser.GetArgValue(OPT_include) == "/opt/include");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestMixedJoinedAndRegular() {
  std::cout << "Test: Mixed Joined and Regular Options... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-Lcuda", "--output", "app.out", "-v"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(JoinedTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "cuda");
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "app.out");
  assert(parser.HasArg(OPT_verbose));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestJoinedWithInvalidValue() {
  std::cout << "Test: Joined Option With Invalid Value... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-Linvalid"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(JoinedTable);
  // Should fail because "invalid" is not in allowed values
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  TestJoinedShortOption();
  TestJoinedLongOption();
  TestMixedJoinedAndRegular();
  TestJoinedWithInvalidValue();
  std::cout << "All joined option tests PASSED!" << std::endl;
  return 0;
}
