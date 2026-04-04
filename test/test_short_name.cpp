#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

using namespace cppargs;

#define TEST_OPTIONS(V)                                                        \
  V(port, p, "Server port number", SEPARATE, {})                               \
  V(log_lvl, l, "Logging verbosity", SEPARATE, {"debug", "info"})              \
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
#define SHORT_OPTS(F)                                                          \
  F(help, h, "Print help", FLAG | SHORT, {})                                   \
  F(verbose, v, "Verbose mode", FLAG | SHORT, {})                              \
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
#define SHORT_OPTS2(F)                                                         \
  F(output, o, "Output file", SEPARATE | EQ_JOIN | SHORT, {})                  \
  F(level, l, "Log level", SEPARATE | EQ_JOIN | SHORT,                         \
    {"debug", "info", "error"})

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
#define MIXED_DASH_OPTS(F)                                                     \
  F(help, h, "Print help", FLAG | SHORT, {})                                   \
  F(version, V, "Print version", FLAG, {}) // Normal option (double dash only)

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

// Test that short name options require single dash format
void TestShortNameRequiresSingleDash() {
#define SHORT_NAME_TEST_OPTS(F)                                                \
  F(port, p, "Server port", SEPARATE, {})                                      \
  F(verbose, v, "Verbose mode", FLAG, {})

  DEFINE_ARGS(ShortNameTestOpts, ShortNameTestTable, SHORT_NAME_TEST_OPTS)

  ArgumentParser parser(ShortNameTestTable);

  // Test that -p works (single dash)
  const char *argv1[] = {"program", "-p", "8080"};
  int argc1 = 3;
  bool result1 = parser.Parse(argc1, const_cast<char **>(argv1));
  assert(result1);
  assert(parser.HasArg((int)OPT_port));
  assert(parser.GetArgValue((int)OPT_port) == "8080");

  // Reset parser state
  parser = ArgumentParser(ShortNameTestTable);

  // Test that --p does NOT work (double dash should not match short name)
  const char *argv2[] = {"program", "--p", "8080"};
  int argc2 = 3;
  bool result2 = parser.Parse(argc2, const_cast<char **>(argv2));
  // This should fail because --p is not a valid long option name
  assert(!result2 || !parser.HasArg((int)OPT_port));

  // Reset parser state
  parser = ArgumentParser(ShortNameTestTable);

  // Test that -v works (single dash for flag)
  const char *argv3[] = {"program", "-v"};
  int argc3 = 2;
  bool result3 = parser.Parse(argc3, const_cast<char **>(argv3));
  assert(result3);
  assert(parser.HasArg((int)OPT_verbose));

  // Reset parser state
  parser = ArgumentParser(ShortNameTestTable);

  // Test that --v does NOT work (double dash should not match short name for
  // flag)
  const char *argv4[] = {"program", "--v"};
  int argc4 = 2;
  bool result4 = parser.Parse(argc4, const_cast<char **>(argv4));
  // This should fail or not mark verbose as present
  assert(!result4 || !parser.HasArg((int)OPT_verbose));

  std::cout << "TestShortNameRequiresSingleDash PASSED" << std::endl;
}

// Test that short alias requires single dash format
void TestShortAliasRequiresSingleDash() {
#define ALIAS_TEST_OPTS(F) F(output, o, "Output file", SEPARATE, {})

  DEFINE_ARGS(AliasTestOpts, AliasTestTable, ALIAS_TEST_OPTS)

  // Define alias: 'out' -> 'output', 'O' -> 'output'
  AliasTable aliasTable = {
      {"output", "O", "output"} // Long alias "output" with short alias "O"
  };

  ArgumentParser parser(AliasTestTable);
  parser.SetAliasTable(aliasTable);

  // Test that -o works (original short name with single dash)
  const char *argv1[] = {"program", "-o", "file.txt"};
  int argc1 = 3;
  bool result1 = parser.Parse(argc1, const_cast<char **>(argv1));
  assert(result1);
  assert(parser.HasArg((int)OPT_output));
  assert(parser.GetArgValue((int)OPT_output) == "file.txt");

  // Reset parser state
  parser = ArgumentParser(AliasTestTable);
  parser.SetAliasTable(aliasTable);

  // Test that -O works (short alias with single dash)
  const char *argv2[] = {"program", "-O", "file2.txt"};
  int argc2 = 3;
  bool result2 = parser.Parse(argc2, const_cast<char **>(argv2));
  assert(result2);
  assert(parser.HasArg((int)OPT_output));
  assert(parser.GetArgValue((int)OPT_output) == "file2.txt");

  // Reset parser state
  parser = ArgumentParser(AliasTestTable);
  parser.SetAliasTable(aliasTable);

  // Test that --O does NOT work (short alias with double dash should fail)
  const char *argv3[] = {"program", "--O", "file3.txt"};
  int argc3 = 3;
  bool result3 = parser.Parse(argc3, const_cast<char **>(argv3));
  // This should fail or be treated as unknown option
  assert(!result3 || !parser.HasArg((int)OPT_output));

  std::cout << "TestShortAliasRequiresSingleDash PASSED" << std::endl;
}

// Test that long alias requires double dash format
void TestLongAliasRequiresDoubleDash() {
#define LONG_ALIAS_OPTS(F) F(keep, k, "Keep temporary files", SEPARATE, {})

  DEFINE_ARGS(LongAliasOpts, LongAliasTable, LONG_ALIAS_OPTS)

  // Define alias: 'save_temps' -> 'keep' (long alias)
  AliasTable aliasTable = {
      {"save_temps", "", "keep"} // Long alias "save_temps" with no short alias
  };

  ArgumentParser parser(LongAliasTable);
  parser.SetAliasTable(aliasTable);

  // Test that --keep works (original long name with double dash)
  const char *argv1[] = {"program", "--keep", "true"};
  int argc1 = 3;
  bool result1 = parser.Parse(argc1, const_cast<char **>(argv1));
  assert(result1);
  assert(parser.HasArg((int)OPT_keep));
  assert(parser.GetArgValue((int)OPT_keep) == "true");

  // Reset parser state
  parser = ArgumentParser(LongAliasTable);
  parser.SetAliasTable(aliasTable);

  // Test that --save_temps works (long alias with double dash)
  const char *argv2[] = {"program", "--save_temps", "true"};
  int argc2 = 3;
  bool result2 = parser.Parse(argc2, const_cast<char **>(argv2));
  assert(result2);
  assert(parser.HasArg((int)OPT_keep));
  assert(parser.GetArgValue((int)OPT_keep) == "true");

  // Reset parser state
  parser = ArgumentParser(LongAliasTable);
  parser.SetAliasTable(aliasTable);

  // Test that -save_temps does NOT work (long alias with single dash should
  // fail)
  const char *argv3[] = {"program", "-save_temps", "true"};
  int argc3 = 3;
  bool result3 = parser.Parse(argc3, const_cast<char **>(argv3));
  // This should fail or be treated as unknown option
  assert(!result3 || !parser.HasArg((int)OPT_keep));

  std::cout << "TestLongAliasRequiresDoubleDash PASSED" << std::endl;
}

// Test that short name normalization works correctly (underscore to dash)
void TestShortNameNormalization() {
#define SHORT_NORM_OPTS(F)                                                     \
  F(output_file, o_f, "Output file path", SEPARATE, {})                        \
  F(log_level, l_l, "Log level setting", SEPARATE, {"debug", "info", "error"})

  DEFINE_ARGS(ShortNormOpts, ShortNormTable, SHORT_NORM_OPTS)

  ArgumentParser parser(ShortNormTable);

  // Test that short name with underscore is normalized and matches
  // The macro defines short names as "o_f" and "l_l", which should be
  // normalized to "o-f" and "l-l"
  const char *argv1[] = {"program", "-o_f", "result.txt"};
  int argc1 = 3;
  bool result1 = parser.Parse(argc1, const_cast<char **>(argv1));
  assert(result1);
  assert(parser.HasArg((int)OPT_output_file));
  assert(parser.GetArgValue((int)OPT_output_file) == "result.txt");

  // Reset parser state
  parser = ArgumentParser(ShortNormTable);

  // Test with dash format in short name (should also work after normalization)
  const char *argv2[] = {"program", "-o-f", "output.dat"};
  int argc2 = 3;
  bool result2 = parser.Parse(argc2, const_cast<char **>(argv2));
  assert(result2);
  assert(parser.HasArg((int)OPT_output_file));
  assert(parser.GetArgValue((int)OPT_output_file) == "output.dat");

  // Reset parser state
  parser = ArgumentParser(ShortNormTable);

  // Test another short name with underscore
  const char *argv3[] = {"program", "-l_l", "debug"};
  int argc3 = 3;
  bool result3 = parser.Parse(argc3, const_cast<char **>(argv3));
  assert(result3);
  assert(parser.HasArg((int)OPT_log_level));
  assert(parser.GetArgValue((int)OPT_log_level) == "debug");

  // Reset parser state
  parser = ArgumentParser(ShortNormTable);

  // Test with dash format
  const char *argv4[] = {"program", "-l-l", "info"};
  int argc4 = 3;
  bool result4 = parser.Parse(argc4, const_cast<char **>(argv4));
  assert(result4);
  assert(parser.HasArg((int)OPT_log_level));
  assert(parser.GetArgValue((int)OPT_log_level) == "info");

  std::cout << "TestShortNameNormalization PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for Argument Parser" << std::endl;
  std::cout << "========================================" << std::endl;

  TestShortName();
  TestSingleDashLongOption();
  TestSingleDashLongOptionWithValue();
  TestMixedSingleAndDoubleDash();
  TestShortNameRequiresSingleDash();
  TestShortAliasRequiresSingleDash();
  TestLongAliasRequiresDoubleDash();
  TestShortNameNormalization();

  std::cout << "========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
