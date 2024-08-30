#include "parser.h"

#include <iso646.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
    An unified data of this unit
 */
struct ArgParserData_ {
    // the result structure of the main function
    struct parser_result *res;
    // error callback function pointer, NULL is exit(EXIT_FAILURE)
    void (*ErrorCallback)(struct parser_result *);
    int argc;
    char **argv;
    // the arg index being processed, it records the first unprocessed arg
    int arg_ind;
    int config_count;
    struct parser *configs;
};

static void CallError_(struct ArgParserData_ *data)
{
    data->res->status = kArgParserShiftingArg;

    if (data->ErrorCallback == NULL)
        exit(EXIT_FAILURE);
    else
        data->ErrorCallback(data->res);
}

static void GetFlagArguments_(struct ArgParserData_ *data, int flag_ind /* flag index */)
{
    for (int i = 0; i < data->configs[flag_ind].var_count; /* increment at below */) {
        switch (data->configs[flag_ind].var_types[i]) {
        case kTypeString:
            *(char **)data->configs[flag_ind].var_ptrs[i] = data->argv[data->arg_ind];
            break;
        }

        data->arg_ind++;
        i++;
        // if args has reached the end, but this flag still need more parameter, call the error
        if (not(data->arg_ind < data->argc) and i < data->configs[flag_ind].var_count)
            CallError_(data);
    }
}

/*
    If ErrorCallback is NULL then use exit(EXIT_FAILURE),
    if is a function then need accept a result structure, this should help

    TODO I don't like this parameter style
 */
struct parser_result *ArgParser(int argc, int last_arg, char *argv[], struct parser *opts, int opt_count,
                                void (*ErrorCallback)(struct parser_result *))
{
    struct ArgParserData_ data = {
        .res = malloc(sizeof(struct parser_result)), // res -> result
        .ErrorCallback = ErrorCallback,
        .argc = argc,
        .argv = argv,
        .arg_ind = last_arg,
        .configs = opts,
        .config_count = opt_count,
    };

    for (; data.arg_ind < data.argc; data.arg_ind++) {
        // TODO add "/", "+" prefix
        // all we need to do is just let the user set the prefix
        if (strncmp(data.argv[data.arg_ind], "--", 2) == 0) {
            // TODO implement "="
            for (int i = 0; i < data.config_count; i++) {
                if (strcmp(data.configs[i].long_name, data.argv[data.arg_ind] + 2) != 0)
                    continue;
                data.arg_ind++;
                GetFlagArguments_(&data, i);
            }
            continue;
        } else if (strncmp(argv[last_arg], "-", 1) == 0) {
            // TODO same as "--"
            continue;
        } else {
            data.res->params_count += 1;
            size_t this_arg_size = strlen(data.argv[data.arg_ind]) + 1;

            data.res->parameters = realloc(data.res->parameters, sizeof(char **) * data.res->params_count);
            data.res->parameters[data.res->params_count - 1] = data.argv[data.arg_ind];
        }
    }
    // last_arg equal with argc now, but here need a valid index
    data.res->parsed_argc_index = data.arg_ind - 1;

    // TODO find a way to free those malloc()
    return data.res;
}
