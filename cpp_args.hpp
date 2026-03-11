#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
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

  // Option kind:
  // - OPTION requires a value (separate argument)
  // - FLAG does not require a value
  // - JOINED allows value to be joined with option name
  enum Kind { OPTION_KIND = 0, FLAG_KIND = 1, JOINED_KIND = 2 };
  Kind kind;

  // Group ID for grouping options in help output
  int groupId;

  // Helper: Normalize name by converting _ to -
  static std::string Normalize(std::string str) {
    for (char &c : str)
      if (c == '_')
        c = '-';
    return str;
  }
};

using OptionTable = std::vector<Option>;

struct OptionGroup {
  int id;
  std::string name;
};

using OptionGroupTable = std::vector<OptionGroup>;

class ArgumentParser {
public:
  explicit ArgumentParser(const OptionTable &table)
      : optionTable(table), allowUnknown(false) {}

  /**
   * @brief Constructor with option groups support
   * @param table The option table
   * @param groups The option group table
   */
  ArgumentParser(const OptionTable &table, const OptionGroupTable &groups)
      : optionTable(table), optionGroups(groups), allowUnknown(false) {}

  /**
   * @brief Fuzzy parsing: treats "_" and "-" as identical.
   * Supports joined options where value is attached to the option name
   * Remaining arguments after known options are treated as inputs
   */
  bool Parse(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];

      // Check if this looks like an option (starts with - or --)
      bool isOption = (arg.size() > 0 && arg[0] == '-');

      // If it doesn't look like an option, treat it as input
      if (!isOption) {
        inputs.push_back(arg);
        continue;
      }

      // Try to find exact match first
      const Option *opt = FindOption(arg);

      // If not found and it's a short option, try to extract joined value
      if (!opt && arg.size() > 2 && arg[0] == '-' && arg[1] != '-') {
        std::string shortName = arg.substr(1, 1);
        std::string value = arg.substr(2);
        opt = FindOptionByShortName(shortName);

        if (opt && opt->kind == Option::JOINED_KIND) {
          // Validate value if allowed values are specified
          if (!opt->allowed.empty() &&
              opt->allowed.find(value) == opt->allowed.end()) {
            std::cerr << "Error: Invalid value '" << value << "' for " << arg
                      << std::endl;
            return false;
          }
          parsedArgs[opt->id].push_back(value);
          continue;
        }
      }

      // If not found and it's a long option with '=', try to extract joined
      // value
      if (!opt && arg.find('=') != std::string::npos && arg.find("--") == 0) {
        size_t eqPos = arg.find('=');
        std::string longName = arg.substr(2, eqPos - 2);
        std::string value = arg.substr(eqPos + 1);
        opt = FindOptionByLongName(longName);

        if (opt && opt->kind == Option::JOINED_KIND) {
          // Validate value if allowed values are specified
          if (!opt->allowed.empty() &&
              opt->allowed.find(value) == opt->allowed.end()) {
            std::cerr << "Error: Invalid value '" << value << "' for " << arg
                      << std::endl;
            return false;
          }
          parsedArgs[opt->id].push_back(value);
          continue;
        }
      }

      if (!opt) {
        // Unknown option
        unknownArgs.push_back(arg);
        if (!allowUnknown) {
          std::string suggestion = FindSimilarOption(arg);
          if (!suggestion.empty()) {
            std::cerr << "Error: Unknown argument '" << arg
                      << "'. Did you mean '" << suggestion << "'?" << std::endl;
          } else {
            std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
          }
          return false;
        }
        continue;
      }

      // If it's a flag, just mark it as present
      if (opt->kind == Option::FLAG_KIND) {
        parsedArgs[opt->id].push_back("");
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
        parsedArgs[opt->id].push_back(value);
      } else {
        std::cerr << "Error: Missing value for " << arg << std::endl;
        return false;
      }
    }
    return true;
  }

  bool HasArg(int id) const { return parsedArgs.find(id) != parsedArgs.end(); }

  /**
   * @brief Get the value of an option (returns last value if specified multiple times)
   * @param id The option enum ID (e.g., OPT_port)
   * @return The option value, or empty string if not provided
   */
  std::string GetArgValue(int id) const {
    auto it = parsedArgs.find(id);
    if (it != parsedArgs.end() && !it->second.empty()) {
      return it->second.back(); // Return last value for backward compatibility
    }
    return "";
  }

  /**
   * @brief Get all values of an option (if specified multiple times)
   * @param id The option enum ID (e.g., OPT_port)
   * @return Vector of all values, or empty vector if not provided
   */
  const std::vector<std::string>& GetAllArgValues(int id) const {
    static const std::vector<std::string> emptyVec;
    auto it = parsedArgs.find(id);
    return (it != parsedArgs.end()) ? it->second : emptyVec;
  }

  /**
   * @brief Get the list of positional inputs (non-option arguments)
   */
  const std::vector<std::string> &GetInputs() const { return inputs; }

  /**
   * @brief Set whether to allow unknown options without error
   * @param allow If true, unknown options are stored but don't cause parse
   * failure
   */
  void SetAllowUnknown(bool allow) { allowUnknown = allow; }

  /**
   * @brief Get the list of unknown options
   * @return Vector of unknown option strings
   */
  const std::vector<std::string> &GetUnknown() const { return unknownArgs; }

  /**
   * @brief Find the most similar option name to the given input
   * @param input The misspelled or unknown option name (e.g. "--verbos" or
   * "-vve")
   * @return The most similar valid option name with prefix (e.g. "--verbose"),
   *         or empty string if no similar option found
   */
  std::string FindSimilarOption(const std::string &input) const {
    if (input.empty())
      return "";

    // Extract the name part (without prefix and potential value)
    std::string cleanInput = input;
    std::string prefix = "-";

    if (input.find("--") == 0) {
      cleanInput = input.substr(2);
      prefix = "--";
    } else if (input.find("-") == 0) {
      cleanInput = input.substr(1);
      prefix = "-";
    }

    // Remove any joined value (after '=' or attached to short option)
    size_t eqPos = cleanInput.find('=');
    if (eqPos != std::string::npos) {
      cleanInput = cleanInput.substr(0, eqPos);
    }

    std::string normInput = Option::Normalize(cleanInput);

    std::string bestMatch;
    int minDistance = std::numeric_limits<int>::max();

    for (const auto &opt : optionTable) {
      int distance;
      std::string candidate;

      // For short options (single character), use short name
      if (cleanInput.length() == 1 && !opt.shortName.empty()) {
        distance = LevenshteinDistance(normInput, opt.shortName);
        candidate = "-" + opt.shortName;
      } else {
        // For long options, compare against normalized long name
        std::string normLongName = Option::Normalize(opt.longName);
        distance = LevenshteinDistance(normInput, normLongName);
        candidate = "--" + normLongName;

        // Also consider short name if it exists and might be a better match
        if (!opt.shortName.empty()) {
          int shortDistance = LevenshteinDistance(normInput, opt.shortName);
          if (shortDistance < distance) {
            distance = shortDistance;
            candidate = "-" + opt.shortName;
          }
        }
      }

      if (distance < minDistance) {
        minDistance = distance;
        bestMatch = candidate;
      }
    }

    // Only return suggestion if it's reasonably close (threshold based on input
    // length)
    int threshold = std::max(2, (int)normInput.length() / 2);
    return (minDistance <= threshold) ? bestMatch : "";
  }

  /**
   * @brief Get the group ID for a given option ID
   * @param optionId The option enum value (e.g., OPT_host)
   * @return The group ID, or -1 if not found or no groups defined
   */
  int GetGroupId(int optionId) const {
    for (const auto &opt : optionTable) {
      if (opt.id == optionId) {
        return opt.groupId;
      }
    }
    return -1;
  }

  void PrintHelp() const {
    std::cout << "Usage Options:" << std::endl;

    // Check if we have groups defined
    if (optionGroups.empty()) {
      // Print all options without grouping
      for (const auto &opt : optionTable) {
        printOption(&opt);
      }
    } else {
      // Group options by groupId
      std::map<int, std::vector<const Option *>> groupedOptions;
      std::vector<const Option *> ungroupedOptions;

      for (const auto &opt : optionTable) {
        // If the option's group has an empty name, treat it as ungrouped
        bool isUngrouped = (opt.groupId < 0 || 
                           opt.groupId >= static_cast<int>(optionGroups.size()) ||
                           optionGroups[opt.groupId].name.empty());
        
        if (!isUngrouped) {
          groupedOptions[opt.groupId].push_back(&opt);
        } else {
          ungroupedOptions.push_back(&opt);
        }
      }

      // Print ungrouped options first
      bool hasUngrouped = !ungroupedOptions.empty();
      for (const auto *opt : ungroupedOptions) {
        printOption(opt);
      }

      // Print grouped options
      bool firstGroup = true;
      for (const auto &group : groupedOptions) {
        int groupId = group.first;
        const auto &options = group.second;

        // Skip if group ID is out of range
        if (groupId >= static_cast<int>(optionGroups.size()))
          continue;

        // Add separator before each group (except possibly the first)
        if (!firstGroup || hasUngrouped) {
          std::cout << std::endl;
        }
        firstGroup = false;

        // Print group header if we have a group name
        if (!optionGroups[groupId].name.empty()) {
          std::cout << optionGroups[groupId].name << ":" << std::endl;
        }

        for (const auto *opt : options) {
          printOption(opt);
        }
      }
    }
  }

private:
  void printOption(const Option *opt) const {
    std::string names = "--" + Option::Normalize(opt->longName);
    if (!opt->shortName.empty())
      names += ", -" + opt->shortName;

    std::cout << "  " << std::left << std::setw(25) << names << opt->help;
    if (!opt->allowed.empty()) {
      std::cout << " [Values: ";
      for (auto it = opt->allowed.begin(); it != opt->allowed.end(); ++it) {
        std::cout << (it == opt->allowed.begin() ? "" : "|") << *it;
      }
      std::cout << "]";
    }
    if (opt->kind == Option::FLAG_KIND) {
      std::cout << " [flag]";
    } else if (opt->kind == Option::JOINED_KIND) {
      std::cout << " [joined]";
    }
    std::cout << std::endl;
  }

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

  const Option *FindOptionByShortName(const std::string &shortName) const {
    for (const auto &opt : optionTable) {
      if (opt.shortName == shortName)
        return &opt;
    }
    return nullptr;
  }

  const Option *FindOptionByLongName(const std::string &longName) const {
    std::string normName = Option::Normalize(longName);
    for (const auto &opt : optionTable) {
      if (Option::Normalize(opt.longName) == normName)
        return &opt;
    }
    return nullptr;
  }

  /**
   * @brief Calculate Levenshtein distance between two strings
   * @param s1 First string
   * @param s2 Second string
   * @return The minimum number of single-character edits required to transform
   * s1 into s2
   */
  int LevenshteinDistance(const std::string &s1, const std::string &s2) const {
    size_t len1 = s1.length();
    size_t len2 = s2.length();

    // Create a matrix
    std::vector<std::vector<int>> matrix(len1 + 1, std::vector<int>(len2 + 1));

    // Initialize first column and row
    for (size_t i = 0; i <= len1; ++i)
      matrix[i][0] = i;
    for (size_t j = 0; j <= len2; ++j)
      matrix[0][j] = j;

    // Compute distances
    for (size_t i = 1; i <= len1; ++i) {
      for (size_t j = 1; j <= len2; ++j) {
        int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
        matrix[i][j] = std::min({
            matrix[i - 1][j] + 1,       // deletion
            matrix[i][j - 1] + 1,       // insertion
            matrix[i - 1][j - 1] + cost // substitution
        });

        // Check for transposition
        if (i > 1 && j > 1 && s1[i - 1] == s2[j - 2] &&
            s1[i - 2] == s2[j - 1]) {
          matrix[i][j] = std::min(matrix[i][j], matrix[i - 2][j - 2] + cost);
        }
      }
    }

    return matrix[len1][len2];
  }

  OptionTable optionTable;
  OptionGroupTable optionGroups;
  std::map<int, std::vector<std::string>> parsedArgs; // Support multiple values per option
  std::vector<std::string> inputs; // Positional inputs (non-option arguments)
  std::vector<std::string> unknownArgs; // Unknown options
  bool allowUnknown; // Whether to allow unknown options without error
};

/**
 * @brief X-Macros for unified argument definition.
 *
 * Usage format:
 * - For options: F(name, short_name, help_text, OPTION, {allowed_values})
 * - For flags: F(name, short_name, help_text, FLAG, {}) - empty allowed values
 * ignored
 * - For joined options: F(name, short_name, help_text, JOINED,
 * {allowed_values}) Joined options allow value to be attached directly (e.g.,
 * -lcuda, --library=cuda)
 */

// Kind identifiers for macro usage
#define OPTION Option::OPTION_KIND
#define FLAG Option::FLAG_KIND
#define JOINED Option::JOINED_KIND

// GENERATE_ENUM takes kind and optional allowed values (ignored for enum
// generation)
#define GENERATE_ENUM(name, sh, help, kind, ...) OPT_##name,

// GENERATE_TABLE uses kind and allowed values
// Parameters for DEFINE_ARGS: name, sh, help, kind, allowed_values...
// This is used by DEFINE_ARGS for non-grouped options (default groupId = -1)
#define MAKE_ALLOWED(...) __VA_ARGS__

#define GENERATE_TABLE(name, sh, help, kind, ...)                              \
  Option{(int)OPT_##name,                                                      \
         #name,                                                                \
         #sh,                                                                  \
         help,                                                                 \
         MAKE_ALLOWED(__VA_ARGS__),                                            \
         static_cast<Option::Kind>(kind),                                      \
         -1},

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
 *    #define ARGS(F)                                                          \
 *       F(port, p, "Server port", OPTION, {})                                 \
 *       F(verbose, v, "Enable verbose", FLAG, {})                             \
 *       F(log_lvl, l, "Log level", OPTION, {"debug", "info"})                 \
 *       F(help, h, "Print help", FLAG, {})                                    \
 *    DEFINE_ARGS(App, AppTable, ARGS)
 *
 */
#define DEFINE_ARGS(EnumName, TableName, ArgsMacro)                            \
  enum EnumName { ArgsMacro(GENERATE_ENUM) EnumName##_COUNT };                 \
  const Option InitList_##TableName[] = {ArgsMacro(GENERATE_TABLE)};           \
  static const OptionTable TableName(InitList_##TableName,                     \
                                     InitList_##TableName +                    \
                                         sizeof(InitList_##TableName) /        \
                                             sizeof(InitList_##TableName[0]));

/**
 * @brief Unified macro for defining all arguments with groups
 * @param ArgEnumName Name of the argument enum to generate (e.g., AppArgs)
 * @param GroupEnumName Name of the group enum to generate (e.g., AppGroups)
 * @param TableName Name of the OptionTable to generate
 * @param ArgsMacro Macro that defines all arguments with format:
 *                  F(group, name, short, help, kind, allowed)
 * @param GroupsMacro Macro that defines all groups with format:
 *                    G(name, display_name)
 *
 * Usage example:
 *    #define GROUPS(F)                                                        \
 *       F(Frontend, "Frontend Options")                                       \
 *       F(Backend, "Backend Options")                                         \
 *       F(Default, "") // Empty name means ungrouped
 *
 *    #define ARGS(F)                                                          \
 *       F(Frontend, host, H, "Host", OPTION, {})                              \
 *       F(Frontend, port, p, "Port", OPTION, {})                              \
 *       F(Backend, db, d, "Database", OPTION, {})                             \
 *       F(Default, verbose, v, "Verbose", FLAG, {})                           \
 *
 *    DEFINE_ARGS_WITH_GROUP(App, AppGroup, AppTable, ARGS, GROUPS)
 */
#define GENERATE_ENUM_WITH_GROUP(group, name, sh, help, kind, ...) OPT_##name,

#define GENERATE_TABLE_WITH_GROUP(group, name, sh, help, kind, ...)            \
  Option{(int)OPT_##name,                                                      \
         #name,                                                                \
         #sh,                                                                  \
         help,                                                                 \
         __VA_ARGS__,                                                          \
         static_cast<Option::Kind>(kind),                                      \
         group##_ID},

#define GENERATE_GROUP_ENUM(name, display) name##_ID,
#define GENERATE_GROUP_INFO(name, display) {name##_ID, display},

#define DEFINE_ARGS_WITH_GROUP(ArgEnumName, GroupEnumName, TableName,          \
                               ArgsMacro, GroupsMacro)                         \
  enum GroupEnumName {                                                         \
    GroupsMacro(GENERATE_GROUP_ENUM) GroupEnumName##_COUNT                     \
  };                                                                           \
  enum ArgEnumName {                                                           \
    ArgsMacro(GENERATE_ENUM_WITH_GROUP) ArgEnumName##_COUNT                    \
  };                                                                           \
  const Option InitList_##TableName[] = {                                      \
      ArgsMacro(GENERATE_TABLE_WITH_GROUP)};                                   \
  static const OptionTable TableName(InitList_##TableName,                     \
                                     InitList_##TableName +                    \
                                         sizeof(InitList_##TableName) /        \
                                             sizeof(InitList_##TableName[0])); \
  const OptionGroup GroupList_##TableName[] = {                                \
      GroupsMacro(GENERATE_GROUP_INFO)};                                       \
  static const OptionGroupTable TableName##Groups(                             \
      GroupList_##TableName,                                                   \
      GroupList_##TableName +                                                  \
          sizeof(GroupList_##TableName) / sizeof(GroupList_##TableName[0]));

#endif
