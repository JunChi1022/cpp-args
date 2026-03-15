#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace cppargs;

// Define TEST macro for simple function declaration
#define TEST(name) void name()

// Test EQ_JOIN feature - equals format only
void TestEqJoin_Basic();
void TestEqJoin_WithShort();
void TestSeparate_NoEquals();
void TestSeparate_WithSpace();
void TestSeparateOrEqJoin_BothFormats();
void TestRender_EqJoin();
void TestRender_EqJoinWithShort();
void TestRender_SeparateOrEqJoin();

int main() {
  std::cout << "Running Unit Tests for EQ_JOIN Feature" << std::endl;
  std::cout << "========================================" << std::endl;
  
  TestEqJoin_Basic();
  TestEqJoin_WithShort();
  TestSeparate_NoEquals();
  TestSeparate_WithSpace();
  TestSeparateOrEqJoin_BothFormats();
  TestRender_EqJoin();
  TestRender_EqJoinWithShort();
  TestRender_SeparateOrEqJoin();
  
  std::cout << "========================================" << std::endl;
  std::cout << "All EQ_JOIN tests PASSED!" << std::endl;
  
  return 0;
}

// Test EQ_JOIN feature - equals format only
TEST(TestEqJoin_Basic) {
  #define ARGS(F)                                                            \
    F(output, o, "Output file", EQ_JOIN, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "--output=result.txt"            // equals format
  }, argc);
  
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "result.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "EQ_JOIN basic test PASSED" << std::endl;
  
  #undef ARGS
}

// Test EQ_JOIN with short option
TEST(TestEqJoin_WithShort) {
  #define ARGS(F)                                                            \
    F(output, o, "Output file", EQ_JOIN | SHORT, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "-o=result.txt"                  // short with equals
  }, argc);
  
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "result.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "EQ_JOIN with short test PASSED" << std::endl;
  
  #undef ARGS
}

// Test SEPARATE does NOT support equals format
TEST(TestSeparate_NoEquals) {
  #define ARGS(F)                                                            \
    F(output, o, "Output file", SEPARATE, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "--output=result.txt"            // equals format should FAIL for SEPARATE
  }, argc);
  
  // This should fail because SEPARATE doesn't support equals
  bool result = parser.Parse(argc, argv);
  assert(result == false);
  
  CleanupArgs(argv, argc);
  std::cout << "SEPARATE rejects equals format PASSED" << std::endl;
  
  #undef ARGS
}

// Test SEPARATE supports space format
TEST(TestSeparate_WithSpace) {
  #define ARGS(F)                                                            \
    F(output, o, "Output file", SEPARATE, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "--output", "result.txt"         // space format
  }, argc);
  
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "result.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "SEPARATE with space test PASSED" << std::endl;
  
  #undef ARGS
}

// Test SEPARATE | EQ_JOIN supports both formats
TEST(TestSeparateOrEqJoin_BothFormats) {
  #define ARGS(F)                                                            \
    F(output, o, "Output file", SEPARATE | EQ_JOIN, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  // Test space format
  ArgumentParser parser1(AppTable);
  int argc1;
  char **argv1 = CreateArgs({
    "program",
    "--output", "result1.txt"        // space format
  }, argc1);
  
  assert(parser1.Parse(argc1, argv1));
  assert(parser1.GetArgValue(OPT_output) == "result1.txt");
  CleanupArgs(argv1, argc1);
  
  // Test equals format
  ArgumentParser parser2(AppTable);
  int argc2;
  char **argv2 = CreateArgs({
    "program",
    "--output=result2.txt"           // equals format
  }, argc2);
  
  assert(parser2.Parse(argc2, argv2));
  assert(parser2.GetArgValue(OPT_output) == "result2.txt");
  CleanupArgs(argv2, argc2);
  
  std::cout << "SEPARATE | EQ_JOIN supports both formats PASSED" << std::endl;
  
  #undef ARGS
}

// Test Render with EQ_JOIN
TEST(TestRender_EqJoin) {
  #define ARGS(F)                                                            \
    F(output, o, "Output file", EQ_JOIN, {})                                 \
    F(input, i, "Input file", SEPARATE, {})                                  \
    F(verbose, v, "Verbose", FLAG, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "--output=result.txt",
    "--input", "input.txt",
    "--verbose"
  }, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render EQ_JOIN option - should use equals format
  std::vector<std::string> rendered;
  parser.Render(OPT_output, rendered);
  assert(rendered.size() == 1);
  assert(rendered[0] == "--output=result.txt");
  
  // Render SEPARATE option - should use space format
  rendered.clear();
  parser.Render(OPT_input, rendered);
  assert(rendered.size() == 2);
  assert(rendered[0] == "--input");
  assert(rendered[1] == "input.txt");
  
  // Render FLAG option
  rendered.clear();
  parser.Render(OPT_verbose, rendered);
  assert(rendered.size() == 1);
  assert(rendered[0] == "--verbose");
  
  CleanupArgs(argv, argc);
  std::cout << "Render EQ_JOIN test PASSED" << std::endl;
  
  #undef ARGS
}

// Test Render with EQ_JOIN | SHORT
TEST(TestRender_EqJoinWithShort) {
  #define ARGS(F)                                                            \
    F(output, o, "Output file", EQ_JOIN | SHORT, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "-o=result.txt"
  }, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render EQ_JOIN|SHORT - should use single dash with equals
  std::vector<std::string> rendered;
  parser.Render(OPT_output, rendered);
  assert(rendered.size() == 1);
  assert(rendered[0] == "-output=result.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "Render EQ_JOIN|SHORT test PASSED" << std::endl;
  
  #undef ARGS
}

// Test Render with SEPARATE | EQ_JOIN (prefers space format)
TEST(TestRender_SeparateOrEqJoin) {
  #define ARGS(F)                                                            \
    F(output, o, "Output file", SEPARATE | EQ_JOIN, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "--output=result.txt"
  }, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render SEPARATE|EQ_JOIN - should prefer space format
  std::vector<std::string> rendered;
  parser.Render(OPT_output, rendered);
  assert(rendered.size() == 2);
  assert(rendered[0] == "--output");
  assert(rendered[1] == "result.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "Render SEPARATE|EQ_JOIN test PASSED" << std::endl;
  
  #undef ARGS
}
