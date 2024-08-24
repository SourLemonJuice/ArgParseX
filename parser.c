#include "parser.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct parser_result *ArgParser(int argc, int last_arg, char *argv[], struct parser *options, int opt_num)
{
    struct parser_result *res = malloc(sizeof(struct parser_result)); // res -> result

    for (; last_arg < argc; last_arg++) {
        if (strncmp(argv[last_arg], "--", 2) == 0) {
            // TODO
            continue;
        } else if (strncmp(argv[last_arg], "-", 1) == 0) {
            // TODO
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

    return res;
}
