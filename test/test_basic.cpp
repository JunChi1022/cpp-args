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

void TestHasArgMultipleIds() {
  std::cout << "Test: HasArg with Multiple IDs... ";

  // Test case 1: Only one option is present
  {
    int argc;
    const char *prog = "./test";
    std::vector<std::string> args = {prog, "--port", "8080"};
    char **argv = CreateArgs(args, argc);

    ArgumentParser parser(TestTable);
    assert(parser.Parse(argc, argv));
    
    // Should return true because OPT_port is present
    assert(parser.HasArg(OPT_port, OPT_log_lvl, OPT_output) == true);
    
    CleanupArgs(argv, argc);
  }

  // Test case 2: Two options are present
  {
    int argc;
    const char *prog = "./test";
    std::vector<std::string> args = {prog, "--port", "8080", "--log_lvl", "info"};
    char **argv = CreateArgs(args, argc);

    ArgumentParser parser(TestTable);
    assert(parser.Parse(argc, argv));
    
    // Should return true because both OPT_port and OPT_log_lvl are present
    assert(parser.HasArg(OPT_port, OPT_log_lvl, OPT_output) == true);
    
    CleanupArgs(argv, argc);
  }

  // Test case 3: No options are present
  {
    int argc;
    const char *prog = "./test";
    std::vector<std::string> args = {prog};
    char **argv = CreateArgs(args, argc);

    ArgumentParser parser(TestTable);
    assert(parser.Parse(argc, argv));
    
    // Should return false because none of the options are present
    assert(parser.HasArg(OPT_port, OPT_log_lvl, OPT_output) == false);
    
    CleanupArgs(argv, argc);
  }

  // Test case 4: Last option is present
  {
    int argc;
    const char *prog = "./test";
    std::vector<std::string> args = {prog, "--output", "result.txt"};
    char **argv = CreateArgs(args, argc);

    ArgumentParser parser(TestTable);
    assert(parser.Parse(argc, argv));
    
    // Should return true because OPT_output is present
    assert(parser.HasArg(OPT_port, OPT_log_lvl, OPT_output) == true);
    
    CleanupArgs(argv, argc);
  }

  // Test case 5: Single ID
  {
    int argc;
    const char *prog = "./test";
    std::vector<std::string> args = {prog, "--port", "8080"};
    char **argv = CreateArgs(args, argc);

    ArgumentParser parser(TestTable);
    assert(parser.Parse(argc, argv));
    
    // Single ID should still work (backward compatibility)
    assert(parser.HasArg(OPT_port) == true);
    assert(parser.HasArg(OPT_log_lvl) == false);
    
    CleanupArgs(argv, argc);
  }

  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for Argument Parser" << std::endl;
  std::cout << "========================================" << std::endl;

  TestBasicParsing();
  TestHasArgMultipleIds();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
