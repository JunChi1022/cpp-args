#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>
#include <sstream>

#define TEST(name) void name()
#define RUN_TEST(name)                                                       \
  do {                                                                       \
  std::cout << "Running " << #name << "... ";                              \
    name();                                                                  \
  std::cout << "PASSED" << std::endl;                                      \
  } while (0)

// Test basic option group functionality
TEST(TestOptionGroup_BasicGroup) {
  #define GROUPS(F)                                                          \
   F(Frontend, "Frontend Options")                                           \
   F(Backend, "Backend Options")
  
  #define ARGS(F)                                                            \
   F(Frontend, host, H, "Host", OPTION, {})                                  \
   F(Frontend, port, p, "Port", OPTION, {})                                  \
   F(Backend, db, d, "Database", OPTION, {})                                 \
   F(Backend, cache, c, "Cache", OPTION, {})
  
  DEFINE_ARGS_WITH_GROUP(AppArgs, AppGroups, AppTable, ARGS, GROUPS)
  
  // Create parser with groups support
  ArgumentParser parser(AppTable, AppTableGroups);
  
  // Test that options are parsed correctly
  int argc;
  char **argv = CreateArgs({"program", "--host", "localhost", "--port", "8080", 
                           "--db", "mysql", "--cache", "redis"}, argc);
  
  bool result = parser.Parse(argc, argv);
  assert(result);
  
  assert(parser.HasArg(OPT_host));
  assert(parser.GetArgValue(OPT_host) == "localhost");
  assert(parser.HasArg(OPT_port));
  assert(parser.GetArgValue(OPT_port) == "8080");
  assert(parser.HasArg(OPT_db));
  assert(parser.GetArgValue(OPT_db) == "mysql");
  assert(parser.HasArg(OPT_cache));
  assert(parser.GetArgValue(OPT_cache) == "redis");
  
  CleanupArgs(argv, argc);
  std::cout << "All group options parsed correctly" << std::endl;
  
  #undef GROUPS
  #undef ARGS
}

// Test GetGroupId interface
TEST(TestOptionGroup_GetGroupId) {
  #define GROUPS(F)                                                          \
   F(Frontend, "Frontend Options")                                           \
   F(Backend, "Backend Options")
  
  #define ARGS(F)                                                            \
   F(Frontend, host, H, "Host", OPTION, {})                                  \
   F(Frontend, port, p, "Port", OPTION, {})                                  \
   F(Backend, db, d, "Database", OPTION, {})
  
  DEFINE_ARGS_WITH_GROUP(AppArgs, AppGroups, AppTable, ARGS, GROUPS)
  
  // Create parser with groups support
  ArgumentParser parser(AppTable, AppTableGroups);
  
  // Test group ID lookup
  assert(parser.GetGroupId(OPT_host) == Frontend_ID);
  assert(parser.GetGroupId(OPT_port) == Frontend_ID);
  assert(parser.GetGroupId(OPT_db) == Backend_ID);
  
  std::cout << "GetGroupId returns correct values" << std::endl;
  
  #undef GROUPS
  #undef ARGS
}

// Test mixed grouped and non-grouped options
TEST(TestOptionGroup_MixedGroups) {
  #define GROUPS(F)                                                          \
   F(Frontend, "Frontend Options")                                           \
   F(Default, "")
  
  #define ARGS(F)                                                            \
   F(Default, verbose, v, "Verbose mode", FLAG, {})                          \
   F(Default, help, h, "Print help", FLAG, {})                               \
   F(Frontend, host, H, "Host", OPTION, {})                                  \
   F(Frontend, port, p, "Port", OPTION, {})
  
  DEFINE_ARGS_WITH_GROUP(AppArgs, AppGroups, AppTable, ARGS, GROUPS)
  
  // Create parser with groups support
  ArgumentParser parser(AppTable, AppTableGroups);
  
  // Test parsing
  int argc;
  char **argv = CreateArgs({"program", "-v", "--host", "localhost", "-p", "9090"}, argc);
  
  bool result = parser.Parse(argc, argv);
  assert(result);
  
  assert(parser.HasArg(OPT_verbose));
  assert(parser.HasArg(OPT_host));
  assert(parser.GetArgValue(OPT_host) == "localhost");
  assert(parser.HasArg(OPT_port));
  assert(parser.GetArgValue(OPT_port) == "9090");
  
  // Test group assignment
  assert(parser.GetGroupId(OPT_verbose) == Default_ID); // Default group
  assert(parser.GetGroupId(OPT_help) == Default_ID);
  assert(parser.GetGroupId(OPT_host) == Frontend_ID);
  assert(parser.GetGroupId(OPT_port) == Frontend_ID);
  
  CleanupArgs(argv, argc);
  std::cout << "Mixed groups work correctly" << std::endl;
  
  #undef GROUPS
  #undef ARGS
}

// Test PrintHelp with groups
TEST(TestOptionGroup_PrintHelp) {
  #define GROUPS(F)                                                          \
   F(Frontend, "Frontend Options")                                           \
   F(Backend, "Backend Options")                                             \
   F(Default, "")
  
  #define ARGS(F)                                                            \
   F(Default, verbose, v, "Verbose", FLAG, {})                               \
   F(Frontend, host, H, "Host", OPTION, {})                                  \
   F(Frontend, port, p, "Port", OPTION, {})                                  \
   F(Backend, db, d, "Database", OPTION, {})                                 \
   F(Backend, cache, c, "Cache", OPTION, {})
  
  DEFINE_ARGS_WITH_GROUP(AppArgs, AppGroups, AppTable, ARGS, GROUPS)
  
  // Create parser with groups support
  ArgumentParser parser(AppTable, AppTableGroups);
  
  // Capture stdout
  std::streambuf *sbuf = std::cout.rdbuf();
  std::ostringstream captured;
  std::cout.rdbuf(captured.rdbuf());
  
  parser.PrintHelp();
  
  // Restore stdout
  std::cout.rdbuf(sbuf);
  
  std::string output = captured.str();
  
  // Verify output contains group names
  assert(output.find("Frontend Options:") != std::string::npos);
  assert(output.find("Backend Options:") != std::string::npos);
  assert(output.find("--host") != std::string::npos);
  assert(output.find("--port") != std::string::npos);
  assert(output.find("--db") != std::string::npos);
  assert(output.find("--cache") != std::string::npos);
  assert(output.find("--verbose") != std::string::npos);
  
  std::cout << "PrintHelp output contains group headers" << std::endl;
  
  #undef GROUPS
  #undef ARGS
}

// Test three groups
TEST(TestOptionGroup_ThreeGroups) {
  #define GROUPS(F)                                                          \
   F(Frontend, "Frontend Options")                                           \
   F(Backend, "Backend Options")                                             \
   F(Utils, "Utility Options")
  
  #define ARGS(F)                                                            \
   F(Frontend, host, H, "Host", OPTION, {})                                  \
   F(Backend, db, d, "Database", OPTION, {})                                 \
   F(Utils, verbose, v, "Verbose", FLAG, {})
  
  DEFINE_ARGS_WITH_GROUP(AppArgs, AppGroups, AppTable, ARGS, GROUPS)
  
  // Create parser with groups support
  ArgumentParser parser(AppTable, AppTableGroups);
  
  int argc;
  char **argv = CreateArgs({"program", "--host", "localhost", "--db", "postgres", "-v"}, argc);
  
  bool result = parser.Parse(argc, argv);
  assert(result);
  
  assert(parser.HasArg(OPT_host));
  assert(parser.HasArg(OPT_db));
  assert(parser.HasArg(OPT_verbose));
  
  CleanupArgs(argv, argc);
  std::cout << "Three groups work correctly" << std::endl;
  
  #undef GROUPS
  #undef ARGS
}

int main() {
  try {
    RUN_TEST(TestOptionGroup_BasicGroup);
    RUN_TEST(TestOptionGroup_GetGroupId);
    RUN_TEST(TestOptionGroup_MixedGroups);
    RUN_TEST(TestOptionGroup_PrintHelp);
    RUN_TEST(TestOptionGroup_ThreeGroups);
    
  std::cout << "\nAll tests PASSED!" << std::endl;
    return 0;
  } catch (const std::exception &e) {
  std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
