#include "../cpp_args.hpp"
#include "test_utils.h"
#include <cassert>
#include <iostream>
#include <sstream>

using namespace cppargs;

#define TEST(name) void name()
#define RUN_TEST(name)                                                       \
  do {                                                                       \
  std::cout << "Running " << #name << "... ";                              \
    name();                                                                  \
  std::cout << "PASSED" << std::endl;                                      \
  } while (0)

// Test multiple values for the same option
TEST(TestMultipleValues_SingleOption) {
  #define ARGS(F)                                                            \
   F(config, c, "Config file", SEPARATE, {})                                   \
   F(verbose, v, "Verbose mode", FLAG, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  // Test specifying the same option multiple times
  int argc;
  const char **argv = CreateArgs({"program", "-c", "config1.txt", 
                           "-c", "config2.txt", 
                           "-c", "config3.txt"}, argc);
  
  bool result = parser.Parse(argc, argv);
  assert(result);
  
  // HasArg should return true
  assert(parser.HasArg(OPT_config));
  
  // GetArgValue should return the last value
  assert(parser.GetArgValue(OPT_config) == "config3.txt");
  
  // GetAllArgValues should return all values
  const auto& allValues = parser.GetAllArgValues(OPT_config);
  assert(allValues.size() == 3);
  assert(allValues[0] == "config1.txt");
  assert(allValues[1] == "config2.txt");
  assert(allValues[2] == "config3.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "Single option with multiple values works correctly" << std::endl;
  
  #undef ARGS
}

// Test multiple values with long option names
TEST(TestMultipleValues_LongOption) {
  #define ARGS(F)                                                            \
   F(input, i, "Input file", SEPARATE, {})                                     \
   F(output, o, "Output file", SEPARATE, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  const char **argv = CreateArgs({"program", 
                           "--input", "file1.txt",
                           "--input", "file2.txt",
                           "--output", "out.txt",
                           "--input", "file3.txt"}, argc);
  
  bool result = parser.Parse(argc, argv);
  assert(result);
  
  // GetArgValue returns last value
  assert(parser.GetArgValue(OPT_input) == "file3.txt");
  assert(parser.GetArgValue(OPT_output) == "out.txt");
  
  // GetAllArgValues returns all values
  const auto& inputValues = parser.GetAllArgValues(OPT_input);
  assert(inputValues.size() == 3);
  assert(inputValues[0] == "file1.txt");
  assert(inputValues[1] == "file2.txt");
  assert(inputValues[2] == "file3.txt");
  
  const auto& outputValues = parser.GetAllArgValues(OPT_output);
  assert(outputValues.size() == 1);
  assert(outputValues[0] == "out.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "Long option with multiple values works correctly" << std::endl;
  
  #undef ARGS
}

// Test multiple flag occurrences
TEST(TestMultipleValues_Flag) {
  #define ARGS(F)                                                            \
   F(verbose, v, "Verbose", FLAG, {})                                        \
   F(debug, d, "Debug", FLAG, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  const char **argv = CreateArgs({"program", "-v", "-v", "-v", "-d"}, argc);
  
  bool result = parser.Parse(argc, argv);
  assert(result);
  
  assert(parser.HasArg(OPT_verbose));
  assert(parser.HasArg(OPT_debug));
  
  // For flags, GetAllArgValues returns empty strings
  const auto& verboseValues = parser.GetAllArgValues(OPT_verbose);
  assert(verboseValues.size() == 3); // Flag specified 3 times
  
  const auto& debugValues = parser.GetAllArgValues(OPT_debug);
  assert(debugValues.size() == 1);
  
  CleanupArgs(argv, argc);
  std::cout << "Flag with multiple occurrences works correctly" << std::endl;
  
  #undef ARGS
}

// Test mixed usage - some options once, some multiple times
TEST(TestMultipleValues_Mixed) {
  #define ARGS(F)                                                            \
   F(host, H, "Host", SEPARATE, {})                                            \
   F(port, p, "Port", SEPARATE, {"8080", "9090"})                              \
   F(tag, t, "Tag", SEPARATE, {})                                              \
   F(verbose, v, "Verbose", FLAG, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  const char **argv = CreateArgs({"program",
                           "-H", "localhost",
                           "-p", "8080",
                           "-t", "tag1",
                           "-v",
                           "-t", "tag2",
                           "-t", "tag3",
                           "-p", "9090"}, argc);
  
  bool result = parser.Parse(argc, argv);
  assert(result);
  
  // Single occurrence
  assert(parser.GetArgValue(OPT_host) == "localhost");
  const auto& hostValues = parser.GetAllArgValues(OPT_host);
  assert(hostValues.size() == 1);
  assert(hostValues[0] == "localhost");
  
  // Multiple occurrences with validation
  assert(parser.GetArgValue(OPT_port) == "9090"); // Last value
  const auto& portValues = parser.GetAllArgValues(OPT_port);
  assert(portValues.size() == 2);
  assert(portValues[0] == "8080");
  assert(portValues[1] == "9090");
  
  // Multiple occurrences
  const auto& tagValues = parser.GetAllArgValues(OPT_tag);
  assert(tagValues.size() == 3);
  assert(tagValues[0] == "tag1");
  assert(tagValues[1] == "tag2");
  assert(tagValues[2] == "tag3");
  
  // Flag
  assert(parser.HasArg(OPT_verbose));
  const auto& verboseValues = parser.GetAllArgValues(OPT_verbose);
  assert(verboseValues.size() == 1);
  
  CleanupArgs(argv, argc);
  std::cout << "Mixed usage with multiple values works correctly" << std::endl;
  
  #undef ARGS
}

// Test backward compatibility - GetArgValue still works as before
TEST(TestMultipleValues_BackwardCompatibility) {
  #define ARGS(F)                                                            \
   F(file, f, "File", SEPARATE, {})
  
  DEFINE_ARGS(AppArgs, AppTable, ARGS)
  
  ArgumentParser parser(AppTable);
  
  int argc;
  const char **argv = CreateArgs({"program", "-f", "single.txt"}, argc);
  
  bool result = parser.Parse(argc, argv);
  assert(result);
  
  // Old code using GetArgValue should still work
  std::string value = parser.GetArgValue(OPT_file);
  assert(value == "single.txt");
  
  // New API also available
  const auto& allValues = parser.GetAllArgValues(OPT_file);
  assert(allValues.size() == 1);
  assert(allValues[0] == "single.txt");
  
  CleanupArgs(argv, argc);
  std::cout << "Backward compatibility maintained" << std::endl;
  
  #undef ARGS
}

int main() {
  try {
    RUN_TEST(TestMultipleValues_SingleOption);
    RUN_TEST(TestMultipleValues_LongOption);
    RUN_TEST(TestMultipleValues_Flag);
    RUN_TEST(TestMultipleValues_Mixed);
    RUN_TEST(TestMultipleValues_BackwardCompatibility);
    
    std::cout << "\nAll tests PASSED!" << std::endl;
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test failed: " << e.what() << std::endl;
    return 1;
  }
}
