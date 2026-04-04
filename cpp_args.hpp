#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace cppargs {

/**
 * @brief Parse result status for option parsing
 */
enum class ParseResult {
  Success,    // Successfully parsed
  Failed,     // Failed to parse (e.g., invalid value) - option was recognized
  NotAnOption // This is not the type of option we're looking for
};

/**
 * @brief Option metadata.
 */
struct Option {
  int id;
  std::string longName;  // Raw name from macro (e.g. "log_lvl")
  std::string shortName; // e.g. "l"
  std::string help;
  std::set<std::string> allowed;

  // Option feature using bit flags (one-hot encoding):
  // - SEPARATE: value separate from option (space or '=')
  // - JOINED: value attached directly to option name
  // - FLAG: no value required
  // - SHORT: long option can be used with single dash (e.g., -help)
  // Can be combined: SEPARATE | JOINED = both formats supported
  enum Feature {
    FEAT_SEPARATE = 1, // Bit 0: separate format support (--option value)
    FEAT_FLAG = 2,     // Bit 1: flag (no value)
    FEAT_JOINED = 4,   // Bit 2: joined format support (--optionvalue)
    FEAT_HIDDEN = 8,   // Bit 3: Hidden option (will not be printed in help)
    FEAT_SHORT =
        16, // Bit 4: Long option also accepts single dash (e.g., -help)
    FEAT_EQ_JOIN = 32 // Bit 5: equals join format support (--option=value)
  };

  int feature; // Changed to int to support bit combinations

  // Helper to check if feature has specific flag
  bool HasFeature(int flag) const { return (feature & flag) != 0; }

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

/**
 * @brief Alias mapping: maps alias names to option IDs
 */
struct AliasEntry {
  std::string aliasName;      // The long alias name (e.g., "save_temps")
  std::string shortAliasName; // The short alias name (e.g., "st"), can be empty
  std::string optionName; // The actual option name it maps to (e.g., "keep")
};

using AliasTable = std::vector<AliasEntry>;

/**
 * @brief Alias lookup map for efficient runtime lookup
 * Maps normalized alias name -> option name
 */
using AliasMap = std::map<std::string, std::string>;

class ArgumentParser {
public:
  explicit ArgumentParser(const OptionTable &table)
      : optionTable(table), allowUnknown(false), errStream(&std::cerr) {}

  /**
   * @brief Constructor with option groups support
   * @param table The option table
   * @param groups The option group table
   */
  ArgumentParser(const OptionTable &table, const OptionGroupTable &groups)
      : optionTable(table), optionGroups(groups), allowUnknown(false),
        errStream(&std::cerr) {}

  /**
   * @brief Set the error message output stream
   * @param stream Pointer to the output stream (default is std::cerr)
   *
   * This allows users to redirect error messages to a custom stream,
   * such as std::ostringstream for testing or logging purposes.
   */
  void SetErrMsgStream(std::ostream *stream) {
    errStream = stream ? stream : &std::cerr;
  }

  /**
   * @brief Set alias table for option name aliases
   * @param aliasTable The alias table mapping alias names to option IDs
   */
  void SetAliasTable(const AliasTable &aliasTable) {
    this->aliasTable = aliasTable;

    // Build lookup map for efficient runtime lookup
    aliasMap.clear();
    for (const auto &alias : aliasTable) {
      // Map long alias name (normalized)
      std::string normLongAlias = Option::Normalize(alias.aliasName);
      aliasMap[normLongAlias] = alias.optionName;

      // Map short alias name if present
      if (!alias.shortAliasName.empty()) {
        aliasMap[alias.shortAliasName] = alias.optionName;
      }
    }
  }

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

      // Try parsing in order of specificity:
      // 1. Flag (no value needed)
      ParseResult flagResult = ParseFlag(arg);
      if (flagResult == ParseResult::Success) {
        continue;
      } else if (flagResult == ParseResult::Failed) {
        // Option was recognized but failed (e.g., invalid value)
        return false;
      }
      // NotAnOption - continue to next parser

      // 2. Separate format (--option value)
      ParseResult separateResult = ParseSeparate(arg, i, argc, argv);
      if (separateResult == ParseResult::Success) {
        continue;
      } else if (separateResult == ParseResult::Failed) {
        // Option was recognized but failed (e.g., missing/invalid value)
        return false;
      }
      // NotAnOption - continue to next parser

      // 3. Equals join format (--option=value)
      ParseResult eqJoinResult = ParseEqJoin(arg);
      if (eqJoinResult == ParseResult::Success) {
        continue;
      } else if (eqJoinResult == ParseResult::Failed) {
        // Option was recognized but failed (e.g., invalid value)
        return false;
      }
      // NotAnOption - continue to next parser

      // 4. Joined format (--optionvalue or -Ivalue)
      ParseResult joinedResult = ParseJoined(arg);
      if (joinedResult == ParseResult::Success) {
        continue;
      } else if (joinedResult == ParseResult::Failed) {
        // Option was recognized but failed (e.g., invalid value)
        return false;
      }
      // NotAnOption - continue to unknown handling

      // Unknown option - not recognized by any parser
      unknownArgs.push_back(arg);
      if (!allowUnknown) {
        std::string suggestion = Option::Normalize(FindSimilarOption(arg));
        if (!suggestion.empty()) {
          *errStream << "Unknown argument '" << arg << "'. Did you mean '"
                     << suggestion << "'?" << std::endl;
        } else {
          *errStream << "Unknown argument '" << arg << "'" << std::endl;
        }
        return false;
      }
    }
    return true;
  }

  bool HasArg(int id) const { return parsedArgs.find(id) != parsedArgs.end(); }

  /**
   * @brief Check if any of the specified options are present
   * @param first The first option ID
   * @param args Additional option IDs to check
   * @return true if any of the specified options are present, false otherwise
   */
  template <typename... Args> bool HasArg(int first, Args... args) const {
    if (HasArg(first))
      return true;
    return sizeof...(args) > 0 && HasArg(args...);
  }

  /**
   * @brief Get the value of an option (returns last value if specified multiple
   * times)
   * @param id The option enum ID (e.g., OPT_port)
   * @param defaultValue Default value to return if the option is not provided
   * @return The option value, or defaultValue if not provided
   */
  std::string GetArgValue(int id, const std::string &defaultValue = "") const {
    auto it = parsedArgs.find(id);
    if (it != parsedArgs.end() && !it->second.empty()) {
      return it->second.back(); // Return last value for backward compatibility
    }
    return defaultValue;
  }

  /**
   * @brief Get all values of an option (if specified multiple times)
   * @param id The option enum ID (e.g., OPT_port)
   * @return Vector of all values, or empty vector if not provided
   */
  const std::vector<std::string> &GetAllArgValues(int id) const {
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

      // For long options, compare against normalized long name
      std::string normLongName;
      if (opt.HasFeature(Option::FEAT_SHORT)) {
        normLongName = "-" + Option::Normalize(opt.longName);
      } else {
        normLongName = "--" + Option::Normalize(opt.longName);
      }
      distance = LevenshteinDistance(normInput, normLongName);
      candidate = normLongName;

      // Also consider short name if it exists and might be a better match
      if (!opt.shortName.empty()) {
        int shortDistance = LevenshteinDistance(normInput, "-" + opt.shortName);
        if (shortDistance < distance) {
          distance = shortDistance;
          candidate = "-" + opt.shortName;
        }
      }

      if (opt.HasFeature(Option::FEAT_EQ_JOIN)) {
        candidate += "=";
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

  /**
   * @brief Render an option back to command-line format based on parse results
   * @param optionId The option enum value to render
   * @param renderArgs Output vector to append rendered arguments
   *
   * This function reconstructs command-line arguments from parsed values.
   * Rendering strategy based on option type:
   * - SEPARATE (only): Uses space-separated format (--option value)
   * - JOINED (only): Uses joined format (--optionvalue)
   * - SEPARATE | JOINED: Uses space-separated format by default (--option
   * value)
   * - FLAG | SHORT: Uses single dash format (-option)
   * - SEPARATE | SHORT: Uses single dash format with space-separated value
   * (-option value)
   * - FLAG (no SHORT): Uses double dash format (--option)
   * For options with multiple values, each value is rendered separately.
   */
  void Render(int optionId, std::vector<std::string> &renderArgs) const {
    // Find the option in the table
    const Option *opt = nullptr;
    for (const auto &o : optionTable) {
      if (o.id == optionId) {
        opt = &o;
        break;
      }
    }

    if (!opt) {
      return; // Option not found
    }

    // Check if this option was parsed
    auto it = parsedArgs.find(optionId);
    if (it == parsedArgs.end()) {
      return; // Option was not specified
    }

    const auto &values = it->second;
    std::string normalizedName = Option::Normalize(opt->longName);

    // Determine rendering format based on option type
    bool isFlag = opt->HasFeature(Option::FEAT_FLAG);
    bool hasShortFeature = opt->HasFeature(Option::FEAT_SHORT);
    bool isJoinedOnly = opt->HasFeature(Option::FEAT_JOINED) &&
                        !opt->HasFeature(Option::FEAT_SEPARATE) &&
                        !opt->HasFeature(Option::FEAT_EQ_JOIN);
    bool hasEqJoin = opt->HasFeature(Option::FEAT_EQ_JOIN);
    bool hasSeparate = opt->HasFeature(Option::FEAT_SEPARATE);

    // Rendering priority:
    // 1. If has SEPARATE feature, use space-separated format (--option value)
    // 2. Else if has EQ_JOIN feature, use equals format (--option=value)
    // 3. Else if pure JOINED, use joined format (--optionvalue)

    // Render each value occurrence
    for (size_t i = 0; i < values.size(); ++i) {
      if (isFlag) {
        // For flags with SHORT feature, use single dash format (-option)
        // For normal flags, use double dash format (--option)
        if (hasShortFeature) {
          renderArgs.push_back("-" + normalizedName);
        } else {
          renderArgs.push_back("--" + normalizedName);
        }
      } else if (hasShortFeature && hasSeparate) {
        // For SEPARATE|SHORT options, use single dash with space-separated
        // value
        renderArgs.push_back("-" + normalizedName);
        renderArgs.push_back(values[i]);
      } else if (hasShortFeature && hasEqJoin && !hasSeparate) {
        // For EQ_JOIN|SHORT (without SEPARATE) options, use single dash with
        // equals format
        renderArgs.push_back("-" + normalizedName + "=" + values[i]);
      } else if (hasShortFeature) {
        // Fallback for other SHORT combinations
        renderArgs.push_back("-" + normalizedName);
        renderArgs.push_back(values[i]);
      } else if (isJoinedOnly) {
        // For pure JOINED options (not SEPARATE|JOINED or EQ_JOIN|JOINED),
        // render as --optionvalue
        if (!values[i].empty()) {
          renderArgs.push_back("--" + normalizedName + values[i]);
        } else {
          // Edge case: empty value for joined option
          renderArgs.push_back("--" + normalizedName);
        }
      } else if (hasEqJoin && !hasSeparate) {
        // For EQ_JOIN or EQ_JOIN|JOINED options (without SEPARATE), render as
        // --option=value
        renderArgs.push_back("--" + normalizedName + "=" + values[i]);
      } else {
        // For SEPARATE or SEPARATE|JOINED or SEPARATE|EQ_JOIN options, render
        // as --option value (space-separated)
        renderArgs.push_back("--" + normalizedName);
        renderArgs.push_back(values[i]);
      }
    }
  }

  virtual void PrintHelp(int wrappedWidth = 100) const {
    // Check if we have groups defined
    if (optionGroups.empty()) {
      // Print all options without grouping
      for (const auto &opt : optionTable) {
        printOption(&opt, wrappedWidth);
      }
    } else {
      // Group options by groupId
      std::map<int, std::vector<const Option *>> groupedOptions;
      std::vector<const Option *> ungroupedOptions;

      for (const auto &opt : optionTable) {
        // If the option's group has an empty name, treat it as ungrouped
        bool isUngrouped =
            (opt.groupId < 0 ||
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
        printOption(opt, wrappedWidth);
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
          std::cout << std::string(wrappedWidth, '=') << std::endl;
        }

        for (const auto *opt : options) {
          printOption(opt, wrappedWidth);
        }
      }
    }
  }

private:
  virtual void printOption(const Option *opt, int wrappedWidth = 100) const {
    if (opt->HasFeature(Option::FEAT_HIDDEN)) {
      return;
    }

    // brief = longName [kind] [allow values]
    // For FEAT_SHORT options, show both --option and -option formats
    std::string brief;
    if (opt->HasFeature(Option::FEAT_SHORT)) {
      brief = "-" + Option::Normalize(opt->longName);
    } else {
      brief = "--" + Option::Normalize(opt->longName);
    }

    if (opt->HasFeature(Option::FEAT_FLAG)) {
      brief += " [flag]";
    }
    if (opt->HasFeature(Option::FEAT_EQ_JOIN)) {
      brief += "=";
    }

    if (!opt->allowed.empty()) {
      brief += " [Values: ";
      for (auto it = opt->allowed.begin(); it != opt->allowed.end(); ++it) {
        if (brief.size() >= wrappedWidth / 2) {
          brief += "|...";
          break;
        }
        brief += (it == opt->allowed.begin() ? "" : "|");
        brief += *it;
      }
      brief += "]";
    }

    std::cout << std::left << std::setw(wrappedWidth - 30) << brief;

    if (!opt->shortName.empty()) {
      std::cout << "(-" << Option::Normalize(opt->shortName) << ")";
    }
    std::cout << std::endl;

    std::istringstream iss(opt->help);
    std::string word;
    std::string line;

    while (iss >> word) {
      if (!line.empty() && line.length() + 1 + word.length() >
                               static_cast<size_t>(wrappedWidth - 4)) {
        std::cout << "    " << line << std::endl;
        line = word;
      } else {
        if (line.empty()) {
          line = word;
        } else {
          line += " " + word;
        }
      }
    }
    if (!line.empty()) {
      std::cout << "    " << line << std::endl;
    }

    // print alias
    for (const auto &it : aliasTable) {
      if (it.optionName == opt->longName) {
        if (it.aliasName.empty()) {
          // only short alias name
          std::cout << std::left << std::setw(wrappedWidth - 30)
                    << ("-" + it.shortAliasName);
        } else {
          std::cout << std::left << std::setw(wrappedWidth - 30)
                    << ("--" + it.aliasName);
          if (!it.shortAliasName.empty()) {
            std::cout << "(-" << it.shortAliasName << ")";
          }
        }
        std::cout << std::endl;
        std::cout << "    Alias for " << "--"
                  << Option::Normalize(opt->longName) << std::endl;
      }
    }
  }

  /**
   * @brief Check if an option value is valid and report error if invalid
   * @param opt The option to check against
   * @param value The value to validate
   * @return true if the value is valid (or no validation required), false if
   * invalid
   *
   * This function provides a centralized way to validate option values,
   * checking against allowed values set and reporting errors with consistent
   * formatting across all parsing functions.
   */
  bool CheckOptionValue(const Option *opt, const std::string &value) const {
    if (!opt)
      return false;

    // If no allowed values specified, the value is always valid
    if (opt->allowed.empty())
      return true;

    // Check if value is in the allowed set
    if (opt->allowed.find(value) == opt->allowed.end()) {
      // Value is invalid - report error
      ReportInvalidValue(opt, value);
      return false;
    }

    return true;
  }

  void ReportInvalidValue(const Option *opt, const std::string &value) const {
    if (!opt)
      return;

    // Determine the option prefix based on option features
    // Always use normalized name (with '-' instead of '_')
    std::string optionName;
    if (opt->HasFeature(Option::FEAT_SHORT)) {
      // For FEAT_SHORT options, show both formats
      optionName = "-" + Option::Normalize(opt->longName);
    } else if (!opt->shortName.empty()) {
      // Show both long and short forms if short name exists
      optionName = "--" + Option::Normalize(opt->longName) + "(-" +
                   Option::Normalize(opt->shortName) + ")";
    } else {
      // Only long name available
      optionName = "--" + Option::Normalize(opt->longName);
    }

    *errStream << "Invalid value '" << value << "' for " << optionName
               << std::endl;
  }

  const Option *FindOption(const std::string &input) const {
    if (input.empty())
      return nullptr;

    // Strip prefix (-- or -) and normalize the input
    std::string cleanInput = input;
    bool isSingleDash = false;
    bool isDoubleDash = false;
    if (cleanInput.find("--") == 0) {
      cleanInput = cleanInput.substr(2);
      isDoubleDash = true;
    } else if (cleanInput.find("-") == 0) {
      cleanInput = cleanInput.substr(1);
      isSingleDash = true;
    }
    std::string normInput = Option::Normalize(cleanInput);

    for (const auto &opt : optionTable) {
      // Match short name ONLY with single dash format
      if (isSingleDash && Option::Normalize(opt.shortName) == normInput)
        return &opt;

      // For FEAT_SHORT options with single dash, match long name
      // This allows -help but NOT --help for FLAG|SHORT options
      if (isSingleDash && opt.HasFeature(Option::FEAT_SHORT) &&
          Option::Normalize(opt.longName) == normInput) {
        return &opt;
      }

      // For normal options (without FEAT_SHORT), match long name with double
      // dash Or match long name regardless of dash type for non-FEAT_SHORT
      // options
      if (!opt.HasFeature(Option::FEAT_SHORT) &&
          Option::Normalize(opt.longName) == normInput) {
        return &opt;
      }
    }

    // If not found, check alias map (efficient O(log n) lookup)
    auto it = aliasMap.find(normInput);
    if (it != aliasMap.end()) {
      std::string normOptionName = Option::Normalize(it->second);

      // Check if this is a short alias by searching the alias table
      bool isShortAlias = false;
      bool isLongAlias = false;
      for (const auto &alias : aliasTable) {
        if (!alias.shortAliasName.empty() &&
            alias.shortAliasName == normInput) {
          isShortAlias = true;
          break;
        }
        // Check if this matches a long alias
        if (Option::Normalize(alias.aliasName) == normInput) {
          isLongAlias = true;
          break;
        }
      }

      // Short aliases require single dash format
      if (isShortAlias && !isSingleDash) {
        return nullptr;
      }

      // Long aliases require double dash format
      if (isLongAlias && !isDoubleDash) {
        return nullptr;
      }

      // Find the option by its name (from alias map value)
      for (const auto &opt : optionTable) {
        if (Option::Normalize(opt.longName) == normOptionName) {
          return &opt;
        }
      }
    }

    return nullptr;
  }

  const Option *FindOptionByShortName(const std::string &shortName) const {
    for (const auto &opt : optionTable) {
      if (opt.shortName == shortName)
        return &opt;
    }

    // Check alias map (efficient O(log n) lookup)
    auto it = aliasMap.find(shortName);
    if (it != aliasMap.end()) {
      std::string normOptionName = Option::Normalize(it->second);
      for (const auto &opt : optionTable) {
        if (Option::Normalize(opt.longName) == normOptionName ||
            opt.shortName == normOptionName) {
          return &opt;
        }
      }
    }

    return nullptr;
  }

  const Option *FindOptionByLongName(const std::string &longName) const {
    std::string normName = Option::Normalize(longName);
    for (const auto &opt : optionTable) {
      if (Option::Normalize(opt.longName) == normName)
        return &opt;
    }

    // Check alias map (efficient O(log n) lookup)
    auto it = aliasMap.find(normName);
    if (it != aliasMap.end()) {
      std::string normOptionName = Option::Normalize(it->second);
      for (const auto &opt : optionTable) {
        if (Option::Normalize(opt.longName) == normOptionName ||
            opt.shortName == normOptionName) {
          return &opt;
        }
      }
    }

    return nullptr;
  }

  /**
   * @brief Parse flag options (--flag or -flag for FEAT_SHORT)
   * @param arg The argument string
   * @return ParseResult indicating success, failure, or not this type
   */
  ParseResult ParseFlag(const std::string &arg) {
    const Option *opt = FindOption(arg);

    // For FEAT_SHORT options, also check single dash format
    if (!opt && arg.find("-") == 0 && arg.find("--") != 0) {
      opt = FindOption(arg);
    }

    if (opt && opt->HasFeature(Option::FEAT_FLAG)) {
      parsedArgs[opt->id].push_back("");
      return ParseResult::Success;
    }

    return ParseResult::NotAnOption;
  }

  /**
   * @brief Parse separate format options (--option value or -option value for
   * FEAT_SHORT)
   * @param arg The argument string
   * @param i Current index in argv (will be incremented if value is consumed)
   * @param argc Argument count
   * @param argv Argument vector
   * @return ParseResult indicating success, failure, or not this type
   */
  ParseResult ParseSeparate(const std::string &arg, int &i, int argc,
                            char *argv[]) {
    const Option *opt = FindOption(arg);

    if (opt && opt->HasFeature(Option::FEAT_SEPARATE)) {
      if (i + 1 >= argc) {
        // Normalize the argument name for consistent error messages
        std::string normalizedArg = arg;
        if (normalizedArg.find("--") == 0) {
          normalizedArg = "--" + Option::Normalize(normalizedArg.substr(2));
        } else if (normalizedArg.find("-") == 0 && normalizedArg.size() > 2) {
          // For short options with full name (e.g., -log-lvl), normalize it
          std::string shortPrefix = normalizedArg.substr(0, 1);
          std::string rest = normalizedArg.substr(1);
          // Check if this looks like a long name used with single dash
          if (rest.find('-') != std::string::npos || rest.length() > 2) {
            normalizedArg = shortPrefix + Option::Normalize(rest);
          }
        }
        *errStream << "Missing value for " << normalizedArg << std::endl;
        return ParseResult::Failed;
      }

      std::string value = argv[++i];
      if (!CheckOptionValue(opt, value)) {
        return ParseResult::Failed;
      }

      parsedArgs[opt->id].push_back(value);
      return ParseResult::Success;
    }

    return ParseResult::NotAnOption;
  }

  /**
   * @brief Parse equals join format options (--option=value or -option=value
   * for FEAT_SHORT)
   * @param arg The argument string
   * @return ParseResult indicating success, failure, or not this type
   */
  ParseResult ParseEqJoin(const std::string &arg) {
    size_t eqPos = arg.find('=');
    if (eqPos == std::string::npos) {
      return ParseResult::NotAnOption;
    }

    std::string namePart;
    std::string value = arg.substr(eqPos + 1);
    const Option *opt = nullptr;

    if (arg.find("--") == 0) {
      // Long format: --option=value
      namePart = arg.substr(2, eqPos - 2);
      opt = FindOptionByLongName(namePart);
      if (!opt) {
        // Try with full argument for FEAT_SHORT options
        opt = FindOption(arg.substr(0, eqPos));
      }
    } else if (arg.find("-") == 0) {
      // Short format: -option=value (for FEAT_SHORT options)
      namePart = arg.substr(1, eqPos - 1);
      opt = FindOption(arg.substr(0, eqPos));
    }

    if (opt && opt->HasFeature(Option::FEAT_EQ_JOIN)) {
      // Check if value is empty (e.g., --option=)
      if (value.empty()) {
        // Normalize the argument name for consistent error messages
        std::string normalizedArg = arg.substr(0, eqPos);
        if (normalizedArg.find("--") == 0) {
          normalizedArg = "--" + Option::Normalize(normalizedArg.substr(2));
        } else if (normalizedArg.find("-") == 0 && normalizedArg.size() > 1) {
          std::string shortPrefix = normalizedArg.substr(0, 1);
          std::string rest = normalizedArg.substr(1);
          if (rest.find('-') != std::string::npos || rest.length() > 2) {
            normalizedArg = shortPrefix + Option::Normalize(rest);
          }
        }
        *errStream << "Missing value for " << normalizedArg << std::endl;
        return ParseResult::Failed;
      }

      if (!CheckOptionValue(opt, value)) {
        return ParseResult::Failed;
      }

      parsedArgs[opt->id].push_back(value);
      return ParseResult::Success;
    }

    return ParseResult::NotAnOption;
  }

  /**
   * @brief Parse joined format options (--optionvalue or -lvalue for short
   * options)
   * @param arg The argument string
   * @return ParseResult indicating success, failure, or not this type
   */
  ParseResult ParseJoined(const std::string &arg) {
    // Try short option format first: -Ivalue, -Lcuda, etc.
    if (arg.size() > 2 && arg[0] == '-' && arg[1] != '-') {
      std::string shortName = arg.substr(1, 1);
      std::string value = arg.substr(2);

      const Option *opt = FindOptionByShortName(shortName);
      if (opt && opt->HasFeature(Option::FEAT_JOINED)) {
        if (!CheckOptionValue(opt, value)) {
          return ParseResult::Failed;
        }

        parsedArgs[opt->id].push_back(value);
        return ParseResult::Success;
      }
    }

    // Try long option format: --librarycuda, --helpme, etc.
    if (arg.size() > 3 && arg.find("--") == 0) {
      // Search for JOINED options that match the prefix
      for (const auto &option : optionTable) {
        if (option.HasFeature(Option::FEAT_JOINED)) {
          std::string expectedPrefix = "--" + option.longName;
          if (arg.find(expectedPrefix) == 0 &&
              arg.size() > expectedPrefix.size()) {
            std::string value = arg.substr(expectedPrefix.size());

            if (!CheckOptionValue(&option, value)) {
              return ParseResult::Failed;
            }

            parsedArgs[option.id].push_back(value);
            return ParseResult::Success;
          }

          // Also try with normalized name
          std::string normalizedPrefix =
              "--" + Option::Normalize(option.longName);
          if (arg.find(normalizedPrefix) == 0 &&
              arg.size() > normalizedPrefix.size()) {
            std::string value = arg.substr(normalizedPrefix.size());

            if (!CheckOptionValue(&option, value)) {
              return ParseResult::Failed;
            }

            parsedArgs[option.id].push_back(value);
            return ParseResult::Success;
          }

          // Check if argument matches exactly but without value (missing value
          // case)
          if (arg == expectedPrefix || arg == normalizedPrefix) {
            // Found a JOINED option without a value - report error
            *errStream << "Missing value for --"
                       << Option::Normalize(option.longName) << std::endl;
            return ParseResult::Failed;
          }
        }
      }

      // Try alias names for JOINED options
      if (!aliasTable.empty()) {
        for (const auto &alias : aliasTable) {
          std::string normOptionName = Option::Normalize(alias.optionName);
          const Option *targetOpt = nullptr;
          for (const auto &opt : optionTable) {
            if (Option::Normalize(opt.longName) == normOptionName ||
                opt.shortName == normOptionName) {
              targetOpt = &opt;
              break;
            }
          }

          if (targetOpt && targetOpt->HasFeature(Option::FEAT_JOINED)) {
            // Try long alias
            std::string expectedPrefix = "--" + alias.aliasName;
            if (arg.find(expectedPrefix) == 0 &&
                arg.size() > expectedPrefix.size()) {
              std::string value = arg.substr(expectedPrefix.size());

              if (!CheckOptionValue(targetOpt, value)) {
                return ParseResult::Failed;
              }

              parsedArgs[targetOpt->id].push_back(value);
              return ParseResult::Success;
            }

            // Try short alias
            if (!alias.shortAliasName.empty()) {
              std::string shortPrefix = "-" + alias.shortAliasName;
              if (arg.find(shortPrefix) == 0 &&
                  arg.size() > shortPrefix.size()) {
                std::string value = arg.substr(shortPrefix.size());

                if (!CheckOptionValue(targetOpt, value)) {
                  return ParseResult::Failed;
                }

                parsedArgs[targetOpt->id].push_back(value);
                return ParseResult::Success;
              }

              // Check if short alias is used without value
              if (arg == shortPrefix) {
                *errStream << "Missing value for -" << alias.shortAliasName
                           << std::endl;
                return ParseResult::Failed;
              }
            }

            // Check if long alias is used without value
            if (arg == expectedPrefix) {
              *errStream << "Missing value for --"
                         << Option::Normalize(alias.aliasName) << std::endl;
              return ParseResult::Failed;
            }
          }
        }
      }
    }

    // Check for short option without value: -L, -I, etc.
    if (arg.size() == 2 && arg[0] == '-' && arg[1] != '-') {
      std::string shortName = arg.substr(1, 1);
      const Option *opt = FindOptionByShortName(shortName);
      if (opt && opt->HasFeature(Option::FEAT_JOINED)) {
        *errStream << "Missing value for -" << shortName << std::endl;
        return ParseResult::Failed;
      }
    }

    return ParseResult::NotAnOption;
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
  AliasTable aliasTable; // Alias table for option name aliases (for
                         // help/introspection)
  AliasMap aliasMap;     // Alias lookup map for O(log n) runtime lookup
  std::map<int, std::vector<std::string>>
      parsedArgs;                  // Support multiple values per option
  std::vector<std::string> inputs; // Positional inputs (non-option arguments)
  std::vector<std::string> unknownArgs; // Unknown options
  bool allowUnknown;       // Whether to allow unknown options without error
  std::ostream *errStream; // Error message output stream (default: std::cerr)
};

/**
 * @brief X-Macros for unified argument definition.
 *
 * Usage format:
 * - For separate options: F(name, short_name, help_text, SEPARATE,
 * {allowed_values}) Supports space-separated (-o value) and equals-separated
 * (-o=value) formats
 * - For flags: F(name, short_name, help_text, FLAG, {}) - no value required
 * - For joined options: F(name, short_name, help_text, JOINED,
 * {allowed_values}) Value attached directly (-lcuda, --librarycuda)
 * - For both separate and joined: F(name, short_name, help_text, SEPARATE |
 * JOINED, {allowed_values}) Supports all formats: space, equals, and direct
 * attachment
 */

// Kind identifiers for macro usage (using bit flags)
// Using full namespace prefix to avoid conflicts
#define SEPARATE cppargs::Option::FEAT_SEPARATE
#define FLAG cppargs::Option::FEAT_FLAG
#define JOINED cppargs::Option::FEAT_JOINED
#define HIDDEN cppargs::Option::FEAT_HIDDEN
#define SHORT cppargs::Option::FEAT_SHORT
#define EQ_JOIN cppargs::Option::FEAT_EQ_JOIN

// GENERATE_ENUM takes kind and optional allowed values (ignored for enum
// generation)
#define GENERATE_ENUM(name, sh, help, kind, ...) OPT_##name,

// GENERATE_TABLE uses kind and allowed values
// Parameters for DEFINE_ARGS: name, sh, help, kind, allowed_values...
// This is used by DEFINE_ARGS for non-grouped options (default groupId = -1)
#define MAKE_ALLOWED(...) __VA_ARGS__

#define GENERATE_TABLE(name, sh, help, kind, ...)                              \
  cppargs::Option{(int)OPT_##name,           #name, #sh, help,                 \
                  MAKE_ALLOWED(__VA_ARGS__), kind,  -1},

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
  const cppargs::Option InitList_##TableName[] = {ArgsMacro(GENERATE_TABLE)};  \
  static const cppargs::OptionTable TableName(                                 \
      InitList_##TableName,                                                    \
      InitList_##TableName +                                                   \
          sizeof(InitList_##TableName) / sizeof(InitList_##TableName[0]));

/**
 * @brief Macro for defining alias table
 * @param AliasTableName Name of the alias table to generate (e.g., AliasTable)
 * @param AliasMacro Macro that defines all aliases with format:
 *                   A(alias_name, short_alias, option_name)
 *
 * Usage example:
 *    #define ALIAS_OPTION(A)                                                  \
 *       A(save_temps, st, keep)                                               \
 *       A(verbose, v, print_log)                                              \
 *       A(helpme, , help)  // Empty short alias becomes empty string          \
 *    DEFINE_ALIAS(AliasTable, ALIAS_OPTION)
 *
 * Then use: parser.SetAliasTable(AliasTable);
 */

// Generate enum for alias options
#define GENERATE_ALIAS_ENUM(alias, short_alias, option) ALIAS_##alias,

// Generate alias table entries - directly stringify all parameters
// Empty parameters will become empty strings automatically
#define GENERATE_ALIAS_ENTRY(alias, short_alias, option)                       \
  cppargs::AliasEntry{#alias, #short_alias, #option},

#define DEFINE_ALIAS(AliasTableName, AliasMacro)                               \
  enum { AliasMacro(GENERATE_ALIAS_ENUM) ALIAS_COUNT };                        \
  const cppargs::AliasEntry InitList_##AliasTableName[] = {                    \
      AliasMacro(GENERATE_ALIAS_ENTRY)};                                       \
  static const cppargs::AliasTable AliasTableName(                             \
      InitList_##AliasTableName,                                               \
      InitList_##AliasTableName + sizeof(InitList_##AliasTableName) /          \
                                      sizeof(InitList_##AliasTableName[0]));

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
  cppargs::Option{(int)OPT_##name, #name, #sh,        help,                    \
                  __VA_ARGS__,     kind,  GRP_##group},

#define GENERATE_GROUP_ENUM(name, display) GRP_##name,
#define GENERATE_GROUP_INFO(name, display) {GRP_##name, display},

#define DEFINE_ARGS_WITH_GROUP(ArgEnumName, GroupEnumName, TableName,          \
                               ArgsMacro, GroupsMacro)                         \
  enum GroupEnumName {                                                         \
    GroupsMacro(GENERATE_GROUP_ENUM) GroupEnumName##_COUNT                     \
  };                                                                           \
  enum ArgEnumName {                                                           \
    ArgsMacro(GENERATE_ENUM_WITH_GROUP) ArgEnumName##_COUNT                    \
  };                                                                           \
  const cppargs::Option InitList_##TableName[] = {                             \
      ArgsMacro(GENERATE_TABLE_WITH_GROUP)};                                   \
  static const cppargs::OptionTable TableName(                                 \
      InitList_##TableName,                                                    \
      InitList_##TableName +                                                   \
          sizeof(InitList_##TableName) / sizeof(InitList_##TableName[0]));     \
  const cppargs::OptionGroup GroupList_##TableName[] = {                       \
      GroupsMacro(GENERATE_GROUP_INFO)};                                       \
  static const cppargs::OptionGroupTable TableName##Groups(                    \
      GroupList_##TableName,                                                   \
      GroupList_##TableName +                                                  \
          sizeof(GroupList_##TableName) / sizeof(GroupList_##TableName[0]));

} // namespace cppargs

#endif
