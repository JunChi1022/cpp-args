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
 * @brief Minimalist X-Macros.
 * name: The unique identifier. Must use _ in macro (e.g. log_lvl).
 */
#define GENERATE_ENUM(name, sh, help, ...) OPT_##name,

#define GENERATE_TABLE(name, sh, help, ...)                                    \
  Option{(int)OPT_##name, #name, #sh, help, __VA_ARGS__},

#define DEFINE_ARGUMENTS(EnumName, TableName, ListMacro)                       \
  enum EnumName { ListMacro(GENERATE_ENUM) EnumName##_COUNT };                 \
  static const OptionTable TableName = {ListMacro(GENERATE_TABLE)};

#endif
