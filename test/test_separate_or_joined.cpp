#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

using namespace cppargs;

#define TEST_OPTIONS(V)                                                        \
  V(include, I, "Include directory", SEPARATE | JOINED, {})                   \
  V(library, L, "Library path", SEPARATE | JOINED, {})                        \
  V(define, D, "Macro definition", SEPARATE | JOINED, {})                     \
  V(output, o, "Output file", SEPARATE, {})                                      \
  V(verbose, v, "Verbose mode", FLAG, {})

DEFINE_ARGS(TestOption, TestTable, TEST_OPTIONS)

void TestSeparateOrJoined_SeparateForm() {
  std::cout << "Test: SEPARATE_OR_JOINED - Separate Form... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-I", "/usr/include", "-L", "/usr/lib"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  
  assert(parser.HasArg(OPT_include));
  assert(parser.GetArgValue(OPT_include) == "/usr/include");
  
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "/usr/lib");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestSeparateOrJoined_JoinedForm() {
  std::cout << "Test: SEPARATE_OR_JOINED - Joined Form... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-I/usr/include", "-L/usr/lib"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  
  assert(parser.HasArg(OPT_include));
  assert(parser.GetArgValue(OPT_include) == "/usr/include");
  
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "/usr/lib");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestSeparateOrJoined_MixedForm() {
  std::cout << "Test: SEPARATE_OR_JOINED - Mixed Form... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-I/usr/include", "-L", "/usr/lib", "-D", "DEBUG"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  
  assert(parser.HasArg(OPT_include));
  assert(parser.GetArgValue(OPT_include) == "/usr/include");
  
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "/usr/lib");
  
  assert(parser.HasArg(OPT_define));
  assert(parser.GetArgValue(OPT_define) == "DEBUG");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestSeparateOrJoined_MultipleValues() {
  std::cout << "Test: SEPARATE_OR_JOINED - Multiple Values... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-I/usr/include", "-I", "/usr/local/include", "-L/usr/lib"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  
  assert(parser.HasArg(OPT_include));
  auto includeValues = parser.GetAllArgValues(OPT_include);
  assert(includeValues.size() == 2);
  assert(includeValues[0] == "/usr/include");
  assert(includeValues[1] == "/usr/local/include");
  
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "/usr/lib");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestSeparateOrJoined_WithLongName() {
  std::cout << "Test: SEPARATE_OR_JOINED - Long Name with Equals... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--include=/usr/include", "--library=/usr/lib"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  
  assert(parser.HasArg(OPT_include));
  assert(parser.GetArgValue(OPT_include) == "/usr/include");
  
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "/usr/lib");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestSeparateOrJoined_ComplexScenario() {
  std::cout << "Test: SEPARATE_OR_JOINED - Complex Scenario... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {
    prog, 
    "-I/usr/include",           // joined form
    "-L", "/usr/lib",          // separate form
    "--define=DEBUG",          // long name with equals
    "-I", "/usr/local/include", // separate form
    "-o", "output.txt",        // regular option
    "-v",                       // flag
    "input.cpp"                 // positional argument
  };
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(TestTable);
  assert(parser.Parse(argc, argv));
  
  // Check include paths (multiple values)
  assert(parser.HasArg(OPT_include));
  auto includeValues = parser.GetAllArgValues(OPT_include);
  assert(includeValues.size() == 2);
  assert(includeValues[0] == "/usr/include");
  assert(includeValues[1] == "/usr/local/include");
  
  // Check library path
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "/usr/lib");
  
  // Check define
  assert(parser.HasArg(OPT_define));
  assert(parser.GetArgValue(OPT_define) == "DEBUG");
  
  // Check output
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "output.txt");
  
  // Check verbose flag
  assert(parser.HasArg(OPT_verbose));
  
  // Check positional input
  assert(parser.GetInputs().size() == 1);
  assert(parser.GetInputs()[0] == "input.cpp");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for SEPARATE | JOINED" << std::endl;
  std::cout << "================================================" << std::endl;

  TestSeparateOrJoined_SeparateForm();
  TestSeparateOrJoined_JoinedForm();
  TestSeparateOrJoined_MixedForm();
  TestSeparateOrJoined_MultipleValues();
  TestSeparateOrJoined_WithLongName();
  TestSeparateOrJoined_ComplexScenario();

  std::cout << "================================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
