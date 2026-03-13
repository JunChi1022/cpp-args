#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

// Test options with different kinds
#define TEST_OPTIONS(F)                                                        \
  F(library, L, "Link library", JOINED, {"cuda", "stdc++", "pthread"})         \
  F(include, I, "Include directory", SEPARATE_OR_JOINED, {})                   \
  F(output, o, "Output file", OPTION, {})                                      \
  F(port, p, "Port number", OPTION, {})                                        \
  F(verbose, v, "Verbose mode", FLAG, {})

DEFINE_ARGS(OptionTest, ParsingTestTable, TEST_OPTIONS)

// ============================================================================
// JOINED Kind Tests
// ============================================================================

void TestJoined_ShortOption() {
  std::cout << "Test: JOINED - Short option without equals (-Lcuda)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-Lcuda"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_library));
  assert(parser.GetArgValue(OPT_library) == "cuda");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestJoined_LongWithEquals_ShouldFail() {
  std::cout << "Test: JOINED - Long option with equals should fail (--library=cuda)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--library=cuda"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  // JOINED 类型不支持长格式带等号，应该解析失败
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestJoined_MultipleValues() {
  std::cout << "Test: JOINED - Multiple values... ";

  int argc;
  const char *prog = "./test";
  // JOINED 类型只支持短格式直接连接，不支持长格式带等号
  std::vector<std::string> args = {prog, "-Lcuda", "-Lpthread", "-Lstdc++"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_library));
  
  auto values = parser.GetAllArgValues(OPT_library);
  assert(values.size() == 3);
  assert(values[0] == "cuda");
  assert(values[1] == "pthread");
  assert(values[2] == "stdc++");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestJoined_LongOptionWithValue() {
  std::cout << "Test: JOINED - Long option with attached value (--librarycuda)... ";

  int argc;
  const char *prog = "./test";
  // JOINED 类型支持长选项直接连接值的形式
  std::vector<std::string> args = {prog, "--librarycuda", "--librarystdc++"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_library));
  
  auto values = parser.GetAllArgValues(OPT_library);
  assert(values.size() == 2);
  assert(values[0] == "cuda");
  assert(values[1] == "stdc++");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestJoined_ShortAndLongMixed() {
  std::cout << "Test: JOINED - Mixed short and long options... ";

  int argc;
  const char *prog = "./test";
  // 混合使用短选项和长选项直接连接形式
  std::vector<std::string> args = {prog, "-Lcuda", "--librarystdc++", "-Lpthread"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_library));
  
  auto values = parser.GetAllArgValues(OPT_library);
  assert(values.size() == 3);
  assert(values[0] == "cuda");
  assert(values[1] == "stdc++");
  assert(values[2] == "pthread");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// ============================================================================
// SEPARATE_OR_JOINED Kind Tests
// ============================================================================

void TestSeparateOrJoined_SpaceSeparated() {
  std::cout << "Test: SEPARATE_OR_JOINED - Space separated (-I /usr/include)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-I", "/usr/include"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_include));
  assert(parser.GetArgValue(OPT_include) == "/usr/include");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestSeparateOrJoined_ShortWithEquals() {
  std::cout << "Test: SEPARATE_OR_JOINED - Short with equals (-I=/usr/include)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-I=/usr/include"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_include));
  assert(parser.GetArgValue(OPT_include) == "/usr/include");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestSeparateOrJoined_ShortJoined() {
  std::cout << "Test: SEPARATE_OR_JOINED - Short joined (-I/usr/include)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-I/usr/include"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_include));
  assert(parser.GetArgValue(OPT_include) == "/usr/include");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestSeparateOrJoined_LongWithEquals() {
  std::cout << "Test: SEPARATE_OR_JOINED - Long with equals (--include=/usr/include)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::vector<std::string>> testCases = {
    {prog, "--include=/usr/include"},
    {prog, "--include", "/usr/include"}
  };

  for (const auto& args : testCases) {
    char **argv = CreateArgs(args, argc);
    ArgumentParser parser(ParsingTestTable);
    assert(parser.Parse(argc, argv));
    assert(parser.HasArg(OPT_include));
    assert(parser.GetArgValue(OPT_include) == "/usr/include");
    CleanupArgs(argv, argc);
  }

  std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Regular OPTION Kind Tests
// ============================================================================

void TestRegularOption_SpaceSeparated() {
  std::cout << "Test: OPTION - Space separated (-o output.txt)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-o", "output.txt"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "output.txt");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestRegularOption_ShortWithEquals() {
  std::cout << "Test: OPTION - Short with equals (-o=output.txt)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-o=output.txt"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "output.txt");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestRegularOption_LongWithEquals() {
  std::cout << "Test: OPTION - Long with equals (--output=output.txt)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--output=output.txt"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "output.txt");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestRegularOption_LongSpaceSeparated() {
  std::cout << "Test: OPTION - Long space separated (--output output.txt)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--output", "output.txt"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "output.txt");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestRegularOption_WithNumericValue() {
  std::cout << "Test: OPTION - Numeric value with equals (-p=8080)... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-p=8080"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_port));
  assert(parser.GetArgValue(OPT_port) == "8080");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Mixed Options Tests
// ============================================================================

void TestMixed_AllKinds() {
  std::cout << "Test: MIXED - All option kinds together... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {
    prog,
    "-Lcuda",                    // JOINED short
    "-Lpthread",                 // JOINED short (JOINED 不支持长格式带等号)
    "-I", "/usr/include",        // SEPARATE_OR_JOINED space
    "-I=/usr/local/include",     // SEPARATE_OR_JOINED short with =
    "-I/usr/share/include",      // SEPARATE_OR_JOINED short joined
    "--include=/opt/include",    // SEPARATE_OR_JOINED long with =
    "-o", "app.out",             // OPTION space
    "--port=8080",               // OPTION long with =
    "-v"                         // FLAG
  };
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(parser.Parse(argc, argv));
  
  // Check library (JOINED)
  assert(parser.HasArg(OPT_library));
  auto libs = parser.GetAllArgValues(OPT_library);
  assert(libs.size() == 2);
  assert(libs[0] == "cuda");
  assert(libs[1] == "pthread");
  
  // Check include (SEPARATE_OR_JOINED)
  assert(parser.HasArg(OPT_include));
  auto includes = parser.GetAllArgValues(OPT_include);
  assert(includes.size() == 4);
  assert(includes[0] == "/usr/include");
  assert(includes[1] == "/usr/local/include");
  assert(includes[2] == "/usr/share/include");
  assert(includes[3] == "/opt/include");
  
  // Check output (OPTION)
  assert(parser.HasArg(OPT_output));
  assert(parser.GetArgValue(OPT_output) == "app.out");
  
  // Check port (OPTION)
  assert(parser.HasArg(OPT_port));
  assert(parser.GetArgValue(OPT_port) == "8080");
  
  // Check verbose (FLAG)
  assert(parser.HasArg(OPT_verbose));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Error Cases Tests
// ============================================================================

void TestJoined_InvalidValue() {
  std::cout << "Test: ERROR - JOINED with invalid value... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-Linvalid"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

void TestRegularOption_MissingValue() {
  std::cout << "Test: ERROR - OPTION missing value... ";

  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-o"};
  char **argv = CreateArgs(args, argc);

  ArgumentParser parser(ParsingTestTable);
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Comprehensive Option Parsing Tests" << std::endl;
  std::cout << "==========================================" << std::endl;

  // JOINED tests
  TestJoined_ShortOption();
  TestJoined_LongWithEquals_ShouldFail();
  TestJoined_MultipleValues();
  TestJoined_LongOptionWithValue();
  TestJoined_ShortAndLongMixed();

  // SEPARATE_OR_JOINED tests
  TestSeparateOrJoined_SpaceSeparated();
  TestSeparateOrJoined_ShortWithEquals();
  TestSeparateOrJoined_ShortJoined();
  TestSeparateOrJoined_LongWithEquals();

  // Regular OPTION tests
  TestRegularOption_SpaceSeparated();
  TestRegularOption_ShortWithEquals();
  TestRegularOption_LongWithEquals();
  TestRegularOption_LongSpaceSeparated();
  TestRegularOption_WithNumericValue();

  // Mixed tests
  TestMixed_AllKinds();

  // Error cases
  TestJoined_InvalidValue();
  TestRegularOption_MissingValue();

  std::cout << "==========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
