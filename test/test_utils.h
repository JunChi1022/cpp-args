#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <cstring>
#include <iostream>
#include <vector>
#include <string>

// Simple test macros shared by all test executables
#define TEST(name) void name()

#define RUN_TEST(name)                                                       \
  do {                                                                       \
    std::cout << "Running " << #name << "... ";                              \
    name();                                                                  \
    std::cout << "PASSED" << std::endl;                                      \
  } while (0)

/**
 * @brief Helper to create argc/argv from string array
 */
inline const char **CreateArgs(const std::vector<std::string> &args, int &argc) {
  argc = args.size();
  const char **argv = new const char *[argc];
  for (int i = 0; i < argc; ++i) {
    argv[i] = new char[args[i].size() + 1];
    strcpy(const_cast<char*>(argv[i]), args[i].c_str());
  }
  return argv;
}

/**
 * @brief Helper to cleanup args
 */
inline void CleanupArgs(const char **argv, int argc) {
  for (int i = 0; i < argc; ++i) {
    delete[] const_cast<char*>(argv[i]);
  }
  delete[] argv;
}

#endif // TEST_UTILS_HPP
