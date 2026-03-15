#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace cppargs;

#define TEST(name) void name()
#define RUN_TEST(name)                                                       \
  do {                                                                       \
    std::cout << "Running " << #name << "... ";                              \
    name();                                                                  \
    std::cout << "PASSED" << std::endl;                                      \
  } while (0)

// Test Render with SEPARATE options
TEST(TestRender_SeparateOptions) {
  #define ARGS(F)                                                            \
    F(port, p, "Server port", SEPARATE, {})                                  \
    F(output, o, "Output file", SEPARATE, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({"program", "--port", "8080", "--output", "result.txt"}, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render the options
  std::vector<std::string> rendered;
  parser.Render(OPT_port, rendered);
  parser.Render(OPT_output, rendered);
  
  // Should render as --option value format (space-separated)
  assert(rendered.size() == 4);  // 2 options * 2 args each
  assert(rendered[0] == "--port");
  assert(rendered[1] == "8080");
  assert(rendered[2] == "--output");
  assert(rendered[3] == "result.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "SEPARATE options render correctly" << std::endl;
  
  #undef ARGS
}

// Test Render with JOINED options
TEST(TestRender_JoinedOptions) {
  #define ARGS(F)                                                            \
    F(library, L, "Library path", JOINED, {})                                \
    F(include, I, "Include path", JOINED, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({"program", "-Lcuda", "-I/usr/include"}, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render the options
  std::vector<std::string> rendered;
  parser.Render(OPT_library, rendered);
  parser.Render(OPT_include, rendered);
  
  // Should render as --optionvalue format (joined, no space or equals)
  assert(rendered.size() == 2);
  assert(rendered[0] == "--librarycuda");
  assert(rendered[1] == "--include/usr/include");
  
  CleanupArgs(argv, argc);
  std::cout << "JOINED options render correctly" << std::endl;
  
  #undef ARGS
}

// Test Render with SEPARATE | JOINED options
TEST(TestRender_SeparateOrJoinedOptions) {
  #define ARGS(F)                                                            \
    F(include, I, "Include directory", SEPARATE | JOINED, {})                \
    F(define, D, "Macro definition", SEPARATE | JOINED, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "-I/usr/include",           // joined form
    "-D", "DEBUG",              // separate form
    "--define=RELEASE"          // long name with equals
  }, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render the options (SEPARATE|JOINED renders as SEPARATE by default)
  std::vector<std::string> rendered;
  parser.Render(OPT_include, rendered);
  parser.Render(OPT_define, rendered);
  
  // Should render all occurrences in --option value format (space-separated)
  // 1 include value + 2 define values = 3 values -> 6 args total
  assert(rendered.size() == 6);
  assert(rendered[0] == "--include");
  assert(rendered[1] == "/usr/include");
  assert(rendered[2] == "--define");
  assert(rendered[3] == "DEBUG");
  assert(rendered[4] == "--define");
  assert(rendered[5] == "RELEASE");
  
  CleanupArgs(argv, argc);
  std::cout << "SEPARATE | JOINED options render correctly" << std::endl;
  
  #undef ARGS
}

// Test Render with SHORT feature options
TEST(TestRender_ShortFeatureOptions) {
  #define ARGS(F)                                                            \
    F(help, h, "Print help", FLAG | SHORT, {})                               \
    F(verbose, v, "Verbose mode", FLAG | SHORT, {})                          \
    F(output, o, "Output file", SEPARATE | SHORT, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "-help",                        // single dash flag
    "-verbose",                     // single dash flag
    "-output=result.txt"            // single dash with value
  }, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render the options
  std::vector<std::string> rendered;
  parser.Render(OPT_help, rendered);
  parser.Render(OPT_verbose, rendered);
  parser.Render(OPT_output, rendered);
  
  // FEAT_SHORT options should render with single dash format
  // 2 flags + 1 separate option = 1 + 1 + 2 = 4 args
  assert(rendered.size() == 4);
  assert(rendered[0] == "-help");
  assert(rendered[1] == "-verbose");
  assert(rendered[2] == "-output");
  assert(rendered[3] == "result.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "SHORT feature options render correctly" << std::endl;
  
  #undef ARGS
}

// Test Render with multiple values for the same option
TEST(TestRender_MultipleValues) {
  #define ARGS(F)                                                            \
    F(config, c, "Config file", SEPARATE, {})                                \
    F(input, i, "Input file", SEPARATE | JOINED, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "-c", "config1.txt",
    "-c", "config2.txt",
    "-c", "config3.txt",
    "-iinput1.txt",
    "-i", "input2.txt"
  }, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render config option (multiple values)
  std::vector<std::string> rendered;
  parser.Render(OPT_config, rendered);
  
  // Should render all three occurrences as space-separated pairs
  assert(rendered.size() == 6);  // 3 pairs of --config + value
  assert(rendered[0] == "--config");
  assert(rendered[1] == "config1.txt");
  assert(rendered[2] == "--config");
  assert(rendered[3] == "config2.txt");
  assert(rendered[4] == "--config");
  assert(rendered[5] == "config3.txt");
  
  // Render input option (multiple values, mixed format)
  rendered.clear();
  parser.Render(OPT_input, rendered);
  
  // SEPARATE|JOINED renders as space-separated by default
  assert(rendered.size() == 4);  // 2 pairs of --input + value
  assert(rendered[0] == "--input");
  assert(rendered[1] == "input1.txt");
  assert(rendered[2] == "--input");
  assert(rendered[3] == "input2.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "Multiple values render correctly" << std::endl;
  
  #undef ARGS
}

// Test Render with FLAG options
TEST(TestRender_FlagOptions) {
  #define ARGS(F)                                                            \
    F(verbose, v, "Verbose", FLAG, {})                                       \
    F(debug, d, "Debug", FLAG, {})                                           \
    F(quiet, q, "Quiet", FLAG, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "--verbose",
    "-d",
    "--quiet"
  }, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render the flags
  std::vector<std::string> rendered;
  parser.Render(OPT_verbose, rendered);
  parser.Render(OPT_debug, rendered);
  parser.Render(OPT_quiet, rendered);
  
  // Flags should render without values
  assert(rendered.size() == 3);
  assert(rendered[0] == "--verbose");
  assert(rendered[1] == "--debug");
  assert(rendered[2] == "--quiet");
  
  CleanupArgs(argv, argc);
  std::cout << "FLAG options render correctly" << std::endl;
  
  #undef ARGS
}

// Test Render with option not specified (should not add anything)
TEST(TestRender_OptionNotSpecified) {
  #define ARGS(F)                                                            \
    F(port, p, "Server port", SEPARATE, {})                                  \
    F(output, o, "Output file", SEPARATE, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({"program", "--port", "8080"}, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render output option which was not specified
  std::vector<std::string> rendered;
  parser.Render(OPT_output, rendered);
  
  // Should remain empty since output was not specified
  assert(rendered.empty());
  
  CleanupArgs(argv, argc);
  std::cout << "Unspecified option renders nothing" << std::endl;
  
  #undef ARGS
}

// Test Render with complex scenario mixing all types
TEST(TestRender_ComplexScenario) {
  #define ARGS(F)                                                            \
    F(include, I, "Include directory", SEPARATE | JOINED, {})                \
    F(library, L, "Library path", SEPARATE | JOINED, {})                     \
    F(define, D, "Macro definition", SEPARATE | JOINED, {})                  \
    F(verbose, v, "Verbose mode", FLAG | SHORT, {})                          \
    F(output, o, "Output file", SEPARATE | SHORT, {})                        \
    F(config, c, "Config files", SEPARATE, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  char **argv = CreateArgs({
    "program",
    "-I/usr/include",              // joined
    "-I", "/usr/local/include",    // separate
    "-Lcuda",                      // joined
    "-L", "/opt/lib",              // separate
    "--define=DEBUG",              // long with equals
    "-v",                          // short flag
    "-o", "output.txt",            // short separate
    "-c", "config1.txt",           // multiple values
    "-c", "config2.txt"
  }, argc);
  
  assert(parser.Parse(argc, argv));
  
  // Render all options
  std::vector<std::string> rendered;
  parser.Render(OPT_include, rendered);   // 2 values -> 4 args (space-separated)
  parser.Render(OPT_library, rendered);   // 2 values -> 4 args (space-separated)
  parser.Render(OPT_define, rendered);    // 1 value -> 2 args (space-separated)
  parser.Render(OPT_verbose, rendered);   // 1 flag -> 1 arg
  parser.Render(OPT_output, rendered);    // 1 value -> 2 args (space-separated)
  parser.Render(OPT_config, rendered);    // 2 values -> 4 args (space-separated)
  
  // Total: 4 + 4 + 2 + 1 + 2 + 4 = 17 arguments
  assert(rendered.size() == 17);
  
  // Verify each rendered argument
  size_t idx = 0;
  // Include
  assert(rendered[idx++] == "--include");
  assert(rendered[idx++] == "/usr/include");
  assert(rendered[idx++] == "--include");
  assert(rendered[idx++] == "/usr/local/include");
  // Library
  assert(rendered[idx++] == "--library");
  assert(rendered[idx++] == "cuda");
  assert(rendered[idx++] == "--library");
  assert(rendered[idx++] == "/opt/lib");
  // Define
  assert(rendered[idx++] == "--define");
  assert(rendered[idx++] == "DEBUG");
  // Verbose (FLAG|SHORT renders as -verbose)
  assert(rendered[idx++] == "-verbose");
  // Output (SEPARATE|SHORT renders as -output value)
  assert(rendered[idx++] == "-output");
  assert(rendered[idx++] == "output.txt");
  // Config
  assert(rendered[idx++] == "--config");
  assert(rendered[idx++] == "config1.txt");
  assert(rendered[idx++] == "--config");
  assert(rendered[idx++] == "config2.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "Complex scenario renders correctly" << std::endl;
  
  #undef ARGS
}

// Test Render parse equivalence - verify that parsing rendered output gives same result
TEST(TestRender_ParseEquivalence) {
  #define ARGS(F)                                                            \
    F(include, I, "Include directory", SEPARATE | JOINED, {})                \
    F(define, D, "Macro definition", SEPARATE | JOINED, {})                  \
    F(verbose, v, "Verbose", FLAG | SHORT, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  // Original parsing
  ArgumentParser parser1(AppTable);
  
  int argc1;
  char **argv1 = CreateArgs({
    "program",
    "-I/usr/include",
    "-I", "/usr/local/include",
    "-DDEBUG",
    "-D", "RELEASE",
    "-v"
  }, argc1);
  
  assert(parser1.Parse(argc1, argv1));
  
  // Get original values
  auto origInclude = parser1.GetAllArgValues(OPT_include);
  auto origDefine = parser1.GetAllArgValues(OPT_define);
  auto origVerbose = parser1.HasArg(OPT_verbose);
  
  // Render to new arguments (space-separated format)
  std::vector<std::string> renderedArgs;
  renderedArgs.push_back("program");
  parser1.Render(OPT_include, renderedArgs);  // 2 values -> 4 args
  parser1.Render(OPT_define, renderedArgs);   // 2 values -> 4 args
  parser1.Render(OPT_verbose, renderedArgs);  // 1 flag -> 1 arg
  
  // Parse the rendered arguments
  ArgumentParser parser2(AppTable);
  
  int argc2;
  char **argv2 = CreateArgs(renderedArgs, argc2);
  
  assert(parser2.Parse(argc2, argv2));
  
  // Verify parsed values match original
  auto newInclude = parser2.GetAllArgValues(OPT_include);
  auto newDefine = parser2.GetAllArgValues(OPT_define);
  auto newVerbose = parser2.HasArg(OPT_verbose);
  
  assert(newInclude.size() == origInclude.size());
  assert(newInclude[0] == origInclude[0]);
  assert(newInclude[1] == origInclude[1]);
  
  assert(newDefine.size() == origDefine.size());
  assert(newDefine[0] == origDefine[0]);
  assert(newDefine[1] == origDefine[1]);
  
  assert(newVerbose == origVerbose);
  
  CleanupArgs(argv1, argc1);
  CleanupArgs(argv2, argc2);
  std::cout << "Parse-render-parse equivalence verified" << std::endl;
  
  #undef ARGS
}

int main() {
  std::cout << "Running Unit Tests for Render Function" << std::endl;
  std::cout << "========================================" << std::endl;
  
  RUN_TEST(TestRender_SeparateOptions);
  RUN_TEST(TestRender_JoinedOptions);
  RUN_TEST(TestRender_SeparateOrJoinedOptions);
  RUN_TEST(TestRender_ShortFeatureOptions);
  RUN_TEST(TestRender_MultipleValues);
  RUN_TEST(TestRender_FlagOptions);
  RUN_TEST(TestRender_OptionNotSpecified);
  RUN_TEST(TestRender_ComplexScenario);
  RUN_TEST(TestRender_ParseEquivalence);
  
  std::cout << "========================================" << std::endl;
  std::cout << "All Render tests PASSED!" << std::endl;
  
  return 0;
}
