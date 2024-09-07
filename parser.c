#include "parser.h"

#include <iso646.h>
#include <stdbool.h>
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
    // no no no, try to make it to record the last processed arg
    int arg_ind;
    int config_count;
    struct parser *configs;
};

/*
    Call the error callback registered by the user
 */
static void CallError_(struct ArgParserData_ *data, enum parser_status status)
{
    data->res->status = status;

    if (data->ErrorCallback == NULL)
        exit(EXIT_FAILURE);
    else
        data->ErrorCallback(data->res);
}

/*
    Check if the arg has reached the boundary
 */
static bool ArgIndexWithinBoundary_(struct ArgParserData_ *data)
{
    if (data->arg_ind < data->argc)
        return true;
    else
        return false;
}

/*
    Converting a string to a specific type.
    And assign it to a pointer
 */
static void ConvertingStringType_(char *string, enum parser_var_type type, void **ptr)
{
    // remember to change the first level pointer, but not just change secondary one
    switch (type) {
    case kTypeString:
        *ptr = string; // TODO debug
        break;
    case kTypeInteger:
        // TODO
        break;
    case kTypeBoolean:
        // TODO
        break;
    case kTypeFloat:
        // TODO
        break;
    case kTypeDouble:
        // TODO
        break;
    }
}

/*
    Invert the bool value of "toggle_ptr" in flag config
 */
static void GetFlagBoolToggle_(struct ArgParserData_ *data, int conf_ind)
{
    // emm, so long, but... not important?
    *data->configs[conf_ind].toggle_ptr = not *data->configs[conf_ind].toggle_ptr;
}

// TODO implement the other methods
static void GetFlagMultiArgs_(struct ArgParserData_ *data, int conf_ind /* config index */)
{
    for (int i = 0; i < data->configs[conf_ind].var_count; /* increment at below */) {
        // i know it's confusing...
        data->arg_ind++;
        ConvertingStringType_(data->argv[data->arg_ind], data->configs[conf_ind].var_types[i],
                              data->configs[conf_ind].var_ptrs[i] /* passing secondary pointer */);

        i++;
        // if args has reached the end, but this flag still need more parameter, call the error
        if (ArgIndexWithinBoundary_(data) == false and i < data->configs[conf_ind].var_count)
            CallError_(data, kArgParserShiftingArg);
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

    for (; ArgIndexWithinBoundary_(&data) == true; data.arg_ind++) {
        // is this a flag?
        bool conf_matched = false;
        for (int i = 0; i < data.config_count; i++) {
            int prefix_len = strlen(data.configs[i].prefix);
            // matching prefix
            if (strncmp(data.configs[i].prefix, data.argv[data.arg_ind], prefix_len) != 0)
                continue;

            // matching name
            char *assigner_ptr = NULL;
            int name_len = 0;
            // if assigner set to '\0', skip it
            if (data.configs[i].assigner == '\0') {
                name_len = strlen(data.argv[data.arg_ind]) - prefix_len;
            } else {
                assigner_ptr = strchr(data.argv[data.arg_ind], data.configs[i].assigner);
                if (assigner_ptr == NULL) {
                    // TODO
                }
                name_len = assigner_ptr - data.argv[data.arg_ind];
                name_len -= prefix_len;
            }
            if (strncmp(data.configs[i].name, data.argv[data.arg_ind] + prefix_len, name_len) != 0)
                    continue;

            // get args
            switch (data.configs[i].method) {
            case kMethodToggle:
                GetFlagBoolToggle_(&data, i);
                break;
            case kMethodSingleVariable:
                // TODO
                break;
            case kMethodMultipleVariable:
                GetFlagMultiArgs_(&data, i);
                break;
            }
            // don't continue to iterate other configs
            conf_matched = true;
            break;
        }
        if (conf_matched == true)
            continue;

        // no any config matched this arg. so... it's a parameter, add it to result structure
        // get flag functions may have run out of args, so check it at first
        if (ArgIndexWithinBoundary_(&data) == true) {
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
