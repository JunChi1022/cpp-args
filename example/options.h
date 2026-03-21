#pragma once

#include "../cpp_args.hpp"

// Define option groups
#define MY_GROUPS(F)                                                           \
  F(General, "")                                                               \
  F(Frontend, "Frontend Options")                                              \
  F(Backend, "Backend Options")

// Define options with group assignments
#define MY_ARGS(F)                                                             \
  F(General, verbose, v, "Verbose mode", FLAG, {})                             \
  F(General, h, , "Display this information.", FLAG | SHORT, {})               \
  F(General, save_temps, save_temps, "Do not delete intermediate files.",      \
    FLAG, {})                                                                  \
  F(Frontend, std, std, "Assume that the input sources are for <standard>.",   \
    EQ_JOIN, {})

#define Alias(A) A(keep, keep, save_temps)

DEFINE_ALIAS(MyAliasTable, Alias)

DEFINE_ARGS_WITH_GROUP(MyApp, MyGroups, MyAppTable, MY_ARGS, MY_GROUPS)

#include "../cpp_args_macro_cleaner.inc"
