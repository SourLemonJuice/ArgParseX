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
    void (*ErrorCallback)(void);
    int argc;
    char **argv;
    // the arg index being processed, it records the first unprocessed arg
    int arg_ind;
    struct parser *configs;
    // TODO custom parameter quantity
    int config_count;
};

static void ShiftArgIndex_(struct ArgParserData_ *data)
{
    data->arg_ind++;
    if (not(data->arg_ind < data->argc)) {
        data->res->status = kArgParserShiftingArg;

        if (data->ErrorCallback == NULL)
            exit(EXIT_FAILURE);
        data->ErrorCallback();
    }

    return;
}

static void GetFlagArguments_(struct ArgParserData_ *data, int flag_ind)
{
    for (int i = 0; i < data->configs[flag_ind].var_count; i++) {
        switch (data->configs[flag_ind].var_types[i]) {
        case kTypeString:
            *(char **)data->configs[flag_ind].var_ptrs[i] = data->argv[data->arg_ind];
            break;
        }
        ShiftArgIndex_(data);
    }

    return;
}

/*
    If ErrorCallback is NULL then use exit(EXIT_FAILURE)

    TODO I don't like this parameter style
 */
struct parser_result *ArgParser(int argc, int last_arg, char *argv[], struct parser *options, int opt_num,
                                void (*ErrorCallback)(void))
{
    struct ArgParserData_ data = {
        .res = malloc(sizeof(struct parser_result)), // res -> result
        .ErrorCallback = ErrorCallback,
        .argc = argc,
        .argv = argv,
        .arg_ind = last_arg,
        .configs = options,
        .config_count = opt_num,
    };

    for (; data.arg_ind < argc; data.arg_ind++) {
        // TODO add "/", "+" prefix
        if (strncmp(data.argv[data.arg_ind], "--", 2) == 0) {
            // TODO implement "="
            for (int i = 0; i < data.config_count; i++) {
                if (strcmp(data.configs[i].long_name, data.argv[data.arg_ind] + 2) != 0)
                    continue;
                ShiftArgIndex_(&data);
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
