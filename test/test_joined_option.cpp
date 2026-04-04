#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

using namespace cppargs;

// Test with joined options - like compiler flags
#define TEST_JOINED_OPTIONS(F)                                                 \
  F(library, L, "Link library", JOINED, {"cuda", "stdc++", "pthread"})         \
  F(include, I, "Include directory", JOINED, {})                               \
  F(output, o, "Output file", SEPARATE, {})                                      \
  F(verbose, v, "Verbose mode", FLAG, {})

DEFINE_ARGS(JoinedTest, JoinedTable, TEST_JOINED_OPTIONS)

void TestJoinedShortOption();
void TestJoinedLongOption();
void TestMixedJoinedAndRegular();
void TestJoinedWithInvalidValue();
void TestJoinedMissingValue();

int main() {
  TestJoinedShortOption();
  TestJoinedLongOption();
  TestMixedJoinedAndRegular();
  TestJoinedWithInvalidValue();
  TestJoinedMissingValue();
  std::cout << "All joined option tests PASSED!" << std::endl;
  return 0;
}

void TestJoinedShortOption() {
  std::cout << "Test: Joined Short Option (-Lcuda)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-Lcuda", "-I/usr/local/include"};
  const char **argv = CreateArgs(args, argc);

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
  std::cout << "Test: Joined Long Option (should not support long form with value)... ";

  // JOINED 类型不支持长格式带值，这个测试应该失败
  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--library=cuda"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(JoinedTable);
  // JOINED 类型的长格式带等号不应该被解析成功
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestMixedJoinedAndRegular() {
  std::cout << "Test: Mixed Joined and Regular Options... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-Lcuda", "--output", "app.out", "-v"};
  const char **argv = CreateArgs(args, argc);

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
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(JoinedTable);
  // Should fail because "invalid" is not in allowed values
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestJoinedMissingValue() {
  std::cout << "Test: Joined Option Missing Value... ";

  // Test short option without value: -L
  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-L"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(JoinedTable);
  assert(!parser.Parse(argc, argv));
  assert(parser.GetUnknown().empty()); // Should not be in unknown list

  CleanupArgs(argv, argc);

  // Test long option without value: --library
  const char *prog2 = "./test";
  std::vector<std::string> args2 = {prog2, "--library"};
  const char **argv2 = CreateArgs(args2, argc);

  ArgumentParser parser2(JoinedTable);
  assert(!parser2.Parse(argc, argv2));
  assert(parser2.GetUnknown().empty()); // Should not be in unknown list

  CleanupArgs(argv2, argc);

  // Test include option without value: -I
  const char *prog3 = "./test";
  std::vector<std::string> args3 = {prog3, "-I"};
  const char **argv3 = CreateArgs(args3, argc);

  ArgumentParser parser3(JoinedTable);
  assert(!parser3.Parse(argc, argv3));
  assert(parser3.GetUnknown().empty()); // Should not be in unknown list

  CleanupArgs(argv3, argc);

  std::cout << "PASSED" << std::endl;
}
