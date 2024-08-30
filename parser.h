#pragma once

#include <stdint.h>

enum parser_status {
    kArgParserSuccess,
    kArgParserShiftingArg,
};

enum parser_var_type {
    kTypeString,
};

// TODO change structure name
struct parser {
    uint16_t flag; // TODO
    char *short_name;
    char *long_name;
    int var_count;
    void **var_ptrs;
    enum parser_var_type *var_types;
};

struct parser_result {
    enum parser_status status; // TODO no one can get this, if processing error
    int parsed_argc_index; // Pointer to the last parsed argument
    int params_count;      // Parameters is non-flag argument
    char **parameters;
};

// TODO TBD
// give use a macro but not enum's name?
#define ARG_PARSER_VAR_TYPE enum parser_var_type

struct parser_result *ArgParser(int argc, int last_arg, char *argv[], struct parser *options, int opt_num,
                                void (*ErrorCallback)(void));
