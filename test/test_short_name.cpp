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

void TestShortName() {
  std::cout << "Test: Short Name... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-p", "9000", "-o", "result.txt"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_port));
  assert(parser.GetArgValue(OPT_port) == "9000");
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "result.txt");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// Test FEAT_SHORT: single dash long option
void TestSingleDashLongOption() {
  #define SHORT_OPTS(F) \
    F(help, h, "Print help", FLAG | SHORT, {}) \
    F(verbose, v, "Verbose mode", FLAG | SHORT, {}) \
    F(output, o, "Output file", SEPARATE | SHORT, {})
  
  DEFINE_ARGS(ShortOpts, ShortOptsTable, SHORT_OPTS)
  
  ArgumentParser parser(ShortOptsTable);
  
  // Test single dash format for long options
  const char *argv[] = {"program", "-help", "-verbose"};
  int argc = 3;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  assert(parser.HasArg((int)OPT_help));
  assert(parser.HasArg((int)OPT_verbose));
  
  std::cout << "TestSingleDashLongOption PASSED" << std::endl;
}

// Test FEAT_SHORT with value
void TestSingleDashLongOptionWithValue() {
  #define SHORT_OPTS2(F) \
    F(output, o, "Output file", SEPARATE | EQ_JOIN | SHORT, {}) \
    F(level, l, "Log level", SEPARATE | EQ_JOIN | SHORT, {"debug", "info", "error"})
  
  DEFINE_ARGS(ShortOpts2, ShortOptsTable2, SHORT_OPTS2)
  
  ArgumentParser parser(ShortOptsTable2);
  
  // Test single dash format with value
  const char *argv[] = {"program", "-output=result.txt", "-level", "debug"};
  int argc = 4;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  assert(parser.HasArg((int)OPT_output));
  assert(parser.GetArgValue((int)OPT_output) == "result.txt");
  assert(parser.HasArg((int)OPT_level));
  assert(parser.GetArgValue((int)OPT_level) == "debug");
  
  std::cout << "TestSingleDashLongOptionWithValue PASSED" << std::endl;
}

// Test FEAT_SHORT mixed with double dash
void TestMixedSingleAndDoubleDash() {
  #define MIXED_DASH_OPTS(F) \
    F(help, h, "Print help", FLAG | SHORT, {}) \
    F(version, V, "Print version", FLAG, {})  // Normal option (double dash only)
  
  DEFINE_ARGS(MixedDashOpts, MixedDashTable, MIXED_DASH_OPTS)
  
  ArgumentParser parser(MixedDashTable);
  
  // Mix single dash and double dash formats
  const char *argv[] = {"program", "-help", "--version"};
  int argc = 3;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  assert(parser.HasArg((int)OPT_help));
  assert(parser.HasArg((int)OPT_version));
  
  std::cout << "TestMixedSingleAndDoubleDash PASSED" << std::endl;
}


int main() {
  std::cout << "Running Unit Tests for Argument Parser" << std::endl;
  std::cout << "========================================" << std::endl;

  TestShortName();
  TestSingleDashLongOption();
  TestSingleDashLongOptionWithValue();
  TestMixedSingleAndDoubleDash();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
