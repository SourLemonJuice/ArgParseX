#pragma once

#include <stdbool.h>
#include <stdint.h>

enum parser_status {
    kArgParserSuccess,
    kArgParserShiftingArg,
};

// TODO not implemented all
enum parser_var_method {
    kMethodToggle,
    kMethodSingleVariable,
    kMethodMultipleVariable,
};

enum parser_var_type {
    kTypeString,
    // TODO
    kTypeInteger,
    kTypeBoolean,
    kTypeFloat,
    kTypeDouble,
};

// TODO change structure name
struct parser {
    enum parser_var_method method;
    char *name;
    char *prefix;
    // it can't be a '\0'(like "") just null or another thing
    char *opr_eq; // TODO not implement
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

struct parser_result *ArgParser(int argc, int last_arg, char *argv[], struct parser *options, int opt_num,
                                void (*ErrorCallback)(struct parser_result *));
