#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP

#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

/**
 * @brief Option metadata.
 */
struct Option {
  int id;
  std::string longName;  // Raw name from macro (e.g. "log_lvl")
  std::string shortName; // e.g. "l"
  std::string help;
  std::set<std::string> allowed;

  // Option kind: OPTION requires a value, FLAG does not
  enum Kind { OPTION_KIND = 0, FLAG_KIND = 1 };
  Kind kind;

  // Helper: Normalize name by converting _ to -
  static std::string Normalize(std::string str) {
    for (char &c : str)
      if (c == '_')
        c = '-';
    return str;
  }
};

using OptionTable = std::vector<Option>;

class ArgumentParser {
public:
  explicit ArgumentParser(const OptionTable &table) : optionTable(table) {}

  /**
   * @brief Fuzzy parsing: treats "_" and "-" as identical.
   */
  bool Parse(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      const Option *opt = FindOption(arg);

      if (!opt) {
        std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
        return false;
      }

      // If it's a flag, just mark it as present
      if (opt->kind == Option::FLAG_KIND) {
        parsedArgs[opt->id] = "";
        continue;
      }

      // For non-flag options, require a value
      if (i + 1 < argc) {
        std::string value = argv[++i];
        if (!opt->allowed.empty() &&
            opt->allowed.find(value) == opt->allowed.end()) {
          std::cerr << "Error: Invalid value '" << value << "' for " << arg
                    << std::endl;
          return false;
        }
        parsedArgs[opt->id] = value;
      } else {
        std::cerr << "Error: Missing value for " << arg << std::endl;
        return false;
      }
    }
    return true;
  }

  bool HasArg(int id) const { return parsedArgs.find(id) != parsedArgs.end(); }

  std::string GetArgValue(int id) const {
    auto it = parsedArgs.find(id);
    return (it != parsedArgs.end()) ? it->second : "";
  }

  void PrintHelp() const {
    std::cout << "Usage Options:" << std::endl;
    for (const auto &opt : optionTable) {
      // Display with hyphens for better CLI aesthetics
      std::string names = "--" + Option::Normalize(opt.longName);
      if (!opt.shortName.empty())
        names += ", -" + opt.shortName;

      std::cout << "  " << std::left << std::setw(25) << names << opt.help;
      if (!opt.allowed.empty()) {
        std::cout << " [Values: ";
        for (auto it = opt.allowed.begin(); it != opt.allowed.end(); ++it) {
          std::cout << (it == opt.allowed.begin() ? "" : "|") << *it;
        }
        std::cout << "]";
      }
      if (opt.kind == Option::FLAG_KIND) {
        std::cout << " [flag]";
      }
      std::cout << std::endl;
    }
  }

private:
  const Option *FindOption(const std::string &input) const {
    if (input.empty())
      return nullptr;

    // Strip prefix (-- or -) and normalize the input
    std::string cleanInput = input;
    if (cleanInput.find("--") == 0)
      cleanInput = cleanInput.substr(2);
    else if (cleanInput.find("-") == 0)
      cleanInput = cleanInput.substr(1);
    std::string normInput = Option::Normalize(cleanInput);

    for (const auto &opt : optionTable) {
      // Match normalized long name OR short name
      if (Option::Normalize(opt.longName) == normInput)
        return &opt;
      if (opt.shortName == normInput)
        return &opt;
    }
    return nullptr;
  }

  OptionTable optionTable;
  std::map<int, std::string> parsedArgs;
};

/**
 * @brief X-Macros for unified argument definition.
 *
 * Usage format:
 * - For options: F(name, short_name, help_text, OPTION, {allowed_values})
 * - For flags: F(name, short_name, help_text, FLAG, {}) - empty allowed values
 * ignored
 */

// Kind identifiers for macro usage
#define OPTION Option::OPTION_KIND
#define FLAG Option::FLAG_KIND

// GENERATE_ENUM takes kind and optional allowed values (ignored for enum
// generation)
#define GENERATE_ENUM(name, sh, help, kind, ...) OPT_##name,

// GENERATE_TABLE uses kind and allowed values
// We need to handle the case where allowed is empty or has values
// Using a wrapper to handle the initializer list properly
#define MAKE_ALLOWED(...) __VA_ARGS__

#define GENERATE_TABLE(name, sh, help, kind, ...)                              \
  Option{(int)OPT_##name,                                                      \
         #name,                                                                \
         #sh,                                                                  \
         help,                                                                 \
         MAKE_ALLOWED(__VA_ARGS__),                                            \
         static_cast<Option::Kind>(kind)},

/**
 * @brief Unified macro for defining all arguments in a single macro
 * @param EnumName Name of the enum to generate
 * @param TableName Name of the OptionTable to generate
 * @param ArgsMacro Macro that defines all arguments (both options and flags)
 *
 * Note: Both OPTION and FLAG types require the allowed_values parameter.
 * For FLAGs, this should be an empty set {} as they don't accept values.
 *
 * Usage examples:
 *
 * 1. Mixed options and flags:
 *    #define ARGS(F)                                                          \
 *       F(port, p, "Server port", OPTION, {})                                 \
 *       F(verbose, v, "Enable verbose", FLAG, {})                             \
 *       F(log_lvl, l, "Log level", OPTION, {"debug", "info"})                 \
 *       F(help, h, "Print help", FLAG, {})                                    \
 *    DEFINE_ARGS(App, AppTable, ARGS)
 *
 * 2. Options only:
 *    #define ARGS(F)                                                          \
 *       F(port, p, "Port", OPTION, {})                                        \
 *       F(host, H, "Host", OPTION, {})                                        \
 *    DEFINE_ARGS(App, AppTable, ARGS)
 *
 * 3. Flags only:
 *    #define ARGS(F)                                                          \
 *       F(help, h, "Help", FLAG, {})                                          \
 *       F(verbose, v, "Verbose", FLAG, {})                                   \
 *    DEFINE_ARGS(App, AppTable, ARGS)
 */
#define DEFINE_ARGS(EnumName, TableName, ArgsMacro)                            \
  enum EnumName { ArgsMacro(GENERATE_ENUM) EnumName##_COUNT };                 \
  const Option InitList_##TableName[] = {ArgsMacro(GENERATE_TABLE)};           \
  static const OptionTable TableName(InitList_##TableName,                     \
                                     InitList_##TableName +                    \
                                         sizeof(InitList_##TableName) /        \
                                             sizeof(InitList_##TableName[0]));

#endif
