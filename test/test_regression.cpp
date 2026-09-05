#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>
#include <sstream>

using namespace cppargs;

// Test the "--" delimiter: everything after it is a positional input
TEST(TestEndOfOptionsDelimiter) {
  #define ARGS(F)                                                            \
    F(port, p, "Server port", SEPARATE, {})                          \
    F(verbose, v, "Verbose mode", FLAG, {})
  DEFINE_ARGS(RegArgs, RegTable, ARGS)

  ArgumentParser parser(RegTable);

  int argc;
  const char **argv = CreateArgs(
      {"program", "--verbose", "--", "--port", "8080", "-v", "file.cpp"}, argc);

  assert(parser.Parse(argc, argv));
  assert(parser.HasArg(OPT_verbose));
  // "--port", "8080", "-v" and "file.cpp" are all inputs after "--"
  const auto &inputs = parser.GetInputs();
  assert(inputs.size() == 4);
  assert(inputs[0] == "--port");
  assert(inputs[1] == "8080");
  assert(inputs[2] == "-v");
  assert(inputs[3] == "file.cpp");
  assert(!parser.HasArg(OPT_port));

  CleanupArgs(argv, argc);
  #undef ARGS
}

// Test that a SEPARATE option does not swallow the next argument when it looks
// like an option (e.g. "--port --verbose")
TEST(TestSeparateDoesNotSwallowOption) {
  #define ARGS(F)                                                            \
    F(port, p, "Server port", SEPARATE, {})                          \
    F(verbose, v, "Verbose mode", FLAG, {})
  DEFINE_ARGS(RegArgs, RegTable, ARGS)

  ArgumentParser parser(RegTable);
  const char *argv[] = {"program", "--port", "--verbose"};
  assert(!parser.Parse(3, argv));
  assert(!parser.HasArg(OPT_port));
  assert(!parser.HasArg(OPT_verbose));
  #undef ARGS
}

// Test that "-" (stdin convention) and negative numbers remain valid values
TEST(TestDashAndNegativeValues) {
  #define ARGS(F)                                                            \
    F(port, p, "Server port", SEPARATE, {})
  DEFINE_ARGS(RegArgs, RegTable, ARGS)

  {
    ArgumentParser parser(RegTable);
    const char *argv[] = {"program", "--port", "-"};
    assert(parser.Parse(3, argv));
    assert(parser.GetArgValue(OPT_port) == "-");
  }
  {
    ArgumentParser parser(RegTable);
    const char *argv[] = {"program", "--port", "-1"};
    assert(parser.Parse(3, argv));
    assert(parser.GetArgValue(OPT_port) == "-1");
  }
  #undef ARGS
}

// Test JOINED longest-prefix matching: --log_lvlhigh must match log_lvl,
// not log, regardless of table order
TEST(TestJoinedLongestPrefixMatch) {
  #define ARGS(F)                                                            \
    F(log, l, "Log prefix", JOINED, {})                              \
    F(log_lvl, L, "Log level", JOINED, {})
  DEFINE_ARGS(RegArgs, RegTable, ARGS)

  {
    ArgumentParser parser(RegTable);
    int argc;
    const char **argv = CreateArgs({"program", "--log_lvlhigh"}, argc);
    assert(parser.Parse(argc, argv));
    assert(!parser.HasArg(OPT_log));
    assert(parser.GetArgValue(OPT_log_lvl) == "high");
    CleanupArgs(argv, argc);
  }
  {
    // Exact name of a JOINED option without a value is a missing value error
    ArgumentParser parser(RegTable);
    int argc;
    const char **argv = CreateArgs({"program", "--log_lvl"}, argc);
    assert(!parser.Parse(argc, argv));
    CleanupArgs(argv, argc);
  }
  #undef ARGS
}

// Test that COMMA_LIST rejects empty fields (e.g. "a," or ",b")
TEST(TestCommaListEmptyFieldRejected) {
  #define ARGS(F)                                                            \
    F(incl, I, "Includes", SEPARATE | COMMA_LIST,            \
      {"a", "b", "c"})
  DEFINE_ARGS(RegArgs, RegTable, ARGS)

  {
    ArgumentParser parser(RegTable);
    const char *argv[] = {"program", "--incl", "a,"};
    assert(!parser.Parse(3, argv));
  }
  {
    ArgumentParser parser(RegTable);
    const char *argv[] = {"program", "--incl", ",a"};
    assert(!parser.Parse(3, argv));
  }
  {
    // No allowed values specified: empty fields must still be rejected
    #define NOVAL(F)                                                         \
      F(values, v, "Any values", SEPARATE | COMMA_LIST, {})
    DEFINE_ARGS(NovalArgs, NovalTable, NOVAL)
    ArgumentParser parser(NovalTable);
    const char *argv[] = {"program", "--values", "x,"};
    assert(!parser.Parse(3, argv));
    #undef NOVAL
  }
  #undef ARGS
}

// Test that invalid values report the allowed values
TEST(TestInvalidValueListsAllowed) {
  #define ARGS(F)                                                            \
    F(log_lvl, l, "Log level", SEPARATE, {"debug", "info"})
  DEFINE_ARGS(RegArgs, RegTable, ARGS)

  std::ostringstream err;
  ArgumentParser parser(RegTable);
  parser.SetErrMsgStream(&err);
  const char *argv[] = {"program", "--log-lvl", "bogus"};
  assert(!parser.Parse(3, argv));
  std::string msg = err.str();
  assert(msg.find("Allowed values: debug|info") != std::string::npos);
  #undef ARGS
}

// Test PrintHelp can be redirected to a custom stream
TEST(TestPrintHelpRedirect) {
  #define ARGS(F)                                                            \
    F(port, p, "Server port", SEPARATE, {})                          \
    F(verbose, v, "Verbose mode", FLAG, {})
  DEFINE_ARGS(RegArgs, RegTable, ARGS)

  ArgumentParser parser(RegTable);
  std::ostringstream help;
  parser.PrintHelp(help);
  std::string out = help.str();
  assert(!out.empty());
  assert(out.find("--port") != std::string::npos);
  assert(out.find("--verbose") != std::string::npos);
  #undef ARGS
}

// Test Parse() appends and Reset() clears (documented behavior)
TEST(TestParseAppendsAndResetClears) {
  #define ARGS(F)                                                            \
    F(port, p, "Server port", SEPARATE, {})
  DEFINE_ARGS(RegArgs, RegTable, ARGS)

  ArgumentParser parser(RegTable);
  const char *argv1[] = {"program", "--port", "1"};
  assert(parser.Parse(3, argv1));
  // Second Parse without Reset appends
  const char *argv2[] = {"program", "--port", "2"};
  assert(parser.Parse(3, argv2));
  const auto &values = parser.GetAllArgValues(OPT_port);
  assert(values.size() == 2);
  assert(values[0] == "1");
  assert(values[1] == "2");

  parser.Reset();
  assert(!parser.HasArg(OPT_port));
  assert(parser.GetAllArgValues(OPT_port).empty());
  #undef ARGS
}

int main() {
  std::cout << "Running Regression Tests" << std::endl;
  std::cout << "=========================" << std::endl;

  RUN_TEST(TestEndOfOptionsDelimiter);
  RUN_TEST(TestSeparateDoesNotSwallowOption);
  RUN_TEST(TestDashAndNegativeValues);
  RUN_TEST(TestJoinedLongestPrefixMatch);
  RUN_TEST(TestCommaListEmptyFieldRejected);
  RUN_TEST(TestInvalidValueListsAllowed);
  RUN_TEST(TestPrintHelpRedirect);
  RUN_TEST(TestParseAppendsAndResetClears);

  std::cout << "=========================" << std::endl;
  std::cout << "All regression tests PASSED!" << std::endl;
  return 0;
}