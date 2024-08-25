#include "parser.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void GetNextArgumentWithType_(char argument[], void **var_ptr, enum parser_var_type var_type)
{
    switch (var_type) {
    case kTypeString:
        *var_ptr = argument;
        break;
    }

    return;
}

struct parser_result *ArgParser(int argc, int last_arg, char *argv[], struct parser *options, int opt_num)
{
    struct parser_result *res = malloc(sizeof(struct parser_result)); // res -> result

    for (; last_arg < argc; last_arg++) {
        // TODO add "/", "+" prefix
        if (strncmp(argv[last_arg], "--", 2) == 0) {
            // TODO code optimization
            // TODO implement "="
            for (int i = 0; i < opt_num; i++) {
                if (strcmp(options[i].long_name, argv[last_arg] + 2) != 0) // TODO make it be configurable
                    continue;
                last_arg++;
                if (last_arg < argc)
                    GetNextArgumentWithType_(argv[last_arg], options[i].variable_ptr, options[i].var_type);
            }
            continue;
        } else if (strncmp(argv[last_arg], "-", 1) == 0) {
            // TODO same as "--"
            continue;
        } else {
            res->params_count += 1;
            size_t this_arg_size = strlen(argv[last_arg]) + 1;

            res->parameters = realloc(res->parameters, sizeof(char **) * res->params_count);
            res->parameters[res->params_count - 1] = argv[last_arg];
        }
    }
    // last_arg equal with argc, but here need a valid index
    res->parsed_argc_index = last_arg - 1;

    // TODO find a way to free those malloc()
    return res;
}
