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
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  // Should fail because --unknown is not defined
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// Test that invalid values for known options are NOT treated as unknown arguments
void TestInvalidValueNotUnknown() {
  std::cout << "Test: Invalid Value Not Treated As Unknown... ";

  int argc;
  const char *prog = "./test";
  // Use an allowed value first to establish baseline
  std::vector<std::string> args1 = {prog, "--log-lvl", "debug"};
  const char **argv1 = CreateArgs(args1, argc);

  ArgumentParser parser1(TestTable);
  assert(parser1.Parse(argc, argv1));
  assert(parser1.HasArg((int)OPT_log_lvl));
  assert(parser1.GetArgValue((int)OPT_log_lvl) == "debug");
  // Should have no unknown arguments
  assert(parser1.GetUnknown().empty());

  CleanupArgs(argv1, argc);

  // Now test with invalid value - should fail but NOT add to unknown list
  std::vector<std::string> args2 = {prog, "--log-lvl", "invalid_level"};
  const char **argv2 = CreateArgs(args2, argc);

  ArgumentParser parser2(TestTable);
  // Should fail because "invalid_level" is not in allowed values
  assert(!parser2.Parse(argc, argv2));
  
  // The option was recognized, so it should NOT be in unknown list
  // This is the key behavior we're testing
  assert(parser2.GetUnknown().empty());

  CleanupArgs(argv2, argc);
  std::cout << "PASSED" << std::endl;
}

// Test missing value for known option is NOT treated as unknown argument
void TestMissingValueNotUnknown() {
  std::cout << "Test: Missing Value Not Treated As Unknown... ";

  int argc;
  const char *prog = "./test";
  // Missing value for --port (SEPARATE option requires value)
  std::vector<std::string> args = {prog, "--port"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  // Should fail because --port requires a value
  assert(!parser.Parse(argc, argv));
  
  // The option was recognized, so it should NOT be in unknown list
  assert(parser.GetUnknown().empty());

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// Test equals join format with invalid value
void TestEqJoinInvalidValueNotUnknown() {
  std::cout << "Test: EqJoin Invalid Value Not Treated As Unknown... ";

  #define EQ_JOIN_OPTIONS(V) \
    V(log_lvl, l, "Logging level", SEPARATE | EQ_JOIN, {"debug", "info", "warn"})

  DEFINE_ARGS(EqJoinOption, EqJoinTable, EQ_JOIN_OPTIONS)

  int argc;
  const char *prog = "./test";
  // Using equals join format with invalid value
  std::vector<std::string> args = {prog, "--log-lvl=trace"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(EqJoinTable);
  // Should fail because "trace" is not in allowed values
  assert(!parser.Parse(argc, argv));
  
  // The option was recognized via equals join format, 
  // so it should NOT be in unknown list
  assert(parser.GetUnknown().empty());

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for Argument Parser" << std::endl;
  std::cout << "========================================" << std::endl;

  TestUnknownArgument();
  TestInvalidValueNotUnknown();
  TestMissingValueNotUnknown();
  TestEqJoinInvalidValueNotUnknown();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
