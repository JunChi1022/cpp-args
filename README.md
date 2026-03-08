# C++ Argument Parser

A lightweight, header-only command-line argument parser for C++

## Features

- **Header-only library** - Just include `cpp_args.hpp` and you're ready to go
- **Flexible naming** - Supports both `--log_lvl` and `--log-lvl` formats (underscores automatically converted to dashes)
- **Short & long options** - Support for both `-p` and `--port` style arguments
- **Value validation** - Define allowed values for specific arguments
- **X-Macros** - Clean, maintainable option definitions using X-Macro pattern
- **Easy to use** - Simple API with minimal boilerplate

## Quick Start

### 1. Define Your Options

```cpp
#include "cpp_args.hpp"
#include <iostream>

#define MY_APP_OPTIONS(V) \
    V(port,     p, "Server port number",   {}) \
    V(log_lvl,  l, "Logging verbosity",   {"debug", "info"}) \
    V(output,   o, "Output file",          {})

DEFINE_ARGUMENTS(AppOption, AppTable, MY_APP_OPTIONS)
```

### 2. Parse Arguments

```cpp
int main(int argc, char* argv[]) {
    ArgumentParser parser(AppTable);

    if (!parser.Parse(argc, argv)) {
        parser.PrintHelp();
        return 1;
    }

    // Access parsed values
    if (parser.HasArg(OPT_port)) {
        std::cout << "Port: " << parser.GetArgValue(OPT_port) << std::endl;
    }

    if (parser.HasArg(OPT_log_lvl)) {
        std::cout << "Log Level: " << parser.GetArgValue(OPT_log_lvl) << std::endl;
    }

    return 0;
}
```

### 3. Run Your Application

```bash
./my_app --port 8080 --log-lvl debug
./my_app -p 8080 -l info
./my_app --port 8080 --log_lvl debug  # Underscore works too!
```

## API Reference

### Macro Definition

```cpp
#define OPTIONS_MACRO(V) \
    V(name, short_name, help_text, {allowed_values})
```

- `name`: Option identifier (use underscores, e.g., `log_lvl`)
- `short_name`: Single character short form (e.g., `l` for `-l`)
- `help_text`: Description shown in help message
- `allowed_values`: Optional list of valid values (empty `{}` means any value)

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

See unit tests in test for example usage
```

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
    ├── test_utils.h              # Test utilities
    ├── test_basic.cpp            # Basic parsing tests
    ├── test_underscore_dash.cpp  # Underscore/dash conversion tests
    ├── test_underscore_input.cpp # Underscore input tests
    ├── test_short_name.cpp       # Short name tests
    ├── test_allowed_values.cpp   # Value validation tests
    ├── test_multiple_args.cpp    # Multiple arguments tests
    ├── test_unknown_arg.cpp      # Unknown argument tests
    └── test_missing_value.cpp    # Missing value tests
```

## Key Benefits

1. **No dependencies** - Pure C++ standard library
2. **Compile-time safety** - Options defined at compile time
3. **Type-safe access** - Use generated enum IDs instead of string literals
4. **User-friendly** - Accepts both `--log_lvl` and `--log-lvl`
5. **Clean code** - X-Macros eliminate repetition

## License

This project is open source. Feel free to use it in your projects!
