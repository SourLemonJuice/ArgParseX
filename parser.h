#pragma once

#include <stdint.h>

enum parser_var_type {
    kTypeString,
};

struct parser {
    uint16_t flag;
    char *short_name;
    char *long_name;
    void *variable_ptr;
    enum parser_var_type var_type;
};

struct parser_result {
    int parsed_argc_index; // Pointer to the last parsed argument
    int params_count;      // Parameters is non-flag argument
    char **parameters;
};

struct parser_result *ArgParser(int argc, int last_arg, char *argv[], struct parser *options, int opt_num);
