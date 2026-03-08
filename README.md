# C++ Argument Parser

A lightweight, header-only command-line argument parser for C++

## Features

- **Header-only library** - Just include `cpp_args.hpp` and you're ready to go
- **Flexible naming** - Supports both `--log_lvl` and `--log-lvl` formats (underscores automatically converted to dashes)
- **Short & long options** - Support for both `-p` and `--port` style arguments
- **Flag support** - Boolean flags that don't require values (e.g., `--help`, `-v`)
- **Value validation** - Define allowed values for specific arguments
- **X-Macros** - Clean, maintainable option definitions using X-Macro pattern
- **Easy to use** - Simple API with minimal boilerplate

## Quick Start

### 1. Define Your Options

**With regular options (require values):**

```cpp
#include "cpp_args.hpp"
#include <iostream>

#define MY_APP_OPTIONS(V) \
    V(port,     p, "Server port number",   {}) \
    V(log_lvl,  l, "Logging verbosity",   {"debug", "info"}) \
    V(output,   o, "Output file",          {})

DEFINE_ARGUMENTS(AppOption, AppTable, MY_APP_OPTIONS)
```

**With flags (no value required):**

```cpp
// Define flags only
#define MY_APP_FLAGS(F) \
    F(help,   h, "Print help information") \
    F(verbose, v, "Enable verbose mode")

DEFINE_FLAGS(AppFlag, AppFlagTable, MY_APP_FLAGS)
```

**Mixed options and flags:**

```cpp
#define MY_OPTIONS(V) \
    V(port,     p, "Server port number",   {}) \
    V(log_lvl,  l, "Logging verbosity",   {"debug", "info"})

#define MY_FLAGS(V) \
    V(help,   h, "Print help information") \
    V(verbose, v, "Enable verbose mode")

DEFINE_ARGUMENTS_WITH_FLAGS(MyOption, MyTable, MY_OPTIONS, MY_FLAGS)
```

### 2. Parse Arguments

```cpp
int main(int argc, char* argv[]) {
    ArgumentParser parser(AppTable);

    if (!parser.Parse(argc, argv)) {
        parser.PrintHelp();
        return 1;
    }

    // Access parsed values for regular options
    if (parser.HasArg(OPT_port)) {
        std::cout << "Port: " << parser.GetArgValue(OPT_port) << std::endl;
    }

    if (parser.HasArg(OPT_log_lvl)) {
        std::cout << "Log Level: " << parser.GetArgValue(OPT_log_lvl) << std::endl;
    }

    // Check flags (they don't have values, just presence)
    if (parser.HasArg(OPT_help)) {
        parser.PrintHelp();
        return 0;
    }

    if (parser.HasArg(OPT_verbose)) {
        std::cout << "Verbose mode enabled" << std::endl;
    }

    return 0;
}
```

### 3. Run Your Application

```
# Regular options with values
./my_app --port 8080 --log-lvl debug
./my_app -p 8080 -l info
./my_app --port 8080 --log_lvl debug  # Underscore works too!

# Flags (no value needed)
./my_app --help
./my_app -v
./my_app --verbose --port 8080

# Mixed usage
./my_app --help
./my_app -v --port 8080 --log-lvl info
```

## API Reference

### Macro Definition

**For options (require values):**

```cpp
#define OPTIONS_MACRO(V) \
    V(name, short_name, help_text, {allowed_values})
```

- `name`: Option identifier (use underscores, e.g., `log_lvl`)
- `short_name`: Single character short form (e.g., `l` for `-l`)
- `help_text`: Description shown in help message
- `allowed_values`: Optional list of valid values (empty `{}` means any value)

**For flags (no value required):**

```cpp
#define FLAGS_MACRO(F) \
    F(name, short_name, help_text)
```

- `name`: Flag identifier (use underscores, e.g., `verbose`)
- `short_name`: Single character short form (e.g., `v` for `-v`)
- `help_text`: Description shown in help message

### DEFINE_ARGUMENTS Macros

**`DEFINE_ARGUMENTS(EnumName, TableName, OptionsMacro)`**
- Defines options that require values
- Use for regular command-line arguments

**`DEFINE_FLAGS(EnumName, TableName, FlagsMacro)`**
- Defines flags that don't require values
- Use for boolean switches like `--help`, `-v`

**`DEFINE_ARGUMENTS_WITH_FLAGS(EnumName, TableName, OptionsMacro, FlagsMacro)`**
- Combines both options and flags in a single definition
- Most flexible option for mixed usage

### ArgumentParser Methods

| Method | Description |
|--------|-------------|
| `Parse(argc, argv)` | Parse command-line arguments, returns `true` on success |
| `HasArg(id)` | Check if an argument was provided |
| `GetArgValue(id)` | Get the value of an argument |
| `PrintHelp()` | Display usage information |

### Generated Enum

The macro generates an enum with format `OPT_<name>` for each option:
```cpp
OPT_port
OPT_log_lvl
OPT_output
```

## Example Usage

### Regular Options Example

```cpp
#include "cpp_args.hpp"
#include <iostream>

#define OPTIONS(V) \
    V(input,  i, "Input file",      {}) \
    V(output, o, "Output file",     {}) \
    V(format, f, "Output format",   {"json", "xml", "text"})

DEFINE_ARGUMENTS(Options, OptionTable, OPTIONS)

int main(int argc, char* argv[]) {
    ArgumentParser parser(OptionTable);
    
    if (!parser.Parse(argc, argv)) {
        parser.PrintHelp();
        return 1;
    }
    
    std::cout << "Input: " << parser.GetArgValue(OPT_input) << std::endl;
    std::cout << "Output: " << parser.GetArgValue(OPT_output) << std::endl;
    std::cout << "Format: " << parser.GetArgValue(OPT_format) << std::endl;
    
    return 0;
}
```

### Flags Example

```cpp
#include "cpp_args.hpp"
#include <iostream>

#define FLAGS(F) \
    F(help,    h, "Print this help message") \
    F(verbose, v, "Enable verbose output") \
    F(debug,   d, "Enable debug mode")

DEFINE_FLAGS(Flags, FlagTable, FLAGS)

int main(int argc, char* argv[]) {
    ArgumentParser parser(FlagTable);
    
    if (!parser.Parse(argc, argv)) {
        parser.PrintHelp();
        return 1;
    }
    
    if (parser.HasArg(OPT_help)) {
        parser.PrintHelp();
        return 0;
    }
    
    if (parser.HasArg(OPT_verbose)) {
        std::cout << "Verbose mode ON" << std::endl;
    }
    
    if (parser.HasArg(OPT_debug)) {
        std::cout << "Debug mode ON" << std::endl;
    }
    
    return 0;
}
```

### Mixed Options and Flags Example

```cpp
#include "cpp_args.hpp"
#include <iostream>

#define OPTIONS(V) \
    V(config, c, "Configuration file", {}) \
    V(port,   p, "Server port",        {"8080", "443"})

#define FLAGS(F) \
    F(help,    h, "Show help") \
    F(daemon,  d, "Run as daemon") \
    F(quiet,   q, "Suppress output")

DEFINE_ARGUMENTS_WITH_FLAGS(App, AppTable, OPTIONS, FLAGS)

int main(int argc, char* argv[]) {
    ArgumentParser parser(AppTable);
    
    if (!parser.Parse(argc, argv)) {
        parser.PrintHelp();
        return 1;
    }
    
    if (parser.HasArg(OPT_help)) {
        parser.PrintHelp();
        return 0;
    }
    
    if (parser.HasArg(OPT_daemon)) {
        std::cout << "Running as daemon..." << std::endl;
    }
    
    if (parser.HasArg(OPT_config)) {
        std::cout << "Config: " << parser.GetArgValue(OPT_config) << std::endl;
    }
    
    return 0;
}
```

See unit tests in `test/` for more example usage.

## Building & Testing

### Prerequisites

- C++11 or later
- CMake 3.10+
- GCC or Clang

### Build Tests

```bash
mkdir build
cd build
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

### Verbose Test Output

```bash
ctest --verbose
```

## Project Structure

```
cpp-args/
├── cpp_args.hpp          # Header-only library
├── example.cpp           # Usage example
├── CMakeLists.txt        # Build configuration
├── README.md            # This file
└── test/                # Unit tests
```

## Key Benefits

1. **No dependencies** - Pure C++ standard library
2. **Compile-time safety** - Options defined at compile time
3. **Type-safe access** - Use generated enum IDs instead of string literals
4. **User-friendly** - Accepts both `--log_lvl` and `--log-lvl`
5. **Clean code** - X-Macros eliminate repetition

## License

This project is open source. Feel free to use it in your projects!
