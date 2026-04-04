#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

using namespace cppargs;

// Test basic comma-separated list parsing with SEPARATE format
void TestCommaListBasic() {
  std::cout << "Test: Comma List Basic... ";

  #define COMMA_LIST_OPTS(F) \
    F(tags, t, "Tags as comma-separated list", SEPARATE | COMMA_LIST, {"debug", "info", "error", "warning"})
  
  DEFINE_ARGS(CommaListOpts, CommaListTable, COMMA_LIST_OPTS)
  
  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--tags", "debug,info,error"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(CommaListTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_tags));
  
  // GetAllArgValues should return all split values
  const auto &allValues = parser.GetAllArgValues(OPT_tags);
  assert(allValues.size() == 3);
  assert(allValues[0] == "debug");
  assert(allValues[1] == "info");
  assert(allValues[2] == "error");
  
  // GetArgValue returns the last value (backward compatibility)
  assert(parser.GetArgValue(OPT_tags) == "error");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// Test comma-separated list with EQ_JOIN format
void TestCommaListEqJoin() {
  std::cout << "Test: Comma List Eq Join... ";

  #define COMMA_EQ_OPTS(F) \
    F(levels, l, "Log levels", SEPARATE | EQ_JOIN | COMMA_LIST, {"debug", "info", "warn", "error"})
  
  DEFINE_ARGS(CommaEqOpts, CommaEqTable, COMMA_EQ_OPTS)
  
  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--levels=debug,warn,error"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(CommaEqTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_levels));
  
  const auto &allValues = parser.GetAllArgValues(OPT_levels);
  assert(allValues.size() == 3);
  assert(allValues[0] == "debug");
  assert(allValues[1] == "warn");
  assert(allValues[2] == "error");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// Test comma-separated list with whitespace handling
void TestCommaListWhitespace() {
  std::cout << "Test: Comma List Whitespace... ";

  #define COMMA_WS_OPTS(F) \
    F(items, i, "Items with spaces", SEPARATE | COMMA_LIST, {"apple", "banana", "cherry"})
  
  DEFINE_ARGS(CommaWsOpts, CommaWsTable, COMMA_WS_OPTS)
  
  int argc;
  const char *prog = "./test";
  // Values with spaces around commas should be trimmed
  std::vector<std::string> args = {prog, "--items", "apple , banana , cherry"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(CommaWsTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_items));
  
  const auto &allValues = parser.GetAllArgValues(OPT_items);
  assert(allValues.size() == 3);
  assert(allValues[0] == "apple");
  assert(allValues[1] == "banana");
  assert(allValues[2] == "cherry");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// Test comma-separated list validation - invalid value
void TestCommaListInvalidValue() {
  std::cout << "Test: Comma List Invalid Value... ";

  #define COMMA_INV_OPTS(F) \
    F(modes, m, "Modes", SEPARATE | COMMA_LIST, {"fast", "slow", "normal"})
  
  DEFINE_ARGS(CommaInvOpts, CommaInvTable, COMMA_INV_OPTS)
  
  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--modes", "fast,invalid,slow"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(CommaInvTable);
  // Should fail because "invalid" is not in allowed values
  assert(!parser.Parse(argc, argv));

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// Test comma-separated list without allowed values (any value accepted)
void TestCommaListNoValidation() {
  std::cout << "Test: Comma List No Validation... ";

  #define COMMA_NOVAL_OPTS(F) \
    F(values, v, "Any values", SEPARATE | COMMA_LIST, {})
  
  DEFINE_ARGS(CommaNoValOpts, CommaNoValTable, COMMA_NOVAL_OPTS)
  
  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--values", "abc,def,ghi,jkl"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(CommaNoValTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_values));
  
  const auto &allValues = parser.GetAllArgValues(OPT_values);
  assert(allValues.size() == 4);
  assert(allValues[0] == "abc");
  assert(allValues[1] == "def");
  assert(allValues[2] == "ghi");
  assert(allValues[3] == "jkl");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// Test single value in comma list (no comma)
void TestCommaListSingleValue() {
  std::cout << "Test: Comma List Single Value... ";

  #define COMMA_SINGLE_OPTS(F) \
    F(single, s, "Single or multiple", SEPARATE | COMMA_LIST, {"one", "two", "three"})
  
  DEFINE_ARGS(CommaSingleOpts, CommaSingleTable, COMMA_SINGLE_OPTS)
  
  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "--single", "two"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(CommaSingleTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_single));
  
  const auto &allValues = parser.GetAllArgValues(OPT_single);
  assert(allValues.size() == 1);
  assert(allValues[0] == "two");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// Test multiple occurrences of comma list option
void TestCommaListMultipleOccurrences() {
  std::cout << "Test: Comma List Multiple Occurrences... ";

  #define COMMA_MULTI_OPTS(F) \
    F(colors, c, "Colors", SEPARATE | COMMA_LIST, {"red", "green", "blue"})
  
  DEFINE_ARGS(CommaMultiOpts, CommaMultiTable, COMMA_MULTI_OPTS)
  
  int argc;
  const char *prog = "./test";
  // Specify the option twice with different comma-separated values
  std::vector<std::string> args = {prog, "--colors", "red,green", "--colors", "blue"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(CommaMultiTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_colors));
  
  const auto &allValues = parser.GetAllArgValues(OPT_colors);
  // Should have all values from both occurrences
  assert(allValues.size() == 3);
  assert(allValues[0] == "red");
  assert(allValues[1] == "green");
  assert(allValues[2] == "blue");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

// Test COMMA_LIST with SHORT feature
void TestCommaListWithShort() {
  std::cout << "Test: Comma List With Short... ";

  #define COMMA_SHORT_OPTS(F) \
    F(flags, f, "Flags", SEPARATE | EQ_JOIN | COMMA_LIST | SHORT, {"a", "b", "c"})
  
  DEFINE_ARGS(CommaShortOpts, CommaShortTable, COMMA_SHORT_OPTS)
  
  // Test with single dash long format and equals join
  int argc;
  const char *prog = "./test";
  std::vector<std::string> args = {prog, "-flags=a,b,c"};
  const char **argv = CreateArgs(args, argc);

  ArgumentParser parser(CommaShortTable);
  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_flags));
  
  const auto &allValues = parser.GetAllArgValues(OPT_flags);
  assert(allValues.size() == 3);
  assert(allValues[0] == "a");
  assert(allValues[1] == "b");
  assert(allValues[2] == "c");

  CleanupArgs(argv, argc);
  std::cout << "PASSED" << std::endl;
}

int main() {
  std::cout << "Running Unit Tests for COMMA_LIST Feature" << std::endl;
  std::cout << "==========================================" << std::endl;

  TestCommaListBasic();
  TestCommaListEqJoin();
  TestCommaListWhitespace();
  TestCommaListInvalidValue();
  TestCommaListNoValidation();
  TestCommaListSingleValue();
  TestCommaListMultipleOccurrences();
  TestCommaListWithShort();

  std::cout << "==========================================" << std::endl;
  std::cout << "All tests PASSED!" << std::endl;

  return 0;
}
