#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

using namespace cppargs;

// Test basic alias functionality
void TestBasicAlias() {
  #define TEST_ALIAS_OPTIONS(A) \
    A(save_temps, st, keep) \
    A(verbose, v, print_log)
  
  DEFINE_ALIAS(TestAliasTable, TEST_ALIAS_OPTIONS)
  
  #define MAIN_ARGS(F) \
    F(keep, k, "Keep temporary files", FLAG, {}) \
    F(print_log, p, "Print log output", FLAG, {})
  
  DEFINE_ARGS(MainArgs, MainTable, MAIN_ARGS)
  
  ArgumentParser parser(MainTable);
  parser.SetAliasTable(TestAliasTable);
  
  const char *argv[] = {"program", "--save_temps", "-v"};
  int argc = 3;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  // Both aliases should map to the same options
  assert(parser.HasArg((int)OPT_keep));
  assert(parser.HasArg((int)OPT_print_log));
  
  std::cout << "TestBasicAlias PASSED" << std::endl;
}

// Test alias with value options
void TestAliasWithValue() {
  #define ALIAS_WITH_VALUE(A) \
    A(output_file, out, output)
  
  DEFINE_ALIAS(ValueAliasTable, ALIAS_WITH_VALUE)
  
  #define MAIN_ARGS2(F)                                                        \
    F(output, o, "Output file", SEPARATE | EQ_JOIN, {})

  DEFINE_ARGS(MainArgs2, MainTable2, MAIN_ARGS2)
  
  ArgumentParser parser(MainTable2);
  parser.SetAliasTable(ValueAliasTable);
  
  const char *argv[] = {"program", "--output_file=value.txt"};
  int argc = 2;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  assert(parser.HasArg((int)OPT_output));
  assert(parser.GetArgValue((int)OPT_output) == "value.txt");
  
  std::cout << "TestAliasWithValue PASSED" << std::endl;
}

// Test mixed usage of alias and original name
void TestMixedAliasUsage() {
  #define MIXED_ALIAS(A) \
    A(verb, vb, verbose)
  
  DEFINE_ALIAS(MixedAliasTable, MIXED_ALIAS)
  
  #define MAIN_ARGS3(F) \
    F(verbose, v, "Verbose mode", FLAG, {})
  
  DEFINE_ARGS(MainArgs3, MainTable3, MAIN_ARGS3)
  
  ArgumentParser parser(MainTable3);
  parser.SetAliasTable(MixedAliasTable);
  
  // Use both alias and original name
  const char *argv[] = {"program", "--verb", "-vb"};
  int argc = 3;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  // Should be recognized as the same option (appears twice)
  assert(parser.HasArg((int)OPT_verbose));
  assert(parser.GetAllArgValues((int)OPT_verbose).size() == 2);
  
  std::cout << "TestMixedAliasUsage PASSED" << std::endl;
}

// Test short name with alias
void TestAliasWithShortName() {
  #define SHORT_ALIAS(A) \
    A(ver, vr, verbose)
  
  DEFINE_ALIAS(ShortAliasTable, SHORT_ALIAS)
  
  #define MAIN_ARGS4(F) \
    F(verbose, v, "Verbose mode", FLAG, {})
  
  DEFINE_ARGS(MainArgs4, MainTable4, MAIN_ARGS4)
  
  ArgumentParser parser(MainTable4);
  parser.SetAliasTable(ShortAliasTable);
  
  // Use short alias
  const char *argv[] = {"program", "-vr"};
  int argc = 2;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  assert(parser.HasArg((int)OPT_verbose));
  
  std::cout << "TestAliasWithShortName PASSED" << std::endl;
}

// Test alias with joined option type
void TestAliasWithJoinedOption() {
  #define JOINED_ALIAS(A) \
    A(lib_path, L, library)
  
  DEFINE_ALIAS(JoinedAliasTable, JOINED_ALIAS)
  
  #define MAIN_ARGS5(F) \
    F(library, L, "Library path", JOINED, {})
  
  DEFINE_ARGS(MainArgs5, MainTable5, MAIN_ARGS5)
  
  ArgumentParser parser(MainTable5);
  parser.SetAliasTable(JoinedAliasTable);
  
  // Test long alias with joined value
  const char *argv[] = {"program", "--lib_pathcuda"};
  int argc = 2;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  assert(parser.HasArg((int)OPT_library));
  assert(parser.GetArgValue((int)OPT_library) == "cuda");
  
  std::cout << "TestAliasWithJoinedOption PASSED" << std::endl;
}

// Test short alias with joined option type
void TestShortAliasWithJoinedOption() {
  #define JOINED_SHORT_ALIAS(A) \
    A(libpath, L, library)
  
  DEFINE_ALIAS(JoinedShortAliasTable, JOINED_SHORT_ALIAS)
  
  #define MAIN_ARGS6(F) \
    F(library, l, "Library path", JOINED, {})
  
  DEFINE_ARGS(MainArgs6, MainTable6, MAIN_ARGS6)
  
  ArgumentParser parser(MainTable6);
  parser.SetAliasTable(JoinedShortAliasTable);
  
  // Test short alias with joined value
  const char *argv[] = {"program", "-Lcuda"};
  int argc = 2;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  assert(parser.HasArg((int)OPT_library));
  assert(parser.GetArgValue((int)OPT_library) == "cuda");
  
  std::cout << "TestShortAliasWithJoinedOption PASSED" << std::endl;
}

// Test alias without short name
void TestAliasWithoutShortName() {
  #define NO_SHORT_ALIAS(A) \
    A(helpme, , help)
  
  DEFINE_ALIAS(NoShortAliasTable, NO_SHORT_ALIAS)
  
  #define MAIN_ARGS7(F) \
    F(help, h, "Print help", FLAG, {})
  
  DEFINE_ARGS(MainArgs7, MainTable7, MAIN_ARGS7)
  
  ArgumentParser parser(MainTable7);
  parser.SetAliasTable(NoShortAliasTable);
  
  // Use long alias only (no short alias)
  const char *argv[] = {"program", "--helpme"};
  int argc = 2;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  assert(parser.HasArg((int)OPT_help));
  
  std::cout << "TestAliasWithoutShortName PASSED" << std::endl;
}

// Test multiple aliases for the same option
void TestMultipleAliasesForSameOption() {
  #define MULTI_ALIASES(A) \
    A(helpme, hm, help) \
    A(hlp, , help) \
    A(assistance, as, help)
  
  DEFINE_ALIAS(MultiAliasTable, MULTI_ALIASES)
  
  #define MAIN_ARGS8(F) \
    F(help, h, "Print help", FLAG, {})
  
  DEFINE_ARGS(MainArgs8, MainTable8, MAIN_ARGS8)
  
  ArgumentParser parser(MainTable8);
  parser.SetAliasTable(MultiAliasTable);
  
  // Use different aliases for the same option
  const char *argv[] = {"program", "--helpme", "-hm", "--hlp", "--assistance"};
  int argc = 5;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  // All should map to the same OPT_help option
  assert(parser.HasArg((int)OPT_help));
  // Should have 4 occurrences (all map to the same option)
  assert(parser.GetAllArgValues((int)OPT_help).size() == 4);
  
  std::cout << "TestMultipleAliasesForSameOption PASSED" << std::endl;
}

// Test multiple aliases with mixed usage
void TestMultipleAliasesMixedUsage() {
  #define MIXED_MULTI_ALIASES(A) \
    A(verb, vb, verbose) \
    A(debug, db, verbose) \
    A(log_all, la, verbose)
  
  DEFINE_ALIAS(MixedMultiAliasTable, MIXED_MULTI_ALIASES)
  
  #define MAIN_ARGS9(F) \
    F(verbose, v, "Verbose mode", FLAG, {})
  
  DEFINE_ARGS(MainArgs9, MainTable9, MAIN_ARGS9)
  
  ArgumentParser parser(MainTable9);
  parser.SetAliasTable(MixedMultiAliasTable);
  
  // Mix original name and aliases
  const char *argv[] = {"program", "--verb", "--debug", "--log_all"};
  int argc = 4;
  
  bool result = parser.Parse(argc, const_cast<char **>(argv));
  assert(result);
  
  assert(parser.HasArg((int)OPT_verbose));
  // All 3 aliases map to the same option
  assert(parser.GetAllArgValues((int)OPT_verbose).size() == 3);
  
  std::cout << "TestMultipleAliasesMixedUsage PASSED" << std::endl;
}

int main() {
  TestBasicAlias();
  TestAliasWithValue();
  TestMixedAliasUsage();
  TestAliasWithShortName();
  TestAliasWithJoinedOption();
  TestShortAliasWithJoinedOption();
  TestAliasWithoutShortName();
  TestMultipleAliasesForSameOption();
  TestMultipleAliasesMixedUsage();
  
  std::cout << "\n=== All Alias Tests PASSED ===" << std::endl;
  return 0;
}
