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
    FEAT_EQ_JOIN = 32,   // Bit 5: equals join format support (--option=value)
    FEAT_COMMA_LIST = 64 // Bit 6: comma-separated list support (--option=a,b,c)
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
      : optionTable(table), allowUnknown(false), errStream(&std::cerr) {
    BuildOptionLookup();
  }

  /**
   * @brief Constructor with option groups support
   * @param table The option table
   * @param groups The option group table
   */
  ArgumentParser(const OptionTable &table, const OptionGroupTable &groups)
      : optionTable(table), optionGroups(groups), allowUnknown(false),
        errStream(&std::cerr) {
    BuildOptionLookup();
  }

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

    BuildAliasLookup();
  }

  /**
   * @brief reset parsed result.
   *
   * Parse() appends results to the current state, so call Reset() before
   * parsing again with the same parser.
   */
  void Reset() {
    parsedArgs.clear();
    inputs.clear();
    unknownArgs.clear();
  }

  /**
   * @brief Fuzzy parsing: treats "_" and "-" as identical.
   * Supports joined options where value is attached to the option name
   * Remaining arguments after known options are treated as inputs
   *
   * Note: this method appends to the parser's state; call Reset() before
   * parsing again with the same parser.
   */
  bool Parse(int argc, const char **argv) {
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];

      // Everything after "--" is a positional input, not an option
      if (arg == "--") {
        for (int j = i + 1; j < argc; ++j) {
          inputs.push_back(argv[j]);
        }
        break;
      }

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

    bool inputHasEq = (eqPos != std::string::npos);

    for (const auto &opt : optionTable) {
      int distance;
      std::string candidate;

      std::string normLongDoubleDash = "--" + Option::Normalize(opt.longName);
      std::string normLongSingleDash = "-" + Option::Normalize(opt.longName);

      if (opt.HasFeature(Option::FEAT_SHORT)) {
        // For FEAT_SHORT, consider both single-dash and double-dash forms
        distance = LevenshteinDistance(normInput, normLongSingleDash);
        candidate = normLongSingleDash;
        int doubleDashDist = LevenshteinDistance(normInput, normLongDoubleDash);
        if (doubleDashDist < distance) {
          distance = doubleDashDist;
          candidate = normLongDoubleDash;
        }
      } else {
        distance = LevenshteinDistance(normInput, normLongDoubleDash);
        candidate = normLongDoubleDash;
      }

      // Also consider short name if it exists and might be a better match
      if (!opt.shortName.empty()) {
        int shortDistance = LevenshteinDistance(normInput, "-" + opt.shortName);
        if (shortDistance < distance) {
          distance = shortDistance;
          candidate = "-" + opt.shortName;
        }
      }

      if (inputHasEq && opt.HasFeature(Option::FEAT_EQ_JOIN)) {
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

  /**
   * @brief Print the help text for all options to a stream
   * @param os Output stream (default is std::cout)
   * @param wrappedWidth Maximum line width for wrapping
   *
   * The stream can be redirected (e.g. std::ostringstream for testing or GUI
   * display).
   */
  void PrintHelp(std::ostream &os = std::cout, int wrappedWidth = 100) const {
    // Check if we have groups defined
    if (optionGroups.empty()) {
      // Print all options without grouping
      for (const auto &opt : optionTable) {
        printOption(&opt, os, wrappedWidth);
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
        printOption(opt, os, wrappedWidth);
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
          os << std::endl;
        }
        firstGroup = false;

        // Print group header if we have a group name
        if (!optionGroups[groupId].name.empty()) {
          os << optionGroups[groupId].name << ":" << std::endl;
          os << std::string(wrappedWidth, '=') << std::endl;
        }

        for (const auto *opt : options) {
          printOption(opt, os, wrappedWidth);
        }
      }
    }
  }

private:
  void printOption(const Option *opt, std::ostream &os,
                         int wrappedWidth = 100) const {
    if (opt->HasFeature(Option::FEAT_HIDDEN)) {
      return;
    }

    int labelWidth = std::max(0, wrappedWidth - 30);
    size_t maxBriefLen = static_cast<size_t>(std::max(2, wrappedWidth) / 2);
    int wrapWidth = std::max(4, wrappedWidth) - 4;

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
        if (brief.size() >= maxBriefLen) {
          brief += "|...";
          break;
        }
        brief += (it == opt->allowed.begin() ? "" : "|");
        brief += *it;
      }
      brief += "]";
    }

    os << std::left << std::setw(labelWidth) << brief;

    if (!opt->shortName.empty()) {
      os << "(-" << Option::Normalize(opt->shortName) << ")";
    }
    os << std::endl;

    std::istringstream iss(opt->help);
    std::string word;
    std::string line;

    while (iss >> word) {
      if (!line.empty() && line.length() + 1 + word.length() >
                               static_cast<size_t>(wrapWidth)) {
        os << "    " << line << std::endl;
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
      os << "    " << line << std::endl;
    }

    // print alias
    for (const auto &it : aliasTable) {
      if (it.optionName == opt->longName) {
        if (it.aliasName.empty()) {
          // only short alias name
          os << std::left << std::setw(labelWidth)
                    << ("-" + it.shortAliasName);
        } else {
          os << std::left << std::setw(labelWidth)
                    << ("--" + it.aliasName);
          if (!it.shortAliasName.empty()) {
            os << "(-" << it.shortAliasName << ")";
          }
        }
        os << std::endl;
        os << "    Alias for " << "--"
                  << Option::Normalize(opt->longName) << std::endl;
      }
    }
  }

  /**
   * @brief Split a comma-separated string into individual values
   * @param value The comma-separated string to split
   * @return Vector of individual values
   *
   * Whitespace around each field is trimmed. Empty fields (including
   * leading/trailing commas, e.g. "a," or ",b") are preserved as empty
   * strings so that validation can reject them.
   */
  static std::vector<std::string> SplitCommaList(const std::string &value) {
    std::vector<std::string> result;
    size_t pos = 0;
    while (pos <= value.size()) {
      size_t comma = value.find(',', pos);
      std::string item = (comma == std::string::npos)
                             ? value.substr(pos)
                             : value.substr(pos, comma - pos);

      size_t start = item.find_first_not_of(" \t");
      size_t end = item.find_last_not_of(" \t");
      if (start == std::string::npos) {
        result.push_back("");
      } else {
        result.push_back(item.substr(start, end - start + 1));
      }

      if (comma == std::string::npos) {
        break;
      }
      pos = comma + 1;
    }
    return result;
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
   * For COMMA_LIST options, splits by comma and validates each value
   * individually.
   */
  bool CheckOptionValue(const Option *opt, const std::string &value) const {
    if (!opt)
      return false;

    // For COMMA_LIST options, split and validate each value individually.
    // Empty fields (e.g. "a," or ",b") are rejected. Validation also runs when
    // no allowed values are specified, so empty fields are always caught.
    if (opt->HasFeature(Option::FEAT_COMMA_LIST)) {
      std::vector<std::string> values = SplitCommaList(value);
      for (const auto &singleValue : values) {
        if (singleValue.empty()) {
          ReportInvalidValue(opt, singleValue);
          return false;
        }
        if (!opt->allowed.empty() &&
            opt->allowed.find(singleValue) == opt->allowed.end()) {
          ReportInvalidValue(opt, singleValue);
          return false;
        }
      }
      return true;
    }

    // If no allowed values specified, the value is always valid
    if (opt->allowed.empty())
      return true;

    // For non-COMMA_LIST options, check the value directly
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

    *errStream << "Invalid value '" << value << "' for " << optionName;
    if (!opt->allowed.empty()) {
      *errStream << ". Allowed values: ";
      bool first = true;
      for (const auto &allowed : opt->allowed) {
        *errStream << (first ? "" : "|") << allowed;
        first = false;
      }
    }
    *errStream << std::endl;
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

    if (isSingleDash || isDoubleDash) {
      // O(log n) lookup: keys are pre-normalized and dash-prefixed
      std::string key = (isDoubleDash ? "--" : "-") + normInput;
      const Option *opt = LookupIn(optionLookupMap, key);
      if (opt)
        return opt;
      return LookupIn(aliasLookupMap, key);
    }

    // No-prefix input matches the bare long name of non-SHORT options
    return LookupIn(optionLookupMap, normInput);
  }

  const Option *FindOptionByAlias(const std::string &key) const {
    auto it = aliasMap.find(key);
    if (it == aliasMap.end())
      return nullptr;

    std::string normOptionName = Option::Normalize(it->second);
    for (const auto &opt : optionTable) {
      if (Option::Normalize(opt.longName) == normOptionName ||
          opt.shortName == normOptionName) {
        return &opt;
      }
    }
    return nullptr;
  }

  const Option *FindOptionByShortName(const std::string &shortName) const {
    std::string key = "-" + Option::Normalize(shortName);
    const Option *opt = LookupIn(optionLookupMap, key);
    if (opt)
      return opt;
    opt = LookupIn(aliasLookupMap, key);
    if (opt)
      return opt;
    return FindOptionByAlias(shortName);
  }

  const Option *FindOptionByLongName(const std::string &longName) const {
    std::string key = "--" + Option::Normalize(longName);
    const Option *opt = LookupIn(optionLookupMap, key);
    if (opt)
      return opt;
    opt = LookupIn(aliasLookupMap, key);
    if (opt)
      return opt;
    return FindOptionByAlias(Option::Normalize(longName));
  }

  /**
   * @brief Build the normalized-name lookup map from the option table.
   *
   * Keys are pre-normalized (underscore -> dash) and dash-prefixed so that
   * FindOption() and friends are O(log n) instead of scanning the whole
   * option table on every argument. Maps store indices into optionTable,
   * which keeps them valid across parser copy/assignment.
   */
  void BuildOptionLookup() {
    optionLookupMap.clear();
    for (size_t i = 0; i < optionTable.size(); ++i) {
      const Option &opt = optionTable[i];
      std::string normLong = Option::Normalize(opt.longName);

      // Double dash always matches the long name
      optionLookupMap["--" + normLong] = static_cast<int>(i);

      if (opt.HasFeature(Option::FEAT_SHORT)) {
        // FEAT_SHORT options also accept the long name with a single dash
        optionLookupMap["-" + normLong] = static_cast<int>(i);
      } else {
        // No-prefix form for backward compatibility
        optionLookupMap[normLong] = static_cast<int>(i);
      }

      if (!opt.shortName.empty()) {
        // Short names only match with a single dash
        optionLookupMap["-" + Option::Normalize(opt.shortName)] =
            static_cast<int>(i);
      }
    }
  }

  /**
   * @brief Build the alias lookup map from the alias table.
   *
   * Long aliases are stored under a double-dash key, short aliases under a
   * single-dash key. This preserves the original dash-format restrictions
   * (short aliases require single dash, long aliases require double dash)
   * without scanning the alias table at parse time.
   */
  void BuildAliasLookup() {
    aliasLookupMap.clear();
    for (const auto &alias : aliasTable) {
      int target = FindOptionIndexByName(alias.optionName);
      if (target < 0)
        continue;

      if (!alias.aliasName.empty()) {
        aliasLookupMap["--" + Option::Normalize(alias.aliasName)] = target;
      }
      if (!alias.shortAliasName.empty()) {
        aliasLookupMap["-" + alias.shortAliasName] = target;
      }
    }
  }

  /**
   * @brief Resolve an option by its long or short name (used for alias targets)
   * @param name The raw option name (e.g. "save_temps")
   * @return Index into optionTable, or -1 if not found
   */
  int FindOptionIndexByName(const std::string &name) const {
    std::string norm = Option::Normalize(name);
    for (size_t i = 0; i < optionTable.size(); ++i) {
      if (Option::Normalize(optionTable[i].longName) == norm ||
          optionTable[i].shortName == norm) {
        return static_cast<int>(i);
      }
    }
    return -1;
  }

  const Option *FindOptionByName(const std::string &name) const {
    int idx = FindOptionIndexByName(name);
    return (idx >= 0) ? &optionTable[idx] : nullptr;
  }

  /**
   * @brief Look up a normalized key in a name->index map and resolve it to an
   * option pointer (or nullptr).
   */
  const Option *LookupIn(const std::map<std::string, int> &map,
                         const std::string &key) const {
    auto it = map.find(key);
    if (it == map.end())
      return nullptr;
    if (it->second < 0 || it->second >= static_cast<int>(optionTable.size()))
      return nullptr;
    return &optionTable[it->second];
  }

  /**
   * @brief Check if a string looks like a negative number (e.g. "-5", "-1.5")
   */
  static bool IsNegativeNumber(const std::string &s) {
    if (s.size() < 2 || s[0] != '-')
      return false;
    char c = s[1];
    if (c >= '0' && c <= '9')
      return true;
    if (c == '.' && s.size() > 2) {
      char c2 = s[2];
      return c2 >= '0' && c2 <= '9';
    }
    return false;
  }

  /**
   * @brief Check if a string looks like an option (starts with '-', but is not
   * a lone "-" or a negative number).
   */
  static bool LooksLikeOption(const std::string &s) {
    if (s.size() < 2 || s[0] != '-')
      return false;
    return !IsNegativeNumber(s);
  }

  /**
   * @brief Normalize an argument name for error messages.
   * Converts underscores to dashes while preserving the prefix.
   */
  static std::string NormalizeArgName(const std::string &arg) {
    std::string normalized = arg;
    if (normalized.find("--") == 0) {
      normalized = "--" + Option::Normalize(normalized.substr(2));
    } else if (normalized.find("-") == 0 && normalized.size() > 1) {
      std::string shortPrefix = normalized.substr(0, 1);
      std::string rest = normalized.substr(1);
      // Check if this looks like a long name used with single dash
      if (rest.find('-') != std::string::npos || rest.length() > 2) {
        normalized = shortPrefix + Option::Normalize(rest);
      }
    }
    return normalized;
  }

  /**
   * @brief Store a validated value for an option.
   *
   * For COMMA_LIST options the value is split and each part is stored
   * separately; otherwise the value is stored as-is. The value is assumed to
   * have already passed CheckOptionValue().
   */
  void StoreValue(const Option *opt, const std::string &value) {
    if (opt->HasFeature(Option::FEAT_COMMA_LIST)) {
      std::vector<std::string> values = SplitCommaList(value);
      for (const auto &singleValue : values) {
        parsedArgs[opt->id].push_back(singleValue);
      }
    } else {
      parsedArgs[opt->id].push_back(value);
    }
  }

  /**
   * @brief Parse flag options (--flag or -flag for FEAT_SHORT)
   * @param arg The argument string
   * @return ParseResult indicating success, failure, or not this type
   */
  ParseResult ParseFlag(const std::string &arg) {
    const Option *opt = FindOption(arg);

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
                            const char **argv) {
    const Option *opt = FindOption(arg);

    if (opt && opt->HasFeature(Option::FEAT_SEPARATE)) {
      // Reject missing values, and do not swallow the next argument as a value
      // when it looks like another option (e.g. "--port --verbose"). A lone
      // "-" (stdin convention) or a negative number is a valid value.
      if (i + 1 >= argc || LooksLikeOption(argv[i + 1])) {
        *errStream << "Missing value for " << NormalizeArgName(arg) << std::endl;
        return ParseResult::Failed;
      }

      std::string value = argv[++i];
      if (!CheckOptionValue(opt, value)) {
        return ParseResult::Failed;
      }

      StoreValue(opt, value);

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
        *errStream << "Missing value for " << NormalizeArgName(arg.substr(0, eqPos))
                   << std::endl;
        return ParseResult::Failed;
      }

      if (!CheckOptionValue(opt, value)) {
        return ParseResult::Failed;
      }

      StoreValue(opt, value);

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

        StoreValue(opt, value);
        return ParseResult::Success;
      }
    }

    // Try long option format: --librarycuda, --helpme, etc.
    // When several JOINED options could match, the longest matching prefix
    // wins (e.g. --log_lvlhigh matches log_lvl, not log). An exact match of a
    // JOINED option name without a value is reported as a missing value.
    if (arg.size() > 3 && arg.find("--") == 0) {
      const Option *bestOpt = nullptr;
      std::string bestValue;
      size_t bestLen = 0;

      for (const auto &option : optionTable) {
        if (!option.HasFeature(Option::FEAT_JOINED)) {
          continue;
        }

        std::string prefix = "--" + option.longName;
        if (arg == prefix) {
          *errStream << "Missing value for --"
                     << Option::Normalize(option.longName) << std::endl;
          return ParseResult::Failed;
        }
        if (arg.size() > prefix.size() &&
            arg.compare(0, prefix.size(), prefix) == 0 &&
            prefix.size() > bestLen) {
          bestLen = prefix.size();
          bestOpt = &option;
          bestValue = arg.substr(prefix.size());
        }

        std::string normalizedPrefix = "--" + Option::Normalize(option.longName);
        if (normalizedPrefix != prefix) {
          if (arg == normalizedPrefix) {
            *errStream << "Missing value for --"
                       << Option::Normalize(option.longName) << std::endl;
            return ParseResult::Failed;
          }
          if (arg.size() > normalizedPrefix.size() &&
              arg.compare(0, normalizedPrefix.size(), normalizedPrefix) == 0 &&
              normalizedPrefix.size() > bestLen) {
            bestLen = normalizedPrefix.size();
            bestOpt = &option;
            bestValue = arg.substr(normalizedPrefix.size());
          }
        }
      }

      if (bestOpt) {
        if (!CheckOptionValue(bestOpt, bestValue)) {
          return ParseResult::Failed;
        }

        StoreValue(bestOpt, bestValue);
        return ParseResult::Success;
      }

      // Try alias names for JOINED options (longest prefix wins)
      if (!aliasTable.empty()) {
        const Option *bestAliasOpt = nullptr;
        std::string bestAliasValue;
        size_t bestAliasLen = 0;

        for (const auto &alias : aliasTable) {
          const Option *targetOpt = FindOptionByName(alias.optionName);
          if (!targetOpt || !targetOpt->HasFeature(Option::FEAT_JOINED)) {
            continue;
          }

          if (!alias.aliasName.empty()) {
            std::string longPrefix = "--" + alias.aliasName;
            if (arg == longPrefix) {
              *errStream << "Missing value for --"
                         << Option::Normalize(alias.aliasName) << std::endl;
              return ParseResult::Failed;
            }
            if (arg.size() > longPrefix.size() &&
                arg.compare(0, longPrefix.size(), longPrefix) == 0 &&
                longPrefix.size() > bestAliasLen) {
              bestAliasLen = longPrefix.size();
              bestAliasOpt = targetOpt;
              bestAliasValue = arg.substr(longPrefix.size());
            }
          }

          if (!alias.shortAliasName.empty()) {
            std::string shortPrefix = "-" + alias.shortAliasName;
            if (arg == shortPrefix) {
              *errStream << "Missing value for -" << alias.shortAliasName
                         << std::endl;
              return ParseResult::Failed;
            }
            if (arg.size() > shortPrefix.size() &&
                arg.compare(0, shortPrefix.size(), shortPrefix) == 0 &&
                shortPrefix.size() > bestAliasLen) {
              bestAliasLen = shortPrefix.size();
              bestAliasOpt = targetOpt;
              bestAliasValue = arg.substr(shortPrefix.size());
            }
          }
        }

        if (bestAliasOpt) {
          if (!CheckOptionValue(bestAliasOpt, bestAliasValue)) {
            return ParseResult::Failed;
          }

          StoreValue(bestAliasOpt, bestAliasValue);
          return ParseResult::Success;
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
  std::map<std::string, int>
      optionLookupMap; // Pre-normalized name -> optionTable index
  std::map<std::string, int>
      aliasLookupMap; // Pre-normalized alias name -> optionTable index
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
 * Recommended usage: define all options in a dedicated header (e.g.
 * options.h), then include cpp_args_macro_cleaner.inc at the end of that
 * header. The cleaner #undefs the library macros, so the short names below
 * (SEPARATE, FLAG, ...) never leak into the rest of the program. Keeping the
 * option definitions in their own header is what makes these short, readable
 * macro names safe.
 *
 * Usage format:
 * - For separate options: F(name, short_name, help_text, SEPARATE,
 * {allowed_values}) Supports space-separated format (-o value). Add
 * EQ_JOIN to also accept equals-separated format (-o=value).
 * - For flags: F(name, short_name, help_text, FLAG, {}) - no value
 * required
 * - For joined options: F(name, short_name, help_text, JOINED,
 * {allowed_values}) Value attached directly (-lcuda, --librarycuda)
 * - For both separate and joined: F(name, short_name, help_text,
 * SEPARATE | JOINED, {allowed_values}) Supports space and
 * direct attachment formats; add EQ_JOIN to also support equals format
 * - For comma-separated lists: F(name, short_name, help_text, SEPARATE
 * | COMMA_LIST | EQ_JOIN, {allowed_values}) Accepts
 * comma-separated values (--option=a,b,c) which are split and stored as
 * multiple values. Each value is validated against the allowed set
 * individually. Empty fields (e.g. "--option=a,") are rejected. Note: the "="
 * syntax requires EQ_JOIN; without it only the space-separated form is
 * accepted.
 *
 * Limitations:
 * - Combined short flags (e.g. -vd) are not supported; each short flag must be
 *   given separately (-v -d).
 * - A token that starts with '-' is always treated as an option, so a negative
 *   number as a positional input (e.g. "prog -5") is rejected. Use "--" to
 *   pass such tokens as inputs (e.g. "prog -- -5"). Negative numbers remain
 *   valid as option values (e.g. "--port -1").
 * - Parse() appends results to the current state; call Reset() before parsing
 *   again with the same parser.
 */

// Kind identifiers for macro usage (using bit flags)
// Using full namespace prefix to avoid conflicts
#define SEPARATE cppargs::Option::FEAT_SEPARATE
#define FLAG cppargs::Option::FEAT_FLAG
#define JOINED cppargs::Option::FEAT_JOINED
#define HIDDEN cppargs::Option::FEAT_HIDDEN
#define SHORT cppargs::Option::FEAT_SHORT
#define EQ_JOIN cppargs::Option::FEAT_EQ_JOIN
#define COMMA_LIST cppargs::Option::FEAT_COMMA_LIST

// GENERATE_ENUM takes kind and optional allowed values (ignored for
// enum generation)
#define GENERATE_ENUM(name, sh, help, kind, ...) OPT_##name,

// GENERATE_TABLE uses kind and allowed values
// Parameters for DEFINE_ARGS: name, sh, help, kind, allowed_values...
// This is used by DEFINE_ARGS for non-grouped options (default
// groupId = -1)
#define MAKE_ALLOWED(...) __VA_ARGS__

#define GENERATE_TABLE(name, sh, help, kind, ...)                       \
  cppargs::Option{(int)OPT_##name,           #name, #sh, help,                  \
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
 *       F(port, p, "Server port", SEPARATE, {})                        \
 *       F(verbose, v, "Enable verbose", FLAG, {})                      \
 *       F(log_lvl, l, "Log level", SEPARATE, {"debug", "info"})        \
 *       F(help, h, "Print help", FLAG, {})                             \
 *    DEFINE_ARGS(App, AppTable, ARGS)
 *
 */
#define DEFINE_ARGS(EnumName, TableName, ArgsMacro)                     \
  enum EnumName { ArgsMacro(GENERATE_ENUM) EnumName##_COUNT };          \
  const cppargs::Option InitList_##TableName[] = {                              \
      ArgsMacro(GENERATE_TABLE)};                                       \
  static const cppargs::OptionTable TableName(                                  \
      InitList_##TableName,                                                     \
      InitList_##TableName +                                                    \
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
#define GENERATE_ALIAS_ENTRY(alias, short_alias, option)                \
  cppargs::AliasEntry{#alias, #short_alias, #option},

#define DEFINE_ALIAS(AliasTableName, AliasMacro)                        \
  enum { AliasMacro(GENERATE_ALIAS_ENUM) ALIAS_COUNT };                 \
  const cppargs::AliasEntry InitList_##AliasTableName[] = {                     \
      AliasMacro(GENERATE_ALIAS_ENTRY)};                                \
  static const cppargs::AliasTable AliasTableName(                              \
      InitList_##AliasTableName,                                                \
      InitList_##AliasTableName + sizeof(InitList_##AliasTableName) /           \
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
 *       F(Frontend, host, H, "Host", SEPARATE, {})                     \
 *       F(Frontend, port, p, "Port", SEPARATE, {})                     \
 *       F(Backend, db, d, "Database", SEPARATE, {})                    \
 *       F(Default, verbose, v, "Verbose", FLAG, {})                    \
 *
 *    DEFINE_ARGS_WITH_GROUP(App, AppGroup, AppTable, ARGS, GROUPS)
 */
#define GENERATE_ENUM_WITH_GROUP(group, name, sh, help, kind, ...)      \
  OPT_##name,

#define GENERATE_TABLE_WITH_GROUP(group, name, sh, help, kind, ...)     \
  cppargs::Option{(int)OPT_##name, #name, #sh,        help,                     \
                  __VA_ARGS__,     kind,  GRP_##group},

#define GENERATE_GROUP_ENUM(name, display) GRP_##name,
#define GENERATE_GROUP_INFO(name, display) {GRP_##name, display},

#define DEFINE_ARGS_WITH_GROUP(ArgEnumName, GroupEnumName, TableName,   \
                                       ArgsMacro, GroupsMacro)                  \
  enum GroupEnumName {                                                          \
    GroupsMacro(GENERATE_GROUP_ENUM) GroupEnumName##_COUNT              \
  };                                                                            \
  enum ArgEnumName {                                                            \
    ArgsMacro(GENERATE_ENUM_WITH_GROUP) ArgEnumName##_COUNT             \
  };                                                                            \
  const cppargs::Option InitList_##TableName[] = {                              \
      ArgsMacro(GENERATE_TABLE_WITH_GROUP)};                            \
  static const cppargs::OptionTable TableName(                                  \
      InitList_##TableName,                                                     \
      InitList_##TableName +                                                    \
          sizeof(InitList_##TableName) / sizeof(InitList_##TableName[0]));      \
  const cppargs::OptionGroup GroupList_##TableName[] = {                        \
      GroupsMacro(GENERATE_GROUP_INFO)};                                \
  static const cppargs::OptionGroupTable TableName##Groups(                     \
      GroupList_##TableName,                                                    \
      GroupList_##TableName +                                                   \
          sizeof(GroupList_##TableName) / sizeof(GroupList_##TableName[0]));

} // namespace cppargs

#endif
