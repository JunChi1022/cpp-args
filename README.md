# C++ Argument Parser

A lightweight, header-only command-line argument parser for C++

## Features

- **Header-only library** - Just include `cpp_args.hpp` and you're ready to go
- **Flexible naming** - Supports both `--log_lvl` and `--log-lvl` formats (underscores automatically converted to dashes)
- **Short & long options** - Support for both `-p` and `--port` style arguments
- **Flag support** - Boolean flags that don't require values (e.g., `--help`, `-v`)
- **Value validation** - Define allowed values for specific arguments
- **Joined options** - Support attached values like `-lcuda` or `--librarycuda` (short and long options)
- **Positional inputs** - Non-option arguments are preserved as positional inputs
- **Unknown option handling** - Configurable handling of unknown options with retrieval API
- **X-Macros** - Clean, maintainable option definitions using X-Macro pattern
- **Unified syntax** - Define options and flags together with a single macro
- **Option Groups** - Organize options into logical groups with grouped help display
- **Option Aliases** - Support multiple names for the same option (e.g., `--helpme` → `--help`)

## Quick Start

### Complete Example

```cpp
#include "cpp_args.hpp"
#include <iostream>

using namespace cppargs;

// Define all arguments in one place - mix options and flags freely
#define MY_APP_ARGS(F)                                                         \
  F(host, H, "Server hostname", SEPARATE, {})                                  \
  F(port, p, "Server port", SEPARATE, {"8080", "9090"})                        \
  F(library, L, "Link library", JOINED, {"cuda", "stdc++"})                    \
  F(verbose, v, "Enable verbose mode", FLAG, {})                               \
  F(help, h, "Print help message", FLAG, {})

DEFINE_ARGS(MyApp, MyAppTable, MY_APP_ARGS)

int main(int argc, char* argv[]) {
  ArgumentParser parser(MyAppTable);
  
  if (!parser.Parse(argc, argv)) {
    std::cerr << "Failed to parse arguments" << std::endl;
    parser.PrintHelp();
    return 1;
  }
  
  if (parser.HasArg(OPT_help)) {
    parser.PrintHelp();
    return 0;
  }
  
  if (parser.HasArg(OPT_verbose)) {
    std::cout << "Verbose mode enabled" << std::endl;
  }
  
  if (parser.HasArg(OPT_host)) {
    std::cout << "Host: " << parser.GetArgValue(OPT_host) << std::endl;
  }
  
  if (parser.HasArg(OPT_port)) {
    std::cout << "Port: " << parser.GetArgValue(OPT_port) << std::endl;
  }
  
  if (parser.HasArg(OPT_library)) {
    std::cout << "Library: " << parser.GetArgValue(OPT_library) << std::endl;
  }
  
  // Access positional inputs
  const auto& inputs = parser.GetInputs();
  for (const auto& input : inputs) {
    std::cout << "Input file: " << input << std::endl;
  }
  
  return 0;
}
```

### Usage Examples

```
# Regular options with values (space-separated)
./my_app --host localhost --port 8080
./my_app -H localhost -p 9090

# Joined options (value attached directly)
./my_app -Lcuda                    # Short option with joined value
./my_app --librarycuda             # Long option with joined value
./my_app -Lstdc++ -I/usr/include   # Multiple joined options

# Flags (no value needed)
./my_app --verbose
./my_app -v
./my_app --help

# Positional inputs (non-option arguments)
./my_app -o out.txt input1.txt input2.txt
./my_app file1.txt -v file2.txt file3.txt

# Mixed usage
./my_app -v --host localhost -Lcuda --port 8080 input.txt
./my_app --verbose -H 127.0.0.1 -Lpthread -p 9090 *.txt

# Underscore/dash flexibility (both work!)
./my_app --log-lvl debug    # with dash
./my_app --log_lvl debug    # with underscore
```

## Syntax Guide

### Defining Arguments

**Separate Options** (require separate value or '=' separator):
```cpp
F(name, short_name, "help text", SEPARATE, {allowed_values})
```

**Flags** (no value required):
```cpp
F(name, short_name, "help text", FLAG, {})
```

**Joined Options** (value attached directly to option name):
```cpp
F(name, short_name, "help text", JOINED, {allowed_values})
```

**Combined Formats** (support both separate and joined):
```cpp
F(name, short_name, "help text", SEPARATE | JOINED, {allowed_values})
```

Parameters:
- `name`: Option identifier (use underscores, e.g., `log_lvl`)
- `short_name`: Single character short form (e.g., `p` for `-p`)
- `help_text`: Description shown in help message
- `SEPARATE`, `FLAG`, `JOINED`, or combination (using `|`): Kind identifier using bit flags
  - `SEPARATE`: Space-separated (`-o value`) or equals-separated (`-o=value`)
  - `JOINED`: Direct attachment (`-lcuda`, `--librarycuda`)
  - `SEPARATE | JOINED`: Both formats supported
  - `FLAG`: Boolean flag, no value
- `allowed_values`: For SEPARATE/JOINED - list of valid values like `{"json", "xml"}`, use `{}` for any value

### Separate Options (Standard Command-line Flags)

Most common option type for standard command-line interfaces. Supports both space-separated and equals-separated formats:

```cpp
#define MY_ARGS(F) \
  F(output, o, "Output file", SEPARATE, {}) \
  F(verbose, v, "Verbose mode", FLAG, {}) \
  F(config, c, "Config file", SEPARATE, {"dev", "test", "prod"})

DEFINE_ARGS(App, AppTable, MY_ARGS)
```

Usage examples:
```bash
# Space-separated format
./app -o out.txt
./app --output out.txt  
./app -c dev

# Equals-separated format
./app -o=out.txt
./app --output=out.txt
./app --config=dev

# Mixed formats are fully supported
./app -o output.txt --config=prod -v
```

**Supported formats for SEPARATE kind**:
- Short option with space: `-o value`
- Long option with space: `--output value`
- Short option with equals: `-o=value`
- Long option with equals: `--output=value`
- All formats are interchangeable and can be mixed

### Separate and Joined Options (Flexible Format Support)

For maximum flexibility, you can combine SEPARATE and JOINED using bitwise OR:

```cpp
#define MY_ARGS(F) \
  F(include, I, "Include path", SEPARATE | JOINED, {}) \
  F(define, D, "Define macro", SEPARATE | JOINED, {})

DEFINE_ARGS(App, AppTable, MY_ARGS)
```

This supports ALL formats:
```bash
# Space-separated (SEPARATE)
./app -I /usr/include

# Equals-separated (SEPARATE)
./app -I=/usr/include
./app --include=/usr/include

# Direct attachment (JOINED)
./app -I/usr/include
./app --include/usr/include

# All formats can be mixed
./app -I/path1 -I=/path2 --include/path3
```

**Note**: This replaces the old `SEPARATE_OR_JOINED` kind. Now you simply use `SEPARATE | JOINED`.

### Joined Options (Compiler-style Flags)

Parameters:
- `name`: Option identifier (use underscores, e.g., `log_lvl`)
- `short_name`: Single character short form (e.g., `p` for `-p`)
- `help_text`: Description shown in help message
- `SEPARATE`, `FLAG`, `JOINED`, or `SEPARATE_OR_JOINED`: Kind identifier
- `allowed_values`: For SEPARATE/JOINED - list of valid values like `{"json", "xml"}`, use `{}` for any value

### Macro Structure

```
#define MY_ARGS(F)                                               \
  F(option1, o, "Option 1", SEPARATE, {})                        \
  F(option2, O, "Option 2", SEPARATE, {"a", "b", "c"})           \
  F(joined1, j, "Joined opt", JOINED, {"x", "y"})                \
  F(flag1, f, "Flag 1", FLAG, {})                                \
  F(flag2, F, "Flag 2", FLAG, {})

DEFINE_ARGS(EnumName, TableName, MY_ARGS)
```

This generates:
- Enum: `OPT_option1`, `OPT_option2`, `OPT_joined1`, `OPT_flag1`, `OPT_flag2`
- OptionTable: `TableName` for the parser

## Option Groups (Advanced)

Organize related options into logical groups with automatic grouped help display.

### Defining Option Groups

```cpp
#include "cpp_args.hpp"
#include <iostream>

// Define option groups
#define MY_GROUPS(F)                                                           \
  F(Server, "Server Options")                                                  \
  F(Database, "Database Options")                                              \
  F(General, "General Options")

// Define options with group assignments
#define MY_ARGS(F)                                                             \
  F(Server, host, H, "Server hostname", SEPARATE, {})                          \
  F(Server, port, p, "Server port", SEPARATE, {"8080", "9090"})                \
  F(Database, db, d, "Database type", SEPARATE, {"mysql", "postgres"})         \
  F(Database, connection, c, "Connection string", SEPARATE, {})                \
  F(General, verbose, v, "Verbose mode", FLAG, {})                             \
  F(General, help, h, "Print help", FLAG, {})

DEFINE_ARGS_WITH_GROUP(MyApp, MyGroups, MyAppTable, MY_ARGS, MY_GROUPS)

int main(int argc, char* argv[]) {
  // Use constructor with groups support
  ArgumentParser parser(MyAppTable, MyAppTableGroups);
  
  if (!parser.Parse(argc, argv)) {
    std::cerr << "Failed to parse arguments" << std::endl;
    return 1;
  }
  
  if (parser.HasArg(OPT_help)) {
    parser.PrintHelp();  // Help will be displayed grouped by category
    return 0;
  }
  
  // Access options normally
  if (parser.HasArg(OPT_host)) {
    std::cout << "Host: " << parser.GetArgValue(OPT_host) << std::endl;
  }
  
  // Query group ID for an option
  int groupId = parser.GetGroupId(OPT_host);  // Returns GRP_Server
  int groupId2 = parser.GetGroupId(OPT_verbose);  // Returns GRP_General

  return 0;
}
```

### Group Features

1. **Grouped Help Display**: `PrintHelp()` automatically organizes options by group with section headers
2. **Group ID Lookup**: Use `GetGroupId(optionId)` to query which group an option belongs to
3. **Flexible Grouping**: Options can be assigned to any group at definition time
4. **Empty Group Names**: Use empty string `""` as group name for ungrouped options

### Usage Example

```bash
./my_app --help
```

Output:
```
Server Options:
--host                                                                (-H)
    Server hostname
--port [Values: 8080|9090]                                            (-p)
    Server port

Database Options:
--db [Values: mysql|postgres]                                         (-d)
    Database type
--connection                                                          (-c)
    Connection string

General Options:
--verbose [flag]                                                      (-v)
    Verbose mode
--help [flag]                                                         (-h)
    Print help
```

### Macro Structure for Groups

```cpp
#define MY_GROUPS(F)                                               \
  F(GroupEnumName, "Display Name")                                 \
  F(AnotherGroup, "Another Display Name")

#define MY_ARGS(F)                                                 \
  F(GroupEnumName, option_name, short_name, "help", kind, {allowed})

DEFINE_ARGS_WITH_GROUP(EnumName, GroupEnumName, TableName, 
                       ArgsMacro, GroupsMacro)
```

This generates:

- Option enum: `OPT_option_name`, etc.
- Group enum: `GRP_GroupEnumName`, `GRP_AnotherGroup`, etc.
- OptionTable: `TableName` with groupId for each option
- GroupTable: `TableNameGroups` with group metadata

## API Reference

### ArgumentParser Methods

| Method | Description |
|--------|-------------|
| `Parse(argc, argv)` | Parse command-line arguments, returns `true` on success |
| `HasArg(id)` | Check if an argument was provided (works for both options and flags) |
| `GetArgValue(id)` | Get the value of an option. **If option specified multiple times, returns last value** |
| `GetAllArgValues(id)` | Get **all values** of an option as `const std::vector<std::string>&`. Returns empty vector if not provided |
| `PrintHelp()` | Display usage information with all options and flags |
| `GetInputs()` | Get positional inputs (non-option arguments) as `const std::vector<std::string>&` |
| `SetAllowUnknown(bool)` | Configure whether unknown options should cause parse failure |
| `GetUnknown()` | Get list of unknown options as `const std::vector<std::string>&` |

### Access Patterns

```cpp
// Check if an option/flag was provided
if (parser.HasArg(OPT_port)) {
    // Get last value (backward compatible)
    std::string value = parser.GetArgValue(OPT_port);
    // use value...
}

// Get all values if option specified multiple times
const auto& allValues = parser.GetAllArgValues(OPT_config);
for (const auto& val : allValues) {
    std::cout << "Config: " << val << std::endl;
}

// Example: Multiple occurrences of same option
// Command: ./my_app -c config1.txt -c config2.txt -c config3.txt
parser.GetArgValue(OPT_config);        // Returns "config3.txt" (last value)
parser.GetAllArgValues(OPT_config);    // Returns {"config1.txt", "config2.txt", "config3.txt"}

// Check a flag
if (parser.HasArg(OPT_verbose)) {
    // verbose mode is enabled
}

// Get positional inputs
const auto& inputs = parser.GetInputs();
for (const auto& input : inputs) {
    std::cout << "Input: " << input << std::endl;
}

// Handle unknown options
parser.SetAllowUnknown(true);  // Don't fail on unknown options
if (!parser.Parse(argc, argv)) {
    // handle parse error...
}

const auto& unknown = parser.GetUnknown();
for (const auto& opt : unknown) {
    std::cout << "Unknown option: " << opt << std::endl;
}

// Show help
parser.PrintHelp();
```

## Building & Testing

### Prerequisites

- C++11 or later
- CMake 3.10+
- GCC or Clang

### Build Tests

```bash
mkdir build && cd build
cmake ..
make
```

### Run All Tests

```bash
make test
```

### Run Specific Test

```bash
ctest -R BasicParsing
ctest -R "ShortName|MultipleArgs"
```

## Project Structure

```
cpp-args/
├── cpp_args.hpp          # Header-only library (all you need!)
├── example.cpp           # Usage example
├── CMakeLists.txt        # Build configuration
├── README.md            # This file
└── test/                # Unit tests
    ├── test_basic.cpp
    ├── test_flag.cpp
    ├── test_allowed_values.cpp
    └── ...
```

## Key Benefits

1. **Zero dependencies** - Pure C++ standard library
2. **Compile-time safety** - Options defined at compile time, enum-based access
3. **Type-safe** - Use generated enum IDs instead of error-prone string literals
4. **User-friendly CLI** - Accepts both `--log_lvl` and `--log-lvl`
5. **Clean code** - X-Macros eliminate repetition, single definition location
6. **Flexible** - Mix options, flags, and joined options naturally in one macro

## Advanced Features

### Value Validation

Restrict option values to a predefined set:

```cpp
#define ARGS(F) \
  F(format, f, "Output format", SEPARATE, {"json", "xml", "text"}) \
  F(level, l, "Log level", SEPARATE, {"debug", "info", "warn", "error"}) \
  F(library, L, "Link library", JOINED, {"cuda", "stdc++", "pthread"})

DEFINE_ARGS(App, AppTable, ARGS)
```

Invalid values will cause parsing to fail with an error message.

### Joined Options (Compiler-style Flags)

Perfect for compiler-like interfaces where values are attached directly to option names:

```cpp
#define COMPILER_ARGS(F) \
  F(library, L, "Link library", JOINED, {"cuda", "stdc++", "pthread"}) \
  F(include, I, "Include path", JOINED, {}) \
  F(define, D, "Define macro", JOINED, {}) \
  F(output, o, "Output file", SEPARATE, {}) \
  F(verbose, v, "Verbose mode", FLAG, {})

DEFINE_ARGS(Compiler, CompilerTable, COMPILER_ARGS)
```

Usage (both short and long options supported):
```bash
g++ -Lcuda -I/usr/local/include -DNDEBUG --librarypthread -o app.out main.cpp
```

**Supported formats**:
- Short option with joined value: `-Lcuda`, `-I/path`
- Long option with joined value: `--librarycuda`, `--lpthread`
- **Not supported**: `--library=cuda` (use SEPARATE_OR_JOINED for `=` separator)

### Positional Inputs

Non-option arguments are automatically preserved as positional inputs:

```cpp
#define MY_ARGS(F) \
  F(output, o, "Output file", SEPARATE, {}) \
  F(verbose, v, "Verbose mode", FLAG, {})

DEFINE_ARGS(App, AppTable, MY_ARGS)

int main(int argc, char* argv[]) {
  ArgumentParser parser(AppTable);
  parser.Parse(argc, argv);
  
  // Get positional inputs (files, etc.)
  const auto& inputs = parser.GetInputs();
  // inputs contains all non-option arguments
}
```

```bash
# Everything that's not an option becomes an input
./app -o out.txt file1.txt file2.txt file3.txt
# inputs = ["file1.txt", "file2.txt", "file3.txt"]

./app file1.txt -v file2.txt -o out.txt file3.txt
# inputs = ["file1.txt", "file2.txt", "file3.txt"]
```

### Unknown Option Handling

Control how unknown options are handled:

```cpp
ArgumentParser parser(Table);

// By default, unknown options cause parse failure
parser.Parse(argc, argv);  // Returns false on unknown option

// Allow unknown options without error
parser.SetAllowUnknown(true);
if (parser.Parse(argc, argv)) {
    // Parse succeeded even with unknown options
    
    // Retrieve unknown options if needed
    const auto& unknown = parser.GetUnknown();
    for (const auto& opt : unknown) {
        std::cout << "Unknown: " << opt << std::endl;
    }
}
```

**Important**: Unknown options must start with `-` or `--`. Arguments without these prefixes are treated as positional inputs, not unknown options.

```cpp
// --fake-opt is unknown, "input" is treated as positional input
./app --fake-opt input.txt
# GetUnknown() returns ["--fake-opt"]
# GetInputs() returns ["input.txt"]
```

### Help Output

The library automatically generates formatted help messages:

```
Usage Options:
  --host, -H               Server hostname
  --port, -p               Server port [Values: 8080|9090]
  --library, -L            Link library [Values: cuda|stdc++]
  --verbose, -v            Enable verbose mode
  --help, -h               Print help message
```

---

See unit tests in `test/` directory for more comprehensive examples and edge cases.
