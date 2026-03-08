# C++ Argument Parser

A lightweight, header-only command-line argument parser for C++

## Features

- **Header-only library** - Just include `cpp_args.hpp` and you're ready to go
- **Flexible naming** - Supports both `--log_lvl` and `--log-lvl` formats (underscores automatically converted to dashes)
- **Short & long options** - Support for both `-p` and `--port` style arguments
- **Flag support** - Boolean flags that don't require values (e.g., `--help`, `-v`)
- **Value validation** - Define allowed values for specific arguments
- **X-Macros** - Clean, maintainable option definitions using X-Macro pattern
- **Unified syntax** - Define options and flags together with a single macro

## Quick Start

### Complete Example

```cpp
#include "cpp_args.hpp"
#include <iostream>

// Define all arguments in one place - mix options and flags freely
#define MY_APP_ARGS(F)                                                         \
  F(host, H, "Server hostname", OPTION, {})                                    \
  F(port, p, "Server port", OPTION, {"8080", "9090"})                          \
  F(verbose, v, "Enable verbose mode", FLAG, {})                               \
  F(help, h, "Print help message", FLAG, {})

DEFINE_ARGS(MyApp, MyAppTable, MY_APP_ARGS)

int main(int argc, char* argv[]) {
  ArgumentParser parser(MyAppTable);
  
  if (!parser.Parse(argc, argv)) {
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
  
  return 0;
}
```

### Usage Examples

```bash
# Options with values
./my_app --host localhost --port 8080
./my_app -H localhost -p 9090
./my_app --host=localhost --port=8080

# Flags (no value needed)
./my_app --verbose
./my_app -v
./my_app --help

# Mixed usage
./my_app -v --host localhost --port 8080
./my_app --verbose -H 127.0.0.1 -p 8080

# Underscore/dash flexibility (both work!)
./my_app --log-lvl debug    # with dash
./my_app --log_lvl debug    # with underscore
```

## Syntax Guide

### Defining Arguments

**Options** (require values):
```cpp
F(name, short_name, "help text", OPTION, {allowed_values})
```

**Flags** (no value required):
```cpp
F(name, short_name, "help text", FLAG, {})
```

Parameters:
- `name`: Option identifier (use underscores, e.g., `log_lvl`)
- `short_name`: Single character short form (e.g., `p` for `-p`)
- `help_text`: Description shown in help message
- `OPTION` or `FLAG`: Kind identifier
- `allowed_values`: For OPTION only - list of valid values like `{"json", "xml"}`, use `{}` for any value

### Macro Structure

```cpp
#define MY_ARGS(F)                                               \
  F(option1, o, "Option 1", OPTION, {})                          \
  F(option2, O, "Option 2", OPTION, {"a", "b", "c"})             \
  F(flag1, f, "Flag 1", FLAG, {})                                \
  F(flag2, F, "Flag 2", FLAG, {})

DEFINE_ARGS(EnumName, TableName, MY_ARGS)
```

This generates:
- Enum: `OPT_option1`, `OPT_option2`, `OPT_flag1`, `OPT_flag2`
- OptionTable: `TableName` for the parser

## API Reference

### ArgumentParser Methods

| Method | Description |
|--------|-------------|
| `Parse(argc, argv)` | Parse command-line arguments, returns `true` on success |
| `HasArg(id)` | Check if an argument was provided (works for both options and flags) |
| `GetArgValue(id)` | Get the value of an option (empty string for flags or missing args) |
| `PrintHelp()` | Display usage information with all options and flags |

### Access Patterns

```cpp
// Check if an option/flag was provided
if (parser.HasArg(OPT_port)) {
    std::string value = parser.GetArgValue(OPT_port);
    // use value...
}

// Check a flag
if (parser.HasArg(OPT_verbose)) {
    // verbose mode is enabled
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
6. **Flexible** - Mix options and flags naturally in one macro

## Advanced Features

### Value Validation

Restrict option values to a predefined set:

```cpp
#define ARGS(F) \
  F(format, f, "Output format", OPTION, {"json", "xml", "text"}) \
  F(level, l, "Log level", OPTION, {"debug", "info", "warn", "error"})

DEFINE_ARGS(App, AppTable, ARGS)
```

Invalid values will cause parsing to fail with an error message.

### Help Output

The library automatically generates formatted help messages:

```
Usage Options:
  --host, -H               Server hostname
  --port, -p               Server port [Values: 8080|9090]
  --verbose, -v            Enable verbose mode [flag]
  --help, -h               Print help message [flag]
```

---

See unit tests in `test/` directory for more comprehensive examples and edge cases.
