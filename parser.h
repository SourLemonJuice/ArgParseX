#pragma once

#include <stdbool.h>
#include <stdint.h>

enum parser_status {
    kArgParserSuccess,
    kArgParserUnknownError,
    kArgParserShiftingArg,
    kArgParserFlagParamFormatIncorrect,
};

// TODO not implemented all
enum parser_var_method {
    kMethodToggle,
    kMethodSingleVariable, // TODO
    kMethodMultipleVariable,
    // TODO set bool
};

enum parser_var_type {
    kTypeString,
    // TODO
    kTypeInteger,
    kTypeBoolean,
    kTypeFloat,
    kTypeDouble,
};

// flag's range is uint16_t
#define ARG_GROUP_MANDATORY_ASSIGNER 0b1
#define ARG_GROUP_MANDATORY_DELIMITER 0b1 << 1

// set separator to '\0' to skip them
struct flag_group {
    uint16_t flag;
    // prefix of flag, like the "--" of "--flag"
    char *prefix;
    // parameter assigner
    char assigner;
    // parameter delimiter
    char delimiter;
};

// TODO change structure name
struct parser {
    int group_idx;
    enum parser_var_method method;
    // name of flag, like the "flagName" of "--flagName"
    char *name;
    // only "toggle" method use it, made that's simple
    bool *toggle_ptr;
    int var_count;
    enum parser_var_type *var_types;
    // a list of secondary pointer of actual variable
    void **var_ptrs;
};

struct parser_result {
    enum parser_status status;
    int parsed_argc_index; // Pointer to the last parsed argument
    int params_count;      // Parameters is non-flag argument
    char **parameters;
};

// TODO TBD
// give user a macro but not enum's name?
#define ARG_PARSER_VAR_TYPE enum parser_var_type

struct parser_result *ArgParser(int argc, int last_arg, char *argv[], struct flag_group *groups, struct parser *opts,
                                int opt_count, void (*ErrorCallback)(struct parser_result *));
