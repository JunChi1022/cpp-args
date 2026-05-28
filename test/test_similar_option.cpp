#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>

using namespace cppargs;

#define TEST(name) void name()
#define RUN_TEST(name)                                                         \
  do {                                                                         \
    std::cout << "Running " << #name << "... ";                                \
    name();                                                                    \
    std::cout << "PASSED" << std::endl;                                        \
  } while (0)

// Test basic typo detection- long option
TEST(TestSimilarOption_BasicTypo) {
#define OPTIONS1(F)                                                            \
  F(verbose, v, "Enable verbose", FLAG, {})                                    \
  F(port, p, "Server port", SEPARATE, {})                                        \
  F(log_lvl, l, "Log level", SEPARATE, {"debug", "info"})

  DEFINE_ARGS(App, AppTable, OPTIONS1)

  ArgumentParser parser(AppTable);

  // Test with a typo: "verbos" instead of "verbose"
  std::string suggestion = parser.FindSimilarOption("--verbos");
  std::cout << "Suggestion for '--verbos': " << suggestion << std::endl;
  assert(suggestion == "--verbose");
}

// Test basic typo detection- short option
TEST(TestSimilarOption_ShortTypo) {
#define OPTIONS2(F)                                                            \
  F(verbose, v, "Enable verbose", FLAG, {})                                    \
  F(port, p, "Server port", SEPARATE, {})

  DEFINE_ARGS(App, AppTable, OPTIONS2)

  ArgumentParser parser(AppTable);

  // Test with a typo in short option
  std::string suggestion = parser.FindSimilarOption("-vv");
  assert(suggestion == "-v");
  std::cout << "Suggestion for '-vv': " << suggestion << std::endl;
}

// Test underscore/dash normalization
TEST(TestSimilarOption_UnderscoreDash) {
#define OPTIONS3(F)                                                            \
  F(log_lvl, l, "Log level", SEPARATE, {"debug", "info"})                        \
  F(output_dir, o, "Output directory", SEPARATE, {})

  DEFINE_ARGS(App, AppTable, OPTIONS3)

  ArgumentParser parser(AppTable);

  // Test that log-lvl matches log_lvl
  std::string suggestion = parser.FindSimilarOption("--log-lvel");
  assert(suggestion == "--log-lvl");
  std::cout << "Suggestion for '--log-lvel': " << suggestion << std::endl;
}

// Test no close match
TEST(TestSimilarOption_NoMatch) {
#define OPTIONS4(F)                                                            \
  F(verbose, v, "Enable verbose", FLAG, {})                                    \
  F(port, p, "Server port", SEPARATE, {})

  DEFINE_ARGS(App, AppTable, OPTIONS4)

  ArgumentParser parser(AppTable);

  // Test with something completely different
  std::string suggestion = parser.FindSimilarOption("--xyz123");
  assert(suggestion.empty());
  std::cout << "Suggestion for '--xyz123': (none)" << std::endl;
}

// Test error message with suggestion
TEST(TestParseErrorWithSuggestion) {
#define OPTIONS5(F)                                                            \
  F(verbose, v, "Enable verbose", FLAG, {})                                    \
  F(port, p, "Server port", SEPARATE, {})

  DEFINE_ARGS(App, AppTable, OPTIONS5)

  ArgumentParser parser(AppTable);
  parser.SetAllowUnknown(false);

  // Capture stderr
  std::streambuf *sbuf = std::cerr.rdbuf();
  std::string captured;

  int argc;
  const char **argv = CreateArgs({"program", "--verbos"}, argc);

  bool result = parser.Parse(argc, argv);
  assert(!result); // Should fail

  CleanupArgs(argv, argc);
  std::cerr.rdbuf(sbuf);

  std::cout << "Error message includes suggestion" << std::endl;
}

// Test joined option typo
TEST(TestSimilarOption_JoinedTypo) {
#define OPTIONS6(F)                                                            \
  F(library, L, "Library to use", JOINED, {"cuda", "opencl"})                  \
  F(output, o, "Output file", SEPARATE, {})

  DEFINE_ARGS(App, AppTable, OPTIONS6)

  ArgumentParser parser(AppTable);

  // Test with a typo in joined option
  std::string suggestion = parser.FindSimilarOption("--libary=cuda");
  assert(suggestion == "--library");
  std::cout << "Suggestion for '--libary=cuda': " << suggestion << std::endl;
}

// Test that EQ_JOIN suggestion does not append = when input has no =
TEST(TestSimilarOption_EqJoinNoEquals) {
#define OPTIONS7(F)                                                            \
  F(config, c, "Config file", SEPARATE | EQ_JOIN, {"dev", "prod"})

  DEFINE_ARGS(App, AppTable, OPTIONS7)

  ArgumentParser parser(AppTable);

  // Misspelling without = should not get = appended
  std::string suggestion = parser.FindSimilarOption("--confi");
  assert(suggestion == "--config");
  std::cout << "Suggestion for '--confi' (no =): " << suggestion << std::endl;
}

// Test that EQ_JOIN suggestion DOES append = when input has =
TEST(TestSimilarOption_EqJoinWithEquals) {
#define OPTIONS8(F)                                                            \
  F(config, c, "Config file", SEPARATE | EQ_JOIN, {"dev", "prod"})

  DEFINE_ARGS(App, AppTable, OPTIONS8)

  ArgumentParser parser(AppTable);

  // Misspelling with = should get = appended
  std::string suggestion = parser.FindSimilarOption("--confi=dev");
  assert(suggestion == "--config=");
  std::cout << "Suggestion for '--confi=dev' (with =): " << suggestion << std::endl;
}

// Test that FEAT_SHORT options suggest both -name and --name forms
TEST(TestSimilarOption_ShortBothDashes) {
#define OPTIONS9(F)                                                            \
  F(help, h, "Print help", FLAG | SHORT, {})

  DEFINE_ARGS(App, AppTable, OPTIONS9)

  ArgumentParser parser(AppTable);

  // Typo with single dash should suggest -help
  std::string s1 = parser.FindSimilarOption("-hel");
  assert(s1 == "-help");
  std::cout << "Suggestion for '-hel': " << s1 << std::endl;

  // Typo with double dash should suggest --help
  std::string s2 = parser.FindSimilarOption("--hel");
  assert(s2 == "--help");
  std::cout << "Suggestion for '--hel': " << s2 << std::endl;
}

int main() {
  try {
    RUN_TEST(TestSimilarOption_BasicTypo);
    RUN_TEST(TestSimilarOption_ShortTypo);
    RUN_TEST(TestSimilarOption_UnderscoreDash);
    RUN_TEST(TestSimilarOption_NoMatch);
    RUN_TEST(TestParseErrorWithSuggestion);
    RUN_TEST(TestSimilarOption_JoinedTypo);
    RUN_TEST(TestSimilarOption_EqJoinNoEquals);
    RUN_TEST(TestSimilarOption_EqJoinWithEquals);
    RUN_TEST(TestSimilarOption_ShortBothDashes);

    std::cout << "\nAll tests PASSED!" << std::endl;
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
