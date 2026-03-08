#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

// Test with mixed options for input parsing
#define TEST_INPUT_ARGS(F)                                                     \
  F(output, o, "Output file", OPTION, {})                                      \
  F(verbose, v, "Verbose mode", FLAG, {})                                      \
  F(library, L, "Link library", JOINED, {"cuda", "stdc++"})

DEFINE_ARGS(InputTest, InputTable, TEST_INPUT_ARGS)

void TestPositionalInputs() {
  std::cout << "Test: Positional Inputs... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-o", "out.txt", "input1.txt", "input2.txt"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(InputTable);
  assert(parser.Parse(argc, argv));
  
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "out.txt");
  
  const auto& inputs = parser.GetInputs();
  assert(inputs.size() == 2);
  assert(inputs[0] == "input1.txt");
  assert(inputs[1] == "input2.txt");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestMixedOptionsAndInputs() {
  std::cout << "Test: Mixed Options and Inputs... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "file1.txt", "-v", "-Lcuda", "file2.txt", "file3.txt"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(InputTable);
  assert(parser.Parse(argc, argv));
  
  assert(parser.HasArg(OPT_verbose));
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "cuda");
  
  const auto& inputs = parser.GetInputs();
  assert(inputs.size() == 3);
  assert(inputs[0] == "file1.txt");
  assert(inputs[1] == "file2.txt");
  assert(inputs[2] == "file3.txt");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestUnknownOptionNotAllowed() {
  std::cout << "Test: Unknown Option (Not Allowed)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--unknown-opt"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(InputTable);
  // Should fail because unknown option is not allowed by default
  assert(!parser.Parse(argc, argv));
  
  const auto& unknown = parser.GetUnknown();
  assert(unknown.size() == 1);
  assert(unknown[0] == "--unknown-opt");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestUnknownOptionAllowed() {
  std::cout << "Test: Unknown Option (Allowed)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--fake-opt", "input.txt", "-v"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(InputTable);
  parser.SetAllowUnknown(true);
  
  // Should succeed even with unknown option
  assert(parser.Parse(argc, argv));
  
  const auto& unknown = parser.GetUnknown();
  assert(unknown.size() == 1);
  assert(unknown[0] == "--fake-opt");
  
  // "input.txt" should be treated as input, not as value of --fake-opt
  const auto& inputs = parser.GetInputs();
  assert(inputs.size() == 1);
  assert(inputs[0] == "input.txt");
  
  assert(parser.HasArg(OPT_verbose));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestInputsBeforeOptions() {
  std::cout << "Test: Inputs Before Options... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "input1.txt", "-o", "out.txt", "input2.txt"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(InputTable);
  assert(parser.Parse(argc, argv));
  
  const auto& inputs = parser.GetInputs();
  assert(inputs.size() == 2);
  assert(inputs[0] == "input1.txt");
  assert(inputs[1] == "input2.txt");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  TestPositionalInputs();
  TestMixedOptionsAndInputs();
  TestUnknownOptionNotAllowed();
  TestUnknownOptionAllowed();
  TestInputsBeforeOptions();
  std::cout << "All input/unknown option tests PASSED!" << std::endl;
  return 0;
}
